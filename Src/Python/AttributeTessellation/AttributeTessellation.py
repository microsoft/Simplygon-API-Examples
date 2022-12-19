# Copyright (c) Microsoft Corporation. 
# Licensed under the MIT License. 

import math
import os
import sys
import glob
import gc
import threading

from pathlib import Path
from simplygon10 import simplygon_loader
from simplygon10 import Simplygon


def LoadScene(sg: Simplygon.ISimplygon, path: str):
    # Create scene importer 
    sgSceneImporter = sg.CreateSceneImporter()
    sgSceneImporter.SetImportFilePath(path)
    
    # Run scene importer. 
    importResult = sgSceneImporter.Run()
    if Simplygon.Failed(importResult):
        raise Exception('Failed to load scene.')
    sgScene = sgSceneImporter.GetScene()
    return sgScene

def SaveScene(sg: Simplygon.ISimplygon, sgScene: Simplygon.spScene, path: str):
    # Create scene exporter. 
    sgSceneExporter = sg.CreateSceneExporter()
    outputScenePath = ''.join(['output\\', 'AttributeTessellation', '_', path])
    sgSceneExporter.SetExportFilePath(outputScenePath)
    sgSceneExporter.SetScene(sgScene)
    
    # Run scene exporter. 
    exportResult = sgSceneExporter.Run()
    if Simplygon.Failed(exportResult):
        raise Exception('Failed to save scene.')

def CheckLog(sg: Simplygon.ISimplygon):
    # Check if any errors occurred. 
    hasErrors = sg.ErrorOccurred()
    if hasErrors:
        errors = sg.CreateStringArray()
        sg.GetErrorMessages(errors)
        errorCount = errors.GetItemCount()
        if errorCount > 0:
            print('Errors:')
            for errorIndex in range(errorCount):
                errorString = errors.GetItem(errorIndex)
                print(errorString)
            sg.ClearErrorMessages()
    else:
        print('No errors.')
    
    # Check if any warnings occurred. 
    hasWarnings = sg.WarningOccurred()
    if hasWarnings:
        warnings = sg.CreateStringArray()
        sg.GetWarningMessages(warnings)
        warningCount = warnings.GetItemCount()
        if warningCount > 0:
            print('Warnings:')
            for warningIndex in range(warningCount):
                warningString = warnings.GetItem(warningIndex)
                print(warningString)
            sg.ClearWarningMessages()
    else:
        print('No warnings.')

