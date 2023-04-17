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
    outputScenePath = ''.join(['output\\', 'OpacityCasting', '_', path])
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
    
    # Error out if Simplygon has errors. 
    if hasErrors:
        raise Exception('Processing failed with an error')

def OpacityCasting(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/Console/Console.obj')
    
    # Create the reduction processor. 
    sgReductionProcessor = sg.CreateReductionProcessor()
    sgReductionProcessor.SetScene( sgScene )
    sgReductionSettings = sgReductionProcessor.GetReductionSettings()
    sgMappingImageSettings = sgReductionProcessor.GetMappingImageSettings()
    
    # Set reduction target to triangle ratio with a ratio of 50%. 
    sgReductionSettings.SetReductionTargets( Simplygon.EStopCondition_All, True, False, False, False )
    sgReductionSettings.SetReductionTargetTriangleRatio( 0.5 )
    
    # Generates a mapping image which is used after the reduction to cast new materials to the new 
    # reduced object. 
    sgMappingImageSettings.SetGenerateMappingImage( True )
    sgMappingImageSettings.SetApplyNewMaterialIds( True )
    sgMappingImageSettings.SetGenerateTangents( True )
    sgMappingImageSettings.SetUseFullRetexturing( True )
    sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0)
    
    # Setting the size of the output material for the mapping image. This will be the output size of the 
    # textures when we do material casting in a later stage. 
    sgOutputMaterialSettings.SetTextureWidth( 2048 )
    sgOutputMaterialSettings.SetTextureHeight( 2048 )
    
    # Start the reduction process.     
    print("Start the reduction process.")
    sgReductionProcessor.RunProcessing()
    
    # Setup and run the opacity material casting. 
    sgOpacityCaster = sg.CreateOpacityCaster()
    sgOpacityCaster.SetMappingImage( sgReductionProcessor.GetMappingImage() )
    sgOpacityCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgOpacityCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgOpacityCaster.SetOutputFilePath( 'OpacityTexture' )

    sgOpacityCasterSettings = sgOpacityCaster.GetOpacityCasterSettings()
    sgOpacityCasterSettings.SetMaterialChannel( 'Opacity' )
    sgOpacityCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgOpacityCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat_R8 )

    sgOpacityCaster.RunProcessing()
    opacityTextureFilePath = sgOpacityCaster.GetOutputFilePath()
    
    # Update scene with new casted texture. 
    sgMaterialTable = sg.CreateMaterialTable()
    sgTextureTable = sg.CreateTextureTable()
    sgMaterial = sg.CreateMaterial()
    sgOpacityTexture = sg.CreateTexture()
    sgOpacityTexture.SetName( 'Opacity' )
    sgOpacityTexture.SetFilePath( opacityTextureFilePath )
    sgTextureTable.AddTexture( sgOpacityTexture )

    sgOpacityTextureShadingNode = sg.CreateShadingTextureNode()
    sgOpacityTextureShadingNode.SetTexCoordLevel( 0 )
    sgOpacityTextureShadingNode.SetTextureName( 'Opacity' )

    sgMaterial.AddMaterialChannel( 'Opacity' )
    sgMaterial.SetShadingNetwork( 'Opacity', sgOpacityTextureShadingNode )
    sgMaterial.SetBlendMode(Simplygon.EMaterialBlendMode_Blend);

    sgMaterialTable.AddMaterial( sgMaterial )

    sgScene.GetTextureTable().Clear()
    sgScene.GetMaterialTable().Clear()
    sgScene.GetTextureTable().Copy(sgTextureTable)
    sgScene.GetMaterialTable().Copy(sgMaterialTable)
    
    # Save processed scene.     
    print("Save processed scene.")
    SaveScene(sg, sgScene, 'Output.glb')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        OpacityCasting(sg)

        sg = None
        gc.collect()

