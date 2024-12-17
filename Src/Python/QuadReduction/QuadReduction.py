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
    outputScenePath = ''.join(['output\\', 'QuadReduction', '_', path])
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

def RunReduction(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/QuadFace/SgFace.obj')
    
    # Create the reduction processor. 
    sgQuadReductionProcessor = sg.CreateQuadReductionProcessor()
    sgQuadReductionProcessor.SetScene( sgScene )
    sgQuadReductionSettings = sgQuadReductionProcessor.GetQuadReductionSettings()
    
    # Set OnScreenSize target. OnScreenSize and MaxDev are recommended.  
    # Note that if you use one of the polygon-count derived targets, both true triangles and 
    # implicitly linked quads are included. 
    # Since the quad reducer only attempts to remove quads, if you have an asset with a lot of 
    # triangles, that will affect the polyvcount and ratio and the reduction might not do what you 
    # expect. 
    sgQuadReductionSettings.SetReductionTargets( Simplygon.EStopCondition_All, False, False, False, True )
    sgQuadReductionSettings.SetReductionTargetOnScreenSize( 100 )
    
    # Start the reduction process.     
    print("Start the reduction process.")
    sgQuadReductionProcessor.RunProcessing()
    
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

