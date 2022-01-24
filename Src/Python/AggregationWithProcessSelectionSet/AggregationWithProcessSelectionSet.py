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

def RunAggregation(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/ObscuredTeapot.obj")
    
    # Create the aggregation processor. 
    sgAggregationProcessor = sg.CreateAggregationProcessor()
    sgAggregationProcessor.SetScene( sgScene )
    sgAggregationSettings = sgAggregationProcessor.GetAggregationSettings()
    
    # Add a selection set to the scene with all nodes which should be aggregated. 
    sgSceneSelectionSetTable = sgScene.GetSelectionSetTable()
    sgAggregationTargetSelectionSet = sg.CreateSelectionSet()
    sgAggregationTargetSelectionSet.SetName("AggregationTarget")
    sgRootTeapot001 = sgScene.GetNodeFromPath('Root/Teapot001')
    if sgRootTeapot001 is not None:
        sgAggregationTargetSelectionSet.AddItem(sgRootTeapot001.GetNodeGUID())
    sgSceneSelectionSetTable.AddSelectionSet(sgAggregationTargetSelectionSet)
    
    # Merge all geometries into a single geometry. 
    sgAggregationSettings.SetMergeGeometries( True )
    
    # Use the selection set created earlier. 
    sgAggregationSettings.SetProcessSelectionSetName( "AggregationTarget" )
    
    # Start the aggregation process.     
    print("Start the aggregation process.")
    sgAggregationProcessor.RunProcessing()
    
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

    RunAggregation(sg)

    sg = None
    gc.collect()

