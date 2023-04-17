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


def SaveScene(sg: Simplygon.ISimplygon, sgScene: Simplygon.spScene, path: str):
    # Create scene exporter. 
    sgSceneExporter = sg.CreateSceneExporter()
    outputScenePath = ''.join(['output\\', 'SceneData', '_', path])
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

def CreateCube(sg: Simplygon.ISimplygon, materialId: int):
    vertexCount = 8
    triangleCount = 12
    cornerCount = triangleCount * 3
    cornerIds = [ 0, 1, 4, 4, 1, 5, 5, 1, 6, 1, 2, 6, 6, 2, 3, 6, 3, 7, 7, 3, 0, 7, 0, 4, 0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7 ]
    vertexCoordinates = [ 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0 ]
    
    # Create the Geometry. All geometry data will be loaded into this object. 
    sgGeometryData = sg.CreateGeometryData()
    
    # Set vertex- and triangle-counts for the Geometry. 
    # NOTE: The number of vertices and triangles has to be set before vertex- and triangle-data is 
    # loaded into the GeometryData. 
    sgGeometryData.SetVertexCount(vertexCount)
    sgGeometryData.SetTriangleCount(triangleCount)
    
    # Array with vertex-coordinates. Will contain 3 real-values for each vertex in the geometry. 
    sgCoords = sgGeometryData.GetCoords()
    
    # Array with triangle-data. Will contain 3 ids for each corner of each triangle, so the triangles 
    # know what vertices to use. 
    sgVertexIds = sgGeometryData.GetVertexIds()
    
    # Add material data. Materials are assigned per triangle. 
    sgGeometryData.AddMaterialIds()
    sgMaterialIds = sgGeometryData.GetMaterialIds()
    
    # Add vertex-coordinates array to the Geometry. 
    sgCoords.SetData(vertexCoordinates, vertexCount * 3)
    
    # Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
    # uses. 
    sgVertexIds.SetData(cornerIds, cornerCount)
    
    # Loop through all the triangles an assign material. 
    for triangleIndex in range(triangleCount):
        sgMaterialIds.SetItem(triangleIndex, materialId)
    
    # Return created cube geometry data. 
    return sgGeometryData

def RunExample(sg: Simplygon.ISimplygon):
    # Create a Simplygon scene. 
    sgScene = sg.CreateScene()
    
    # Get material table from the scene. 
    sgMaterialTable = sgScene.GetMaterialTable()
    
    # Create a red diffuse material and a red specular material. 
    sgRedColorNode = sg.CreateShadingColorNode()
    sgRedColorNode.SetColor(0.5, 0.0, 0.0, 0.0)
    sgDiffuseRedMaterial = sg.CreateMaterial()
    sgDiffuseRedMaterial.SetName('red_diffuse')
    sgDiffuseRedMaterial.AddMaterialChannel(Simplygon.SG_MATERIAL_CHANNEL_DIFFUSE)
    sgDiffuseRedMaterial.SetShadingNetwork(Simplygon.SG_MATERIAL_CHANNEL_DIFFUSE, sgRedColorNode)
    sgSpecularRedMaterial = sg.CreateMaterial()
    sgSpecularRedMaterial.SetName('red_specular')
    sgSpecularRedMaterial.AddMaterialChannel(Simplygon.SG_MATERIAL_CHANNEL_SPECULAR)
    sgSpecularRedMaterial.SetShadingNetwork(Simplygon.SG_MATERIAL_CHANNEL_SPECULAR, sgRedColorNode)
    
    # Add the materials to the material table. 
    diffuseMaterialId = sgMaterialTable.AddMaterial(sgDiffuseRedMaterial)
    specularMaterialId = sgMaterialTable.AddMaterial(sgSpecularRedMaterial)
    
    # Create two scene mesh objects. 
    sgCubeMesh1 = sg.CreateSceneMesh()
    sgCubeMesh2 = sg.CreateSceneMesh()
    
    # Set name on the scene meshes. 
    sgCubeMesh1.SetName('Cube1')
    sgCubeMesh1.SetOriginalName('Cube1')
    sgCubeMesh2.SetName('Cube2')
    sgCubeMesh2.SetOriginalName('Cube2')
    
    # Create cube geometry. 
    sgGeometryDataCube1 = CreateCube(sg, diffuseMaterialId)
    sgGeometryDataCube2 = CreateCube(sg, specularMaterialId)
    sgCubeMesh1.SetGeometry(sgGeometryDataCube1)
    sgCubeMesh2.SetGeometry(sgGeometryDataCube2)
    
    # Add the two scene meshes as child to the root node of the scene. 
    sgScene.GetRootNode().AddChild(sgCubeMesh1)

    sgScene.GetRootNode().AddChild(sgCubeMesh2)

    
    # Create a transform node. 
    sgTransform = sg.CreateTransform3()
    
    # Set the transform to use premultiply. 
    sgTransform.PreMultiply()
    
    # Add 45 degree rotation and 5 units translation. 
    sgTransform.AddRotation(0.785, 0.0, 1.0, 0.0)
    sgTransform.AddRotation(0.785, 1.0, 0.0, 0.0)
    sgTransform.AddTranslation(0.0, 5.0, 0.0)
    
    # Apply transformation on the second cube node. 
    sgTransformMatrix = sgTransform.GetMatrix()
    sgCubeMesh2.GetRelativeTransform().DeepCopy(sgTransformMatrix)

    
    # Save example scene to output.obj.     
    print("Save example scene to output.obj.")
    SaveScene(sg, sgScene, 'Output.obj')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        RunExample(sg)

        sg = None
        gc.collect()

