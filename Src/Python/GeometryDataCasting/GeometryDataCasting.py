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

def RunGeometryDataDasting(sg: Simplygon.ISimplygon):
    # Load scene to process. 
    sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj")
    
    # Create the remeshing processor. 
    sgRemeshingProcessor = sg.CreateRemeshingProcessor()
    sgRemeshingProcessor.SetScene( sgScene )
    sgRemeshingSettings = sgRemeshingProcessor.GetRemeshingSettings()
    sgMappingImageSettings = sgRemeshingProcessor.GetMappingImageSettings()
    
    # Set on-screen size target for remeshing. 
    sgRemeshingSettings.SetOnScreenSize( 300 )
    
    # Generates a mapping image which is used after the remeshing to cast new materials to the new 
    # remeshed object. 
    sgMappingImageSettings.SetGenerateMappingImage( True )
    sgMappingImageSettings.SetApplyNewMaterialIds( True )
    sgMappingImageSettings.SetGenerateTangents( True )
    sgMappingImageSettings.SetUseFullRetexturing( True )
    sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0)
    
    # Setting the size of the output material for the mapping image. This will be the output size of the 
    # textures when we do material casting in a later stage. 
    sgOutputMaterialSettings.SetTextureWidth( 2048 )
    sgOutputMaterialSettings.SetTextureHeight( 2048 )
    
    # Start the remeshing process.     
    print("Start the remeshing process.")
    sgRemeshingProcessor.RunProcessing()
    
    # Setup and run the geometry data caster casting Coords to a texture.     
    print("Setup and run the geometry data caster casting Coords to a texture.")
    sgGeometryData_CoordsCaster = sg.CreateGeometryDataCaster()
    sgGeometryData_CoordsCaster.SetMappingImage( sgRemeshingProcessor.GetMappingImage() )
    sgGeometryData_CoordsCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgGeometryData_CoordsCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgGeometryData_CoordsCaster.SetOutputFilePath( 'GeometryData_CoordsTexture' )

    sgGeometryData_CoordsCasterSettings = sgGeometryData_CoordsCaster.GetGeometryDataCasterSettings()
    sgGeometryData_CoordsCasterSettings.SetMaterialChannel( 'GeometryData_Coords' )
    sgGeometryData_CoordsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgGeometryData_CoordsCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat_R16G16B16 )
    sgGeometryData_CoordsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_NoFill )
    sgGeometryData_CoordsCasterSettings.SetGeometryDataFieldType( Simplygon.EGeometryDataFieldType_Coords )
    sgGeometryData_CoordsCasterSettings.SetGeometryDataFieldIndex( 0 )

    sgGeometryData_CoordsCaster.RunProcessing()
    geometrydata_coordsTextureFilePath = sgGeometryData_CoordsCaster.GetOutputFilePath()
    
    # Setup and run the geometry data caster casting Normals to a texture.     
    print("Setup and run the geometry data caster casting Normals to a texture.")
    sgGeometryData_NormalsCaster = sg.CreateGeometryDataCaster()
    sgGeometryData_NormalsCaster.SetMappingImage( sgRemeshingProcessor.GetMappingImage() )
    sgGeometryData_NormalsCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgGeometryData_NormalsCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgGeometryData_NormalsCaster.SetOutputFilePath( 'GeometryData_NormalsTexture' )

    sgGeometryData_NormalsCasterSettings = sgGeometryData_NormalsCaster.GetGeometryDataCasterSettings()
    sgGeometryData_NormalsCasterSettings.SetMaterialChannel( 'GeometryData_Normals' )
    sgGeometryData_NormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgGeometryData_NormalsCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat_R16G16B16 )
    sgGeometryData_NormalsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_NoFill )
    sgGeometryData_NormalsCasterSettings.SetGeometryDataFieldType( Simplygon.EGeometryDataFieldType_Normals )
    sgGeometryData_NormalsCasterSettings.SetGeometryDataFieldIndex( 0 )

    sgGeometryData_NormalsCaster.RunProcessing()
    geometrydata_normalsTextureFilePath = sgGeometryData_NormalsCaster.GetOutputFilePath()
    
    # Setup and run the geometry data caster casting MaterialIds to a texture.     
    print("Setup and run the geometry data caster casting MaterialIds to a texture.")
    sgGeometryData_MaterialIdsCaster = sg.CreateGeometryDataCaster()
    sgGeometryData_MaterialIdsCaster.SetMappingImage( sgRemeshingProcessor.GetMappingImage() )
    sgGeometryData_MaterialIdsCaster.SetSourceMaterials( sgScene.GetMaterialTable() )
    sgGeometryData_MaterialIdsCaster.SetSourceTextures( sgScene.GetTextureTable() )
    sgGeometryData_MaterialIdsCaster.SetOutputFilePath( 'GeometryData_MaterialIdsTexture' )

    sgGeometryData_MaterialIdsCasterSettings = sgGeometryData_MaterialIdsCaster.GetGeometryDataCasterSettings()
    sgGeometryData_MaterialIdsCasterSettings.SetMaterialChannel( 'GeometryData_MaterialIds' )
    sgGeometryData_MaterialIdsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat_PNG )
    sgGeometryData_MaterialIdsCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat_R8 )
    sgGeometryData_MaterialIdsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode_NoFill )
    sgGeometryData_MaterialIdsCasterSettings.SetGeometryDataFieldType( Simplygon.EGeometryDataFieldType_MaterialIds )
    sgGeometryData_MaterialIdsCasterSettings.SetGeometryDataFieldIndex( 0 )

    sgGeometryData_MaterialIdsCaster.RunProcessing()
    geometrydata_materialidsTextureFilePath = sgGeometryData_MaterialIdsCaster.GetOutputFilePath()
    
    # Update scene with new casted textures. 
    sgMaterialTable = sg.CreateMaterialTable()
    sgTextureTable = sg.CreateTextureTable()
    sgMaterial = sg.CreateMaterial()
    sgGeometryData_CoordsTexture = sg.CreateTexture()
    sgGeometryData_CoordsTexture.SetName( 'GeometryData_Coords' )
    sgGeometryData_CoordsTexture.SetFilePath( geometrydata_coordsTextureFilePath )
    sgTextureTable.AddTexture( sgGeometryData_CoordsTexture )

    sgGeometryData_CoordsTextureShadingNode = sg.CreateShadingTextureNode()
    sgGeometryData_CoordsTextureShadingNode.SetTexCoordLevel( 0 )
    sgGeometryData_CoordsTextureShadingNode.SetTextureName( 'GeometryData_Coords' )

    sgMaterial.AddMaterialChannel( 'GeometryData_Coords' )
    sgMaterial.SetShadingNetwork( 'GeometryData_Coords', sgGeometryData_CoordsTextureShadingNode )
    sgGeometryData_NormalsTexture = sg.CreateTexture()
    sgGeometryData_NormalsTexture.SetName( 'GeometryData_Normals' )
    sgGeometryData_NormalsTexture.SetFilePath( geometrydata_normalsTextureFilePath )
    sgTextureTable.AddTexture( sgGeometryData_NormalsTexture )

    sgGeometryData_NormalsTextureShadingNode = sg.CreateShadingTextureNode()
    sgGeometryData_NormalsTextureShadingNode.SetTexCoordLevel( 0 )
    sgGeometryData_NormalsTextureShadingNode.SetTextureName( 'GeometryData_Normals' )

    sgMaterial.AddMaterialChannel( 'GeometryData_Normals' )
    sgMaterial.SetShadingNetwork( 'GeometryData_Normals', sgGeometryData_NormalsTextureShadingNode )
    sgGeometryData_MaterialIdsTexture = sg.CreateTexture()
    sgGeometryData_MaterialIdsTexture.SetName( 'GeometryData_MaterialIds' )
    sgGeometryData_MaterialIdsTexture.SetFilePath( geometrydata_materialidsTextureFilePath )
    sgTextureTable.AddTexture( sgGeometryData_MaterialIdsTexture )

    sgGeometryData_MaterialIdsTextureShadingNode = sg.CreateShadingTextureNode()
    sgGeometryData_MaterialIdsTextureShadingNode.SetTexCoordLevel( 0 )
    sgGeometryData_MaterialIdsTextureShadingNode.SetTextureName( 'GeometryData_MaterialIds' )

    sgMaterial.AddMaterialChannel( 'GeometryData_MaterialIds' )
    sgMaterial.SetShadingNetwork( 'GeometryData_MaterialIds', sgGeometryData_MaterialIdsTextureShadingNode )

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

    RunGeometryDataDasting(sg)

    sg = None
    gc.collect()

