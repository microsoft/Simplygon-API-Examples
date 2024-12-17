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
    outputScenePath = ''.join(['output\\', 'AggregationPipelineWithVisibilityOccluder', '_', path])
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

def RunAggregation(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/ObscuredTeapot/ObscuredTeapot.obj')
    
    # Create the aggregation pipeline. 
    sgAggregationPipeline = sg.CreateAggregationPipeline()
    sgAggregationSettings = sgAggregationPipeline.GetAggregationSettings()
    sgVisibilitySettings = sgAggregationPipeline.GetVisibilitySettings()
    
    # Merge all geometries into a single geometry. 
    sgAggregationSettings.SetMergeGeometries( True )
    
    # Add a selection set to the scene. We'll use this later as a occluder. 
    sgSceneSelectionSetTable = sgScene.GetSelectionSetTable()
    sgOccluderSelectionSet = sg.CreateSelectionSet()
    sgOccluderSelectionSet.SetName("Occluder")
    sgRootBox002 = sgScene.GetNodeFromPath('Root/Box002')
    if sgRootBox002 is not None:
        sgOccluderSelectionSet.AddItem(sgRootBox002.GetNodeGUID())
    sgSceneSelectionSetTable.AddSelectionSet(sgOccluderSelectionSet)
    
    # Use the occluder previously added. 
    sgVisibilitySettings.SetOccluderSelectionSetName( 'Occluder' )
    
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
    
    # Start the aggregation pipeline.     
    print("Start the aggregation pipeline.")
    sgAggregationPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode_RunInThisProcess)
    
    # Get the processed scene. 
    sgProcessedScene = sgAggregationPipeline.GetProcessedScene()
    
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

        RunAggregation(sg)

        sg = None
        gc.collect()

