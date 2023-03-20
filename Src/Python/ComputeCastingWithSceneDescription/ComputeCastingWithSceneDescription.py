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
    outputScenePath = ''.join(['output\\', 'ComputeCastingWithSceneDescription', '_', path])
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

def ComputeCasting(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/SimplygonMan/SimplygonMan.obj')
    
    # Add tangents to the scene. Tangents are needed to cast the tangent-space normal maps, and the 
    # input scene is missing tangents. 
    sgTangentCalculator = sg.CreateTangentCalculator()
    sgTangentCalculator.CalculateTangentsOnScene(sgScene, True)
    
    # Load scene material evaluation description, which will define all evaluation shaders for all 
    # materials, as well as needed textures and custom buffers.     
    print("Load scene material evaluation description, which will define all evaluation shaders for all materials, as well as needed textures and custom buffers.")
    sgMaterialEvaluationShaderSerializer = sg.CreateMaterialEvaluationShaderSerializer()
    sgMaterialEvaluationShaderSerializer.LoadSceneMaterialEvaluationShadersFromFile('SimplygonManCasting.xml', sgScene)
    
    # Create the aggregation processor. 
    sgAggregationProcessor = sg.CreateAggregationProcessor()
    sgAggregationProcessor.SetScene( sgScene )
    sgAggregationSettings = sgAggregationProcessor.GetAggregationSettings()
    sgMappingImageSettings = sgAggregationProcessor.GetMappingImageSettings()
    
    # Merge all geometries into a single geometry. 
    sgAggregationSettings.SetMergeGeometries( True )
    
    # Generates a mapping image which is used after the aggregation to cast new materials to the new 
    # aggregated object. 
    sgMappingImageSettings.SetGenerateMappingImage( True )
    sgMappingImageSettings.SetApplyNewMaterialIds( True )
    sgMappingImageSettings.SetGenerateTangents( True )
    sgMappingImageSettings.SetUseFullRetexturing( True )
    sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0)
    
    # Setting the size of the output material for the mapping image. This will be the output size of the 
    # textures when we do material casting in a later stage. 
    sgOutputMaterialSettings.SetTextureWidth( 2048 )
    sgOutputMaterialSettings.SetTextureHeight( 2048 )
    
    # Start the aggregation process.     
    print("Start the aggregation process.")
    sgAggregationProcessor.RunProcessing()
    
    # Setup and run the compute caster on the diffuse channel.     
    print("Setup and run the compute caster on the diffuse channel.")
    sgDiffuseCaster = sg.CreateComputeCaster()
    sgDiffuseCaster.SetMappingImage( sgAggregationProcessor.GetMappingImage() )
    sgDiffuseCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgDiffuseCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgDiffuseCaster.SetOutputFilePath( 'DiffuseTexture' )

    sgDiffuseCasterSettings = sgDiffuseCaster.GetComputeCasterSettings()
    sgDiffuseCasterSettings.SetMaterialChannel( 'Diffuse' )
    sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )

    sgDiffuseCaster.RunProcessing()
    diffuseTextureFilePath = sgDiffuseCaster.GetOutputFilePath()
    
    # Setup and run another compute shader on the normals channel.     
    print("Setup and run another compute shader on the normals channel.")
    sgNormalsCaster = sg.CreateComputeCaster()
    sgNormalsCaster.SetMappingImage( sgAggregationProcessor.GetMappingImage() )
    sgNormalsCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgNormalsCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgNormalsCaster.SetOutputFilePath( 'NormalsTexture' )

    sgNormalsCasterSettings = sgNormalsCaster.GetComputeCasterSettings()
    sgNormalsCasterSettings.SetMaterialChannel( 'Normals' )
    sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )

    sgNormalsCaster.RunProcessing()
    normalsTextureFilePath = sgNormalsCaster.GetOutputFilePath()
    
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
    sgNormalsTexture = sg.CreateTexture()
    sgNormalsTexture.SetName( 'Normals' )
    sgNormalsTexture.SetFilePath( normalsTextureFilePath )
    sgTextureTable.AddTexture( sgNormalsTexture )

    sgNormalsTextureShadingNode = sg.CreateShadingTextureNode()
    sgNormalsTextureShadingNode.SetTexCoordLevel( 0 )
    sgNormalsTextureShadingNode.SetTextureName( 'Normals' )

    sgMaterial.AddMaterialChannel( 'Normals' )
    sgMaterial.SetShadingNetwork( 'Normals', sgNormalsTextureShadingNode )

    sgMaterialTable.AddMaterial( sgMaterial )

    sgScene.GetTextureTable().Clear()
    sgScene.GetMaterialTable().Clear()
    sgScene.GetTextureTable().Copy(sgTextureTable)
    sgScene.GetMaterialTable().Copy(sgMaterialTable)
    
    # Save processed scene.     
    print("Save processed scene.")
    SaveScene(sg, sgScene, 'Output.fbx')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        ComputeCasting(sg)

        sg = None
        gc.collect()

