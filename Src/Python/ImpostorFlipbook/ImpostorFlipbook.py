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
    outputScenePath = ''.join(['output\\', 'ImpostorFlipbook', '_', path])
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

def RunFlipbook(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/Bush/Bush.fbx')
    
    # For all materials in the scene set the blend mode to blend (instead of opaque) 
    materialCount = sgScene.GetMaterialTable().GetMaterialsCount()
    for i in range(0, materialCount):
        sgScene.GetMaterialTable().GetMaterial(i).SetBlendMode(Simplygon.EMaterialBlendMode_Blend)
    
    # For all materials in the scene set the opacity mode to Opacity. 
    materialCount = sgScene.GetMaterialTable().GetMaterialsCount()
    for i in range(0, materialCount):
        sgScene.GetMaterialTable().GetMaterial(i).SetOpacityType(Simplygon.EOpacityType_Opacity)
    
    # Create the Impostor processor. 
    sgImpostorProcessor = sg.CreateImpostorProcessor()
    sgImpostorProcessor.SetScene( sgScene )
    sgImpostorSettings = sgImpostorProcessor.GetImpostorSettings()
    
    # Set impostor type to From single view. 
    sgImpostorSettings.SetImpostorType( Simplygon.EImpostorType_Flipbook )
    sgFlipbookSettings = sgImpostorSettings.GetFlipbookSettings()
    sgFlipbookSettings.SetNumberOfViews( 9 )
    sgFlipbookSettings.SetViewDirectionX( 1.0 )
    sgFlipbookSettings.SetViewDirectionY( 0.0 )
    sgFlipbookSettings.SetViewDirectionZ( 0.0 )
    sgFlipbookSettings.SetUpVectorX( 0.0 )
    sgFlipbookSettings.SetUpVectorY( 1.0 )
    sgFlipbookSettings.SetUpVectorZ( 0.0 )
    sgMappingImageSettings = sgImpostorProcessor.GetMappingImageSettings()
    sgMappingImageSettings.SetMaximumLayers( 10 )
    sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0)
    
    # Setting the size of the output material for the mapping image. This will be the output size of the 
    # textures when we do material casting in a later stage. 
    sgOutputMaterialSettings.SetTextureWidth( 256 )
    sgOutputMaterialSettings.SetTextureHeight( 256 )
    sgOutputMaterialSettings.SetMultisamplingLevel( 2 )
    
    # Start the impostor process.     
    print("Start the impostor process.")
    sgImpostorProcessor.RunProcessing()
    
    # Setup and run the diffuse material casting.     
    print("Setup and run the diffuse material casting.")
    sgDiffuseCaster = sg.CreateColorCaster()
    sgDiffuseCaster.SetMappingImage( sgImpostorProcessor.GetMappingImage() )
    sgDiffuseCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgDiffuseCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgDiffuseCaster.SetOutputFilePath( 'DiffuseTexture' )

    sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings()
    sgDiffuseCasterSettings.SetMaterialChannel( 'Diffuse' )
    sgDiffuseCasterSettings.SetOpacityChannel( 'Opacity' )
    sgDiffuseCasterSettings.SetOpacityChannelComponent( Simplygon.EColorComponent_Alpha )
    sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgDiffuseCasterSettings.SetBakeOpacityInAlpha( False )
    sgDiffuseCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat_R8G8B8 )
    sgDiffuseCasterSettings.SetDilation( 10 )
    sgDiffuseCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_Interpolate )

    sgDiffuseCaster.RunProcessing()
    diffuseTextureFilePath = sgDiffuseCaster.GetOutputFilePath()
    
    # Setup and run the specular material casting.     
    print("Setup and run the specular material casting.")
    sgSpecularCaster = sg.CreateColorCaster()
    sgSpecularCaster.SetMappingImage( sgImpostorProcessor.GetMappingImage() )
    sgSpecularCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgSpecularCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgSpecularCaster.SetOutputFilePath( 'SpecularTexture' )

    sgSpecularCasterSettings = sgSpecularCaster.GetColorCasterSettings()
    sgSpecularCasterSettings.SetMaterialChannel( 'Specular' )
    sgSpecularCasterSettings.SetOpacityChannel( 'Opacity' )
    sgSpecularCasterSettings.SetOpacityChannelComponent( Simplygon.EColorComponent_Alpha )
    sgSpecularCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgSpecularCasterSettings.SetDilation( 10 )
    sgSpecularCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_Interpolate )

    sgSpecularCaster.RunProcessing()
    specularTextureFilePath = sgSpecularCaster.GetOutputFilePath()
    
    # Setup and run the normals material casting.     
    print("Setup and run the normals material casting.")
    sgNormalsCaster = sg.CreateNormalCaster()
    sgNormalsCaster.SetMappingImage( sgImpostorProcessor.GetMappingImage() )
    sgNormalsCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgNormalsCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgNormalsCaster.SetOutputFilePath( 'NormalsTexture' )

    sgNormalsCasterSettings = sgNormalsCaster.GetNormalCasterSettings()
    sgNormalsCasterSettings.SetMaterialChannel( 'Normals' )
    sgNormalsCasterSettings.SetOpacityChannel( 'Opacity' )
    sgNormalsCasterSettings.SetOpacityChannelComponent( Simplygon.EColorComponent_Alpha )
    sgNormalsCasterSettings.SetGenerateTangentSpaceNormals( True )
    sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgNormalsCasterSettings.SetDilation( 10 )
    sgNormalsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_Interpolate )

    sgNormalsCaster.RunProcessing()
    normalsTextureFilePath = sgNormalsCaster.GetOutputFilePath()
    
    # Setup and run the opacity material casting. Make sure there is no dilation or fill.     
    print("Setup and run the opacity material casting. Make sure there is no dilation or fill.")
    sgOpacityCaster = sg.CreateOpacityCaster()
    sgOpacityCaster.SetMappingImage( sgImpostorProcessor.GetMappingImage() )
    sgOpacityCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgOpacityCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgOpacityCaster.SetOutputFilePath( 'OpacityTexture' )

    sgOpacityCasterSettings = sgOpacityCaster.GetOpacityCasterSettings()
    sgOpacityCasterSettings.SetMaterialChannel( 'Opacity' )
    sgOpacityCasterSettings.SetOpacityChannel( 'Opacity' )
    sgOpacityCasterSettings.SetOpacityChannelComponent( Simplygon.EColorComponent_Alpha )
    sgOpacityCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgOpacityCasterSettings.SetDilation( 0 )
    sgOpacityCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_NoFill )
    sgOpacityCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat_R8 )

    sgOpacityCaster.RunProcessing()
    opacityTextureFilePath = sgOpacityCaster.GetOutputFilePath()
    
    # Update scene with new casted textures. 
    sgMaterialTable = sg.CreateMaterialTable()
    sgTextureTable = sg.CreateTextureTable()
    sgMaterial = sg.CreateMaterial()
    sgMaterial.SetName("OutputMaterial")
    sgDiffuseTexture = sg.CreateTexture()
    sgDiffuseTexture.SetName( 'Diffuse' )
    sgDiffuseTexture.SetFilePath( diffuseTextureFilePath )
    sgTextureTable.AddTexture( sgDiffuseTexture )

    sgDiffuseTextureShadingNode = sg.CreateShadingTextureNode()
    sgDiffuseTextureShadingNode.SetTexCoordLevel( 0 )
    sgDiffuseTextureShadingNode.SetTextureName( 'Diffuse' )

    sgMaterial.AddMaterialChannel( 'Diffuse' )
    sgMaterial.SetShadingNetwork( 'Diffuse', sgDiffuseTextureShadingNode )
    sgSpecularTexture = sg.CreateTexture()
    sgSpecularTexture.SetName( 'Specular' )
    sgSpecularTexture.SetFilePath( specularTextureFilePath )
    sgTextureTable.AddTexture( sgSpecularTexture )

    sgSpecularTextureShadingNode = sg.CreateShadingTextureNode()
    sgSpecularTextureShadingNode.SetTexCoordLevel( 0 )
    sgSpecularTextureShadingNode.SetTextureName( 'Specular' )

    sgMaterial.AddMaterialChannel( 'Specular' )
    sgMaterial.SetShadingNetwork( 'Specular', sgSpecularTextureShadingNode )
    sgNormalsTexture = sg.CreateTexture()
    sgNormalsTexture.SetName( 'Normals' )
    sgNormalsTexture.SetFilePath( normalsTextureFilePath )
    sgTextureTable.AddTexture( sgNormalsTexture )

    sgNormalsTextureShadingNode = sg.CreateShadingTextureNode()
    sgNormalsTextureShadingNode.SetTexCoordLevel( 0 )
    sgNormalsTextureShadingNode.SetTextureName( 'Normals' )

    sgMaterial.AddMaterialChannel( 'Normals' )
    sgMaterial.SetShadingNetwork( 'Normals', sgNormalsTextureShadingNode )
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

        RunFlipbook(sg)

        sg = None
        gc.collect()

