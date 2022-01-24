# Copyright (c) Microsoft Corporation. 
# Licensed under the MIT License. 

import math
import os
import sys
import glob
import gc
import threading

from pathlib import Path
from simplygon import simplygon_loader
from simplygon import Simplygon


def LoadScene(sg: Simplygon.ISimplygon, path: str):
    # Create scene importer 
    sgSceneImporter = sg.CreateSceneImporter()
    sgSceneImporter.SetImportFilePath(path)
    
    # Run scene importer. 
    importResult = sgSceneImporter.RunImport()
    if not importResult:
        raise Exception('Failed to load scene.')
    sgScene = sgSceneImporter.GetScene()
    return sgScene

def SaveScene(sg: Simplygon.ISimplygon, sgScene:Simplygon.spScene, path: str):
    # Create scene exporter. 
    sgSceneExporter = sg.CreateSceneExporter()
    sgSceneExporter.SetExportFilePath(path)
    sgSceneExporter.SetScene(sgScene)
    
    # Run scene exporter. 
    exportResult = sgSceneExporter.RunExport()
    if not exportResult:
        raise Exception('Failed to save scene.')

def CheckLog(sg: Simplygon.ISimplygon):
    # Check if any errors occurred. 
    hasErrors = sg.ErrorOccurred()
    if hasErrors:
        errors = sg.CreateStringArray()
        sg.GetErrorMessages(errors)
        errorCount = errors.GetItemCount()
        if errorCount > 0:
            print("Errors:")
            for errorIndex in range(errorCount):
                errorString = errors.GetItem(errorIndex)
                print(errorString)
            sg.ClearErrorMessages()
    else:
        print("No errors.")
    
    # Check if any warnings occurred. 
    hasWarnings = sg.WarningOccurred()
    if hasWarnings:
        warnings = sg.CreateStringArray()
        sg.GetWarningMessages(warnings)
        warningCount = warnings.GetItemCount()
        if warningCount > 0:
            print("Warnings:")
            for warningIndex in range(warningCount):
                warningString = warnings.GetItem(warningIndex)
                print(warningString)
            sg.ClearWarningMessages()
    else:
        print("No warnings.")

def RunRemeshingWithMaterialCasting(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj")
    
    # Create the remeshing pipeline. 
    sgRemeshingPipeline = sg.CreateRemeshingPipeline()
    sgRemeshingSettings = sgRemeshingPipeline.GetRemeshingSettings()
    sgMappingImageSettings = sgRemeshingPipeline.GetMappingImageSettings()
    
    # Set on-screen size target for remeshing. 
    sgRemeshingSettings.SetOnScreenSize( 300 )
    
    # Generates a mapping image which is used after the remeshing to cast new materials to the new 
    # remeshed object. 
    sgMappingImageSettings.SetGenerateMappingImage( True )
    sgMappingImageSettings.SetApplyNewMaterialIds( True )
    sgMappingImageSettings.SetGenerateTangents( True )
    sgMappingImageSettings.SetUseFullRetexturing( True )
    sgMappingImageSettings.SetTexCoordGeneratorType( Simplygon.ETexcoordGeneratorType_ChartAggregator )
    sgChartAggregatorSettings = sgMappingImageSettings.GetChartAggregatorSettings()
    
    # Enable the chart aggregator and reuse UV space. 
    sgChartAggregatorSettings.SetChartAggregatorMode( Simplygon.EChartAggregatorMode_SurfaceArea )
    sgChartAggregatorSettings.SetSeparateOverlappingCharts( False )
    sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0)
    
    # Setting the size of the output material for the mapping image. This will be the output size of the 
    # textures when we do material casting in a later stage. 
    sgOutputMaterialSettings.SetTextureWidth( 2048 )
    sgOutputMaterialSettings.SetTextureHeight( 2048 )
    
    # Add diffuse material caster to pipeline.     
    print("Add diffuse material caster to pipeline.")
    sgDiffuseCaster = sg.CreateColorCaster()
    sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings()
    sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" )
    sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )

    sgRemeshingPipeline.AddMaterialCaster( sgDiffuseCaster, 0 )
    
    # Add normals material caster to pipeline.     
    print("Add normals material caster to pipeline.")
    sgNormalsCaster = sg.CreateNormalCaster()
    sgNormalsCasterSettings = sgNormalsCaster.GetNormalCasterSettings()
    sgNormalsCasterSettings.SetMaterialChannel( "Normals" )
    sgNormalsCasterSettings.SetGenerateTangentSpaceNormals( True )
    sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )

    sgRemeshingPipeline.AddMaterialCaster( sgNormalsCaster, 0 )
    
    # Start the remeshing pipeline.     
    print("Start the remeshing pipeline.")
    sgRemeshingPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode_RunInThisProcess)
    
    # Get the processed scene. 
    sgProcessedScene = sgRemeshingPipeline.GetProcessedScene()
    
    # Save processed scene.     
    print("Save processed scene.")
    SaveScene(sg, sgProcessedScene, "Output.fbx")
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
    sg = simplygon_loader.init_simplygon()
    if sg is None:
        exit(Simplygon.GetLastInitializationError())

    RunRemeshingWithMaterialCasting(sg)

    sg = None
    gc.collect()