def RunRemeshingWithTessellatedAttributes(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/SimplygonMan/SimplygonMan.obj')
    
    # Create the remeshing pipeline. 
    sgRemeshingPipeline = sg.CreateRemeshingPipeline()
    
    # Fetch all the needed settings objects for the processing, including the attribute 
    # tessellation settings, which we will use to set up the attribute tessellation on the processed 
    # mesh. 
    sgRemeshingSettings = sgRemeshingPipeline.GetRemeshingSettings()
    sgAttributeTessellationSettings = sgRemeshingPipeline.GetAttributeTessellationSettings()
    sgMappingImageSettings = sgRemeshingPipeline.GetMappingImageSettings()
    
    # Set on-screen size target for remeshing. 
    sgRemeshingSettings.SetOnScreenSize( 500 )
    sgRemeshingSettings.SetGeometricalAccuracy( 2.0 )
    
    # Get the attribute tessellation settings. The displacement data will be cast into a 
    # tessellated displacement attribute. In this example we use relative area as the density 
    # setting, which means that triangles are tessellated based on the size of the triangle, so that 
    # the tessellated attributes roughly take up the same area. The value is normalized and scale 
    # independent, so the total area of all the subvalues will add up to the normalized value 1. We set 
    # the maximum area per value to 1/1000000, which means that there will be at least 1000000 values 
    # total in the scene, unless we cap the total number of values with MaxTotalValuesCount or 
    # MaxTessellationLevel. 
    sgAttributeTessellationSettings.SetEnableAttributeTessellation( True )
    sgAttributeTessellationSettings.SetAttributeTessellationDensityMode( Simplygon.EAttributeTessellationDensityMode_RelativeArea )
    sgAttributeTessellationSettings.SetMaxAreaOfTessellatedValue( 0.000001 )
    sgAttributeTessellationSettings.SetOnlyAllowOneLevelOfDifference( True )
    sgAttributeTessellationSettings.SetMinTessellationLevel( 0 )
    sgAttributeTessellationSettings.SetMaxTessellationLevel( 5 )
    sgAttributeTessellationSettings.SetMaxTotalValuesCount( 1000000 )
    
    # Set up the process to generate a mapping image which will be used after the reduction to cast new 
    # materials to the new reduced object, and also to cast the displacement data from the original 
    # object into the tessellated attributes of the processed mesh. 
    sgMappingImageSettings.SetGenerateMappingImage( True )
    sgMappingImageSettings.SetGenerateTexCoords( True )
    sgMappingImageSettings.SetApplyNewMaterialIds( True )
    sgMappingImageSettings.SetGenerateTangents( True )
    sgMappingImageSettings.SetUseFullRetexturing( True )
    sgMappingImageSettings.SetTexCoordGeneratorType( Simplygon.ETexcoordGeneratorType_ChartAggregator )
    sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0)
    
    # Set the size of the mapping image in the output material. This will be the output size of the 
    # textures when we do the material casting in the pipeline. 
    sgOutputMaterialSettings.SetTextureWidth( 2048 )
    sgOutputMaterialSettings.SetTextureHeight( 2048 )
    sgOutputMaterialSettings.SetMultisamplingLevel( 2 )
    
    # Add a diffuse texture caster to the pipeline. This will cast the diffuse color (aka base 
    # color/albedo) in the original scene into a texture map in the output scene. 
    sgDiffuseCaster = sg.CreateColorCaster()
    sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings()
    sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" )
    sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )

    sgRemeshingPipeline.AddMaterialCaster( sgDiffuseCaster, 0 )
    
    # Add a normals texture caster to the pipeline. This will cast the normals in the original scene 
    # into a normal map in the output scene. 
    sgNormalsCaster = sg.CreateNormalCaster()
    sgNormalsCasterSettings = sgNormalsCaster.GetNormalCasterSettings()
    sgNormalsCasterSettings.SetMaterialChannel( "Normals" )
    sgNormalsCasterSettings.SetGenerateTangentSpaceNormals( True )
    sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )

    sgRemeshingPipeline.AddMaterialCaster( sgNormalsCaster, 0 )
    
    # Add a displacement caster to the pipeline. This will cast the displacement values, but instead 
    # of casting to a texture, it will cast into the tessellated attributes. 
    sgDisplacementCaster = sg.CreateDisplacementCaster()
    sgDisplacementCaster.SetScene( sgScene )
    sgDisplacementCasterSettings = sgDisplacementCaster.GetDisplacementCasterSettings()
    sgDisplacementCasterSettings.SetMaterialChannel( "Displacement" )
    sgDisplacementCasterSettings.SetDilation( 10 )
    sgDisplacementCasterSettings.SetOutputToTessellatedAttributes( True )
    sgDisplacementCasterSettings.GetAttributeTessellationSamplingSettings().SetSourceMaterialId( 0 )
    sgDisplacementCasterSettings.GetAttributeTessellationSamplingSettings().SetAttributeFormat( Simplygon.EAttributeFormat_U16 )
    sgDisplacementCasterSettings.GetAttributeTessellationSamplingSettings().SetSupersamplingCount( 16 )
    sgDisplacementCasterSettings.GetAttributeTessellationSamplingSettings().SetBlendOperation( Simplygon.EBlendOperation_Mean )

    sgRemeshingPipeline.AddMaterialCaster( sgDisplacementCaster, 0 )
    
    # Start the remeshing pipeline.     
    print("Start the remeshing pipeline.")
    sgRemeshingPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode_RunInThisProcess)
    
    # Save processed scene.     
    print("Save processed scene.")
    SaveScene(sg, sgScene, 'RemeshedOutput.gltf')
    
    # Create an attribute tessellation tool object. 
    sgAttributeTessellation = sg.CreateAttributeTessellation()
    
    # Generate a tessellated copy of the scene.     
    print("Generate a tessellated copy of the scene.")
    sgTessellatedScene = sgAttributeTessellation.NewTessellatedScene(sgScene)
    
    # Save the tessellated copy of the scene.     
    print("Save the tessellated copy of the scene.")
    SaveScene(sg, sgTessellatedScene, 'RemeshedTessellatedOutput.obj')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        RunRemeshingWithTessellatedAttributes(sg)

        sg = None
        gc.collect()

