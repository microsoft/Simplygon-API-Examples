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

def RunReduction(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/Teapot.obj")
    
    # Create the reduction pipeline. 
    sgReductionPipeline = sg.CreateReductionPipeline()
    sgReductionSettings = sgReductionPipeline.GetReductionSettings()
    sgVisibilitySettings = sgReductionPipeline.GetVisibilitySettings()
    
    # Set reduction target to triangle ratio with a ratio of 50%. 
    sgReductionSettings.SetReductionTargets( Simplygon.EStopCondition_All, True, False, False, False )
    sgReductionSettings.SetReductionTargetTriangleRatio( 0.5 )
    
    # Add a camera to the scene. We'll use this later as a visibility camera. 
    sgSceneSelectionSetTable = sgScene.GetSelectionSetTable()
    sgCameraSelectionSet = sg.CreateSelectionSet()
    sgCameraSelectionSet.SetName('Camera')
    sgCameraSceneCamera = sg.CreateSceneCamera()
    sgCameraSceneCamera.SetCustomSphereCameraPath(4, 90, 180, 90)
    sgScene.GetRootNode().AddChild(sgCameraSceneCamera)
    sgCameraSelectionSet.AddItem(sgCameraSceneCamera.GetNodeGUID())
    sgSceneSelectionSetTable.AddSelectionSet(sgCameraSelectionSet)
    
    # Use the camera previously added. 
    sgVisibilitySettings.SetCameraSelectionSetName( "Camera" )
    
    # Enabled GPU based visibility calculations. 
    sgVisibilitySettings.SetComputeVisibilityMode( Simplygon.EComputeVisibilityMode_DirectX )
    
    # Disabled conservative mode. 
    sgVisibilitySettings.SetConservativeMode( False )
    
    # Remove all non visible geometry. 
    sgVisibilitySettings.SetCullOccludedGeometry( True )
    
    # Skip filling nonvisible regions. 
    sgVisibilitySettings.SetFillNonVisibleAreaThreshold( 0.0 )
    
    # Don't remove non occluding triangles. 
    sgVisibilitySettings.SetRemoveTrianglesNotOccludingOtherTriangles( False )
    
    # Remove all back facing triangles. 
    sgVisibilitySettings.SetUseBackfaceCulling( True )
    
    # Don't use visibility weights. 
    sgVisibilitySettings.SetUseVisibilityWeightsInReducer( False )
    
    # Start the reduction pipeline.     
    print("Start the reduction pipeline.")
    sgReductionPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode_RunInThisProcess)
    
    # Get the processed scene. 
    sgProcessedScene = sgReductionPipeline.GetProcessedScene()
    
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

    RunReduction(sg)

    sg = None
    gc.collect()
