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
    outputScenePath = ''.join(['output\\', 'ReductionWithVisibilityCameraFromGeometry', '_', path])
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

def RunReduction(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/ObscuredTeapot/ObscuredTeapot.obj')
    
    # Load camera gemometry 
    sgSceneGeometryCamera = LoadScene(sg, '../../../Assets/ObscuredTeapot/CameraMesh.obj')
    
    # Select Mesh Nodes 
    selectionSetId = sgSceneGeometryCamera.SelectNodes('ISceneMesh')
    sgSelectionSetsTable = sgSceneGeometryCamera.GetSelectionSetTable()
    selectionSceneMeshes = sgSelectionSetsTable.GetSelectionSet(selectionSetId)
    itemCount = selectionSceneMeshes.GetItemCount()
    cameraSelectionSet = sg.CreateSelectionSet()
    
    # Copy each mesh from camera scene into a scene and created a camera selection set based on those 
    # ids. 
    for meshIndex in range(itemCount):
        meshNodeId = selectionSceneMeshes.GetItem(meshIndex)
        sceneNode = sgSceneGeometryCamera.GetNodeByGUID(meshNodeId)
        sceneMesh = Simplygon.spSceneMesh.SafeCast(sceneNode)
        geom = sceneMesh.GetGeometry()
        cameraMesh = sgScene.GetRootNode().CreateChildMesh(geom)

        nodeId = cameraMesh.GetNodeGUID()
        cameraSelectionSet.AddItem(nodeId)
    cameraSelectionSetId = sgScene.GetSelectionSetTable().AddSelectionSet(cameraSelectionSet)

    
    # Create the reduction processor. 
    sgReductionProcessor = sg.CreateReductionProcessor()
    
    # Get settings objects 
    sgReductionSettings = sgReductionProcessor.GetReductionSettings()
    sgVisibilitySettings = sgReductionProcessor.GetVisibilitySettings()
    
    # Set camera selection set id with 
    sgVisibilitySettings.SetCameraSelectionSetID( cameraSelectionSetId )
    
    # Setup visibility setting enable GPU based computation, 
    sgVisibilitySettings.SetUseVisibilityWeightsInReducer( True )
    sgVisibilitySettings.SetUseVisibilityWeightsInTexcoordGenerator( False )
    sgVisibilitySettings.SetComputeVisibilityMode( Simplygon.EComputeVisibilityMode_DirectX )
    sgVisibilitySettings.SetConservativeMode( False )
    sgVisibilitySettings.SetCullOccludedGeometry( True )
    sgVisibilitySettings.SetFillNonVisibleAreaThreshold( 0.0 )
    sgVisibilitySettings.SetRemoveTrianglesNotOccludingOtherTriangles( False )
    sgVisibilitySettings.SetUseBackfaceCulling( True )
    
    # Set reduction target to triangle ratio with a ratio of 50%. 
    sgReductionSettings.SetReductionTargetTriangleRatio( 0.5 )
    sgReductionProcessor.SetScene( sgScene )
    
    # Start the reduction process.     
    print("Start the reduction process.")
    sgReductionProcessor.RunProcessing()
    
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

        RunReduction(sg)

        sg = None
        gc.collect()

