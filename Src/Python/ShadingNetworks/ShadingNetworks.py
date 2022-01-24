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

def RunReductionWithShadingNetworks(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj")
    sgReductionProcessor = sg.CreateReductionProcessor()
    sgReductionProcessor.SetScene( sgScene )
    sgReductionSettings = sgReductionProcessor.GetReductionSettings()
    sgMappingImageSettings = sgReductionProcessor.GetMappingImageSettings()
    
    # Generates a mapping image which is used after the reduction to cast new materials to the new 
    # reduced object. 
    sgMappingImageSettings.SetGenerateMappingImage( True )
    sgMappingImageSettings.SetApplyNewMaterialIds( True )
    sgMappingImageSettings.SetGenerateTangents( True )
    sgMappingImageSettings.SetUseFullRetexturing( True )
    
    # Inject a sepia filter into the shading network for the diffuse channel for each material in the 
    # scene. 
    materialCount = sgScene.GetMaterialTable().GetMaterialsCount()
    for i in range(0, materialCount):
        sgSepiaMaterial = sgScene.GetMaterialTable().GetMaterial(i)

        sgMaterialShadingNode = sgSepiaMaterial.GetShadingNetwork("Diffuse")
        sgSepiaColor1 = sg.CreateShadingColorNode()
        sgSepiaColor2 = sg.CreateShadingColorNode()
        sgSepiaColor3 = sg.CreateShadingColorNode()
        sgRedFilter = sg.CreateShadingColorNode()
        sgGreenFilter = sg.CreateShadingColorNode()
        sgBlueFilter = sg.CreateShadingColorNode()
        sgSepiaDot1 = sg.CreateShadingDot3Node()
        sgSepiaDot2 = sg.CreateShadingDot3Node()
        sgSepiaDot3 = sg.CreateShadingDot3Node()
        sgSepiaMul1 = sg.CreateShadingMultiplyNode()
        sgSepiaMul2 = sg.CreateShadingMultiplyNode()
        sgSepiaMul3 = sg.CreateShadingMultiplyNode()
        sgSepiaAdd1 = sg.CreateShadingAddNode()
        sgSepiaAdd2 = sg.CreateShadingAddNode()

        sgSepiaColor1.SetColor(0.393, 0.769, 0.189, 1.0)
        sgSepiaColor2.SetColor(0.349, 0.686, 0.168, 1.0)
        sgSepiaColor3.SetColor(0.272, 0.534, 0.131, 1.0)
        sgRedFilter.SetColor(1.0, 0.0, 0.0, 1.0)
        sgGreenFilter.SetColor(0.0, 1.0, 0.0, 1.0)
        sgBlueFilter.SetColor(0.0, 0.0, 1.0, 1.0)

        sgSepiaDot1.SetInput(0, sgSepiaColor1)
        sgSepiaDot1.SetInput(1, sgMaterialShadingNode)
        sgSepiaDot2.SetInput(0, sgSepiaColor2)
        sgSepiaDot2.SetInput(1, sgMaterialShadingNode)
        sgSepiaDot3.SetInput(0, sgSepiaColor3)
        sgSepiaDot3.SetInput(1, sgMaterialShadingNode)
        sgSepiaMul1.SetInput(0, sgSepiaDot1)
        sgSepiaMul1.SetInput(1, sgRedFilter)
        sgSepiaMul2.SetInput(0, sgSepiaDot2)
        sgSepiaMul2.SetInput(1, sgGreenFilter)
        sgSepiaMul3.SetInput(0, sgSepiaDot3)
        sgSepiaMul3.SetInput(1, sgBlueFilter)
        sgSepiaAdd1.SetInput(0, sgSepiaMul1)
        sgSepiaAdd1.SetInput(1, sgSepiaMul2)
        sgSepiaAdd2.SetInput(0, sgSepiaAdd1)
        sgSepiaAdd2.SetInput(1, sgSepiaMul3)

        sgSepiaMaterial.SetShadingNetwork('Diffuse', sgSepiaAdd2)
    
    # Start the reduction process.     
    print("Start the reduction process.")
    sgReductionProcessor.RunProcessing()
    
    # Setup and run the diffuse material casting.     
    print("Setup and run the diffuse material casting.")
    sgDiffuseCaster = sg.CreateColorCaster()
    sgDiffuseCaster.SetMappingImage( sgReductionProcessor.GetMappingImage() )
    sgDiffuseCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgDiffuseCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgDiffuseCaster.SetOutputFilePath( 'DiffuseTexture' )

    sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings()
    sgDiffuseCasterSettings.SetMaterialChannel( 'Diffuse' )
    sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )

    sgDiffuseCaster.RunProcessing()
    diffuseTextureFilePath = sgDiffuseCaster.GetOutputFilePath()
    
    # Update scene with new casted texture. 
    sgMaterialTable = sg.CreateMaterialTable()
    sgTextureTable = sg.CreateTextureTable()
    sgMaterial = sg.CreateMaterial()
    sgDiffuseTexture = sg.CreateTexture()
    sgDiffuseTexture.SetName( 'Diffuse' )
    sgDiffuseTexture.SetFilePath( diffuseTextureFilePath )
    sgTextureTable.AddTexture( sgDiffuseTexture )

    sgDiffuseTextureShadingNode = sg.CreateShadingTextureNode()
    sgDiffuseTextureShadingNode.SetTexCoordLevel( 0 )
    sgDiffuseTextureShadingNode.SetTextureName( 'Diffuse' )

    sgMaterial.AddMaterialChannel( 'Diffuse' )
    sgMaterial.SetShadingNetwork( 'Diffuse', sgDiffuseTextureShadingNode )

    sgMaterialTable.AddMaterial( sgMaterial )

    sgScene.GetTextureTable().Clear()
    sgScene.GetMaterialTable().Clear()
    sgScene.GetTextureTable().Copy(sgTextureTable)
    sgScene.GetMaterialTable().Copy(sgMaterialTable)
    
    # Save processed scene.     
    print("Save processed scene.")
    SaveScene(sg, sgScene, "Output.fbx")
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
    sg = simplygon_loader.init_simplygon()
    if sg is None:
        exit(Simplygon.GetLastInitializationError())

    RunReductionWithShadingNetworks(sg)

    sg = None
    gc.collect()

