# Copyright (c) Microsoft Corporation. 
# Licensed under the MIT License. 

import math
import os
import sys
import glob
import gc
import threading

from pathlib import Path
from simplygon9 import simplygon_loader
from simplygon9 import Simplygon

class CustomObserver(Simplygon.Observer):
    def OnShadingCustomNodeEvaluate(self, subject: Simplygon.spObject):
        outputValue = Simplygon.ShadingColor( 0.0, 0.0, 0.0, 1.0 )
        if not subject.IsNull():
            customNode = Simplygon.spShadingCustomNode.SafeCast(subject)
            if not customNode.IsNull():
                inputValue = customNode.GetInputValue(0)
                outputValue.r = inputValue.r * 0.393 + inputValue.g * 0.769 + inputValue.b * 0.189
                outputValue.g = inputValue.r * 0.349 + inputValue.g * 0.686 + inputValue.b * 0.168
                outputValue.b = inputValue.r * 0.272 + inputValue.g * 0.534 + inputValue.b * 0.131
        return outputValue
    def OnShadingCustomNodeGenerateShaderCode(self, subject: Simplygon.spObject, shaderLanguage):
        if not subject.IsNull():
            customNode = Simplygon.spShadingCustomNode.SafeCast(subject)
            if not customNode.IsNull():
                customNode.SetCustomShaderCode( 'result = float4(dot(rgba_custom_input_0, float3(0.393f, 0.769f, 0.189f)), dot(rgba_custom_input_0, float3(0.349f, 0.686f, 0.168f)), dot(rgba_custom_input_0, float3(0.272f, 0.534f, 0.131f)), 1.0f);')
                return True
        return False

customObserver = CustomObserver()

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
    
    # Inject a sepia filter as a custom shading node into the shading network for the diffuse channel 
    # for each material in the scene. 
    materialCount = sgScene.GetMaterialTable().GetMaterialsCount()
    for i in range(0, materialCount):
        sgSepiaMaterial = sgScene.GetMaterialTable().GetMaterial(i)

        sgMaterialShadingNode = sgSepiaMaterial.GetShadingNetwork("Diffuse")
        sgSepiaNode = sg.CreateShadingCustomNode()

        # Add the custom observer to our custom shading node.
        sgSepiaNode.AddObserver(customObserver)
        
        # Set the number of input slots to the custom node. In this case we only use the diffuse color from the loaded material as input.
        sgSepiaNode.SetInputCount(1)


        sgSepiaNode.SetInput(0, sgMaterialShadingNode)

        sgSepiaMaterial.SetShadingNetwork('Diffuse', sgSepiaNode)
    
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

