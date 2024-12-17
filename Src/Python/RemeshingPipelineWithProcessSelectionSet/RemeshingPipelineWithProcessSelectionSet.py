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
    outputScenePath = ''.join(['output\\', 'RemeshingPipelineWithProcessSelectionSet', '_', path])
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

def RunRemeshing(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/ObscuredTeapot/ObscuredTeapot.obj')
    
    # Create the remeshing pipeline. 
    sgRemeshingPipeline = sg.CreateRemeshingPipeline()
    sgRemeshingSettings = sgRemeshingPipeline.GetRemeshingSettings()
    
    # Add a selection set to the scene with all nodes which should be remeshed. 
    sgSceneSelectionSetTable = sgScene.GetSelectionSetTable()
    sgRemeshingTargetSelectionSet = sg.CreateSelectionSet()
    sgRemeshingTargetSelectionSet.SetName("RemeshingTarget")
    sgRootTeapot001 = sgScene.GetNodeFromPath('Root/Teapot001')
    if sgRootTeapot001 is not None:
        sgRemeshingTargetSelectionSet.AddItem(sgRootTeapot001.GetNodeGUID())
    sgSceneSelectionSetTable.AddSelectionSet(sgRemeshingTargetSelectionSet)
    
    # Set on-screen size target for remeshing. 
    sgRemeshingSettings.SetOnScreenSize( 300 )
    
    # Use the selection set created earlier. 
    sgRemeshingSettings.SetProcessSelectionSetName( 'RemeshingTarget' )
    
    # Start the remeshing pipeline.     
    print("Start the remeshing pipeline.")
    sgRemeshingPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode_RunInThisProcess)
    
    # Get the processed scene. 
    sgProcessedScene = sgRemeshingPipeline.GetProcessedScene()
    
    # Since we are not casting any materials in this example, add a default material to silence 
    # validation warnings in the exporter. 
    defaultMaterial = sg.CreateMaterial()
    defaultMaterial.SetName("defaultMaterial")
    sgScene.GetMaterialTable().AddMaterial( defaultMaterial )
    
    # Save processed scene.     
    print("Save processed scene.")
    SaveScene(sg, sgProcessedScene, 'Output.fbx')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        RunRemeshing(sg)

        sg = None
        gc.collect()

