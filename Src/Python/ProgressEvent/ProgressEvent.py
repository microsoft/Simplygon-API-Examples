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

class CustomObserver(Simplygon.Observer):
    def OnProgress(self, subject: Simplygon.spObject, progressPercent: float):
        print("Progress: %f" %(progressPercent))
        # return False to abort the processing
        return True

customObserver = CustomObserver()

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
    # Create the reduction pipeline. 
    sgReductionPipeline = sg.CreateReductionPipeline()
    
    # Add the custom observer to the reduction pipeline. 
    sgReductionPipeline.AddObserver(customObserver)
    outputScenePath = ''.join(['output\\', 'ProgressEvent', '_Output.fbx'])
    
    # Start the reduction pipeline.     
    print("Start the reduction pipeline.")
    sgReductionPipeline.RunSceneFromFile('../../../Assets/SimplygonMan/SimplygonMan.obj', outputScenePath, Simplygon.EPipelineRunMode_RunInNewProcess)
    
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

