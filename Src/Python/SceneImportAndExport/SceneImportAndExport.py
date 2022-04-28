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

def ImportExport(sg: Simplygon.ISimplygon):
    # Load obj scene.     
    print("Load obj scene.")
    sgSceneObj = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj")
    
    # Save obj scene.     
    print("Save obj scene.")
    SaveScene(sg, sgSceneObj, "Output.obj")
    
    # Load fbx scene.     
    print("Load fbx scene.")
    sgSceneFbx = LoadScene(sg, "../../../Assets/RiggedSimplygonMan/RiggedSimplygonMan.fbx")
    
    # Save fbx scene.     
    print("Save fbx scene.")
    SaveScene(sg, sgSceneFbx, "Output.fbx")
    
    # Load glb scene.     
    print("Load glb scene.")
    sgSceneGlb = LoadScene(sg, "../../../Assets/RiggedSimplygonMan/RiggedSimplygonMan.glb")
    
    # Save glb scene.     
    print("Save glb scene.")
    SaveScene(sg, sgSceneGlb, "Output.glb")
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
    sg = simplygon_loader.init_simplygon()
    if sg is None:
        exit(Simplygon.GetLastInitializationError())

    ImportExport(sg)

    sg = None
    gc.collect()

