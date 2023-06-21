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
    outputScenePath = ''.join(['output\\', 'ImpostorPipelineBillboardCloudFoliage', '_', path])
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
            print('CheckLog: Errors:')
            for errorIndex in range(errorCount):
                errorString = errors.GetItem(errorIndex)
                print(errorString)
            sg.ClearErrorMessages()
    else:
        print('CheckLog: No errors.')
    
    # Check if any warnings occurred. 
    hasWarnings = sg.WarningOccurred()
    if hasWarnings:
        warnings = sg.CreateStringArray()
        sg.GetWarningMessages(warnings)
        warningCount = warnings.GetItemCount()
        if warningCount > 0:
            print('CheckLog: Warnings:')
            for warningIndex in range(warningCount):
                warningString = warnings.GetItem(warningIndex)
                print(warningString)
            sg.ClearWarningMessages()
    else:
        print('CheckLog: No warnings.')
    
    # Error out if Simplygon has errors. 
    if hasErrors:
        raise Exception('Processing failed with an error')

def RunBillboardCloudVegetationPipeline(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/Bush/Bush.fbx')
    
    # For all materials in the scene set the blend mode to blend (instead of opaque) 
    materialCount = sgScene.GetMaterialTable().GetMaterialsCount()
    for i in range(0, materialCount):
        sgScene.GetMaterialTable().GetMaterial(i).SetBlendMode(Simplygon.EMaterialBlendMode_Blend)
    
    # Create the Impostor processor. 
    sgBillboardCloudVegetationPipeline = sg.CreateBillboardCloudVegetationPipeline()
    sgBillboardCloudSettings = sgBillboardCloudVegetationPipeline.GetBillboardCloudSettings()
    sgMappingImageSettings = sgBillboardCloudVegetationPipeline.GetMappingImageSettings()
    
    # Set billboard cloud mode to Foliage. 
    sgBillboardCloudSettings.SetBillboardMode( Simplygon.EBillboardMode_Foliage )
    sgBillboardCloudSettings.SetBillboardDensity( 0.5 )
    sgBillboardCloudSettings.SetGeometricComplexity( 0.9 )
    sgBillboardCloudSettings.SetMaxPlaneCount( 10 )
    sgBillboardCloudSettings.SetTwoSided( True )
    sgFoliageSettings = sgBillboardCloudSettings.GetFoliageSettings()
    
    # Set the parameters for separating foliage and trunk. 
    sgFoliageSettings.SetSeparateTrunkAndFoliage( True )
    sgFoliageSettings.SetSeparateFoliageTriangleRatio( 0.5 )
    sgFoliageSettings.SetSeparateFoliageTriangleThreshold( 10 )
    sgFoliageSettings.SetSeparateFoliageAreaThreshold( 0.1 )
    sgFoliageSettings.SetSeparateFoliageSizeThreshold( 0.1 )
    sgFoliageSettings.SetTrunkReductionRatio( 0.5 )
    sgMappingImageSettings.SetMaximumLayers( 10 )
    sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0)
    
    # Setting the size of the output material for the mapping image. This will be the output size of the 
    # textures when we do material casting in a later stage. 
    sgOutputMaterialSettings.SetTextureWidth( 1024 )
    sgOutputMaterialSettings.SetTextureHeight( 1024 )
    sgOutputMaterialSettings.SetMultisamplingLevel( 2 )
    
    # Add diffuse material caster to pipeline.     
    print("Add diffuse material caster to pipeline.")
    sgDiffuseCaster = sg.CreateColorCaster()
    sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings()
    sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" )
    sgDiffuseCasterSettings.SetOpacityChannel( 'Opacity' )
    sgDiffuseCasterSettings.SetOpacityChannelComponent( Simplygon.EColorComponent_Alpha )
    sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgDiffuseCasterSettings.SetBakeOpacityInAlpha( False )
    sgDiffuseCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat_R8G8B8 )
    sgDiffuseCasterSettings.SetDilation( 10 )
    sgDiffuseCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_Interpolate )

    sgBillboardCloudVegetationPipeline.AddMaterialCaster( sgDiffuseCaster, 0 )
    
    # Add specular material caster to pipeline.     
    print("Add specular material caster to pipeline.")
    sgSpecularCaster = sg.CreateColorCaster()
    sgSpecularCasterSettings = sgSpecularCaster.GetColorCasterSettings()
    sgSpecularCasterSettings.SetMaterialChannel( "Specular" )
    sgSpecularCasterSettings.SetOpacityChannel( 'Opacity' )
    sgSpecularCasterSettings.SetOpacityChannelComponent( Simplygon.EColorComponent_Alpha )
    sgSpecularCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgSpecularCasterSettings.SetDilation( 10 )
    sgSpecularCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_Interpolate )

    sgBillboardCloudVegetationPipeline.AddMaterialCaster( sgSpecularCaster, 0 )
    
    # Add normals material caster to pipeline.     
    print("Add normals material caster to pipeline.")
    sgNormalsCaster = sg.CreateNormalCaster()
    sgNormalsCasterSettings = sgNormalsCaster.GetNormalCasterSettings()
    sgNormalsCasterSettings.SetMaterialChannel( "Normals" )
    sgNormalsCasterSettings.SetOpacityChannel( 'Opacity' )
    sgNormalsCasterSettings.SetOpacityChannelComponent( Simplygon.EColorComponent_Alpha )
    sgNormalsCasterSettings.SetGenerateTangentSpaceNormals( True )
    sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgNormalsCasterSettings.SetDilation( 10 )
    sgNormalsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_Interpolate )

    sgBillboardCloudVegetationPipeline.AddMaterialCaster( sgNormalsCaster, 0 )
    
    # Add opacity material casting to pipeline. Make sure there is no dilation or fill.     
    print("Add opacity material casting to pipeline. Make sure there is no dilation or fill.")
    sgOpacityCaster = sg.CreateOpacityCaster()
    sgOpacityCasterSettings = sgOpacityCaster.GetOpacityCasterSettings()
    sgOpacityCasterSettings.SetMaterialChannel( "Opacity" )
    sgOpacityCasterSettings.SetOpacityChannel( 'Opacity' )
    sgOpacityCasterSettings.SetOpacityChannelComponent( Simplygon.EColorComponent_Alpha )
    sgOpacityCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgOpacityCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_NoFill )
    sgOpacityCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat_R8 )

    sgBillboardCloudVegetationPipeline.AddMaterialCaster( sgOpacityCaster, 0 )
    
    # Start the impostor pipeline.     
    print("Start the impostor pipeline.")
    sgBillboardCloudVegetationPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode_RunInThisProcess)
    
    # Get the processed scene. 
    sgProcessedScene = sgBillboardCloudVegetationPipeline.GetProcessedScene()
    
    # Save processed scene.     
    print("Save processed scene.")
    SaveScene(sg, sgProcessedScene, 'Output.glb')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        RunBillboardCloudVegetationPipeline(sg)

        sg = None
        gc.collect()

