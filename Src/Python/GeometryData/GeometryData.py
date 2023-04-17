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
    outputScenePath = ''.join(['output\\', 'GeometryData', '_', path])
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

def RunExample1(sg: Simplygon.ISimplygon):
    # 4 separate triangles, with 3 vertices each and 3 sets of UV coordinates each. They make up 2 
    # quads, where each quad has the same set of UV coordinates. 
    vertexCount = 12
    triangleCount = 4
    cornerCount = triangleCount * 3
    
    # 4 triangles x 3 indices ( or 3 corners ). 
    cornerIds = [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 ]
    
    # 12 vertices with values for the x, y and z coordinates. 
    vertexCoordinates = [ 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 2.0, 0.0, 0.0, 2.0, 1.0, 0.0, 2.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0 ]
    
    # UV coordinates for all 12 corners. 
    textureCoordinates = [ 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0 ]
    
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
    
    # Must add texture channel before adding data to it. 
    sgGeometryData.AddTexCoords(0)
    sgTexcoords = sgGeometryData.GetTexCoords(0)
    
    # Add vertex-coordinates array to the Geometry. 
    sgCoords.SetData(vertexCoordinates, vertexCount * 3)
    
    # Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
    # uses. 
    sgVertexIds.SetData(cornerIds, cornerCount)
    
    # Add texture-coordinates array to the Geometry. 
    sgTexcoords.SetData(textureCoordinates, cornerCount * 2)
    
    # Create a scene and a SceneMesh node with the geometry. 
    sgScene = sg.CreateScene()
    sgSceneMesh = sg.CreateSceneMesh()
    sgSceneMesh.SetName('Mesh1')
    sgSceneMesh.SetGeometry(sgGeometryData)
    sgScene.GetRootNode().AddChild(sgSceneMesh)

    
    # Save example1 scene to Example1.obj.     
    print("Save example1 scene to Example1.obj.")
    SaveScene(sg, sgScene, 'Example1.obj')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

def RunExample2(sg: Simplygon.ISimplygon):
    # Same as RunExample1, but now the vertices are shared among the triangles. 
    vertexCount = 6
    triangleCount = 4
    cornerCount = triangleCount * 3
    
    # 4 triangles x 3 indices ( or 3 corners ). 
    cornerIds = [ 0, 1, 2, 0, 2, 3, 1, 4, 5, 1, 5, 2 ]
    
    # 6 vertices with values for the x, y and z coordinates. 
    vertexCoordinates = [ 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 0.0, 2.0, 1.0, 0.0 ]
    
    # UV coordinates for all 12 corners. 
    textureCoordinates = [ 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0 ]
    
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
    
    # Must add texture channel before adding data to it. 
    sgGeometryData.AddTexCoords(0)
    sgTexcoords = sgGeometryData.GetTexCoords(0)
    
    # Add vertex-coordinates array to the Geometry. 
    sgCoords.SetData(vertexCoordinates, vertexCount * 3)
    
    # Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
    # uses. 
    sgVertexIds.SetData(cornerIds, cornerCount)
    
    # Add texture-coordinates array to the Geometry. 
    sgTexcoords.SetData(textureCoordinates, cornerCount * 2)
    
    # Create a scene and a SceneMesh node with the geometry. 
    sgScene = sg.CreateScene()
    sgSceneMesh = sg.CreateSceneMesh()
    sgSceneMesh.SetName('Mesh2')
    sgSceneMesh.SetGeometry(sgGeometryData)
    sgScene.GetRootNode().AddChild(sgSceneMesh)

    
    # Save example2 scene to Example2.obj.     
    print("Save example2 scene to Example2.obj.")
    SaveScene(sg, sgScene, 'Example2.obj')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

def RunExample3(sg: Simplygon.ISimplygon):
    # Same as RunExample1, but now all corner-data is stored as vertex-data, in a packet format. 
    # Since the 2 vertices where the quads meet don't share same UV, they will be 2 separate vertices, 
    # so 4 vertices / quad as opposed to 6 / quad in RunExample1, and only 6 for whole mesh in 
    # RunExample2. 
    vertexCount = 8
    triangleCount = 4
    cornerCount = triangleCount * 3
    
    # 4 triangles x 3 indices ( or 3 corners ). 
    cornerIds = [ 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7 ]
    
    # 8 vertices with values for the x, y and z coordinates. 
    vertexCoordinates = [ 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 2.0, 0.0, 0.0, 2.0, 1.0, 0.0, 1.0, 1.0, 0.0 ]
    
    # UV coordinates for all 8 vertices. 
    textureCoordinates = [ 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0 ]
    
    # Create the PackedGeometry. All geometry data will be loaded into this object. 
    sgPackedGeometryData = sg.CreatePackedGeometryData()
    
    # Set vertex- and triangle-counts for the Geometry. 
    # NOTE: The number of vertices and triangles has to be set before vertex- and triangle-data is 
    # loaded into the GeometryData. 
    sgPackedGeometryData.SetVertexCount(vertexCount)
    sgPackedGeometryData.SetTriangleCount(triangleCount)
    
    # Array with vertex-coordinates. Will contain 3 real-values for each vertex in the geometry. 
    sgCoords = sgPackedGeometryData.GetCoords()
    
    # Array with triangle-data. Will contain 3 ids for each corner of each triangle, so the triangles 
    # know what vertices to use. 
    sgVertexIds = sgPackedGeometryData.GetVertexIds()
    
    # Must add texture channel before adding data to it. 
    sgPackedGeometryData.AddTexCoords(0)
    sgTexcoords = sgPackedGeometryData.GetTexCoords(0)
    
    # Add vertex-coordinates array to the Geometry. 
    sgCoords.SetData(vertexCoordinates, vertexCount * 3)
    
    # Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
    # uses. 
    sgVertexIds.SetData(cornerIds, cornerCount)
    
    # Add texture-coordinates array to the Geometry. 
    sgTexcoords.SetData(textureCoordinates, cornerCount * 2)
    
    # Create a scene and a SceneMesh node with the geometry. 
    sgScene = sg.CreateScene()
    sgSceneMesh = sg.CreateSceneMesh()
    sgSceneMesh.SetName('Mesh3')
    sgGeometryData = sgPackedGeometryData.NewUnpackedCopy()
    sgSceneMesh.SetGeometry(sgGeometryData)
    sgScene.GetRootNode().AddChild(sgSceneMesh)

    
    # Save example3 scene to Example3.obj.     
    print("Save example3 scene to Example3.obj.")
    SaveScene(sg, sgScene, 'Example3.obj')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        RunExample1(sg)
        RunExample2(sg)
        RunExample3(sg)

        sg = None
        gc.collect()

