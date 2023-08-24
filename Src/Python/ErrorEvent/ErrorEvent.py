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

class CustomErrorHandler(Simplygon.ErrorHandler):
    def HandleError(self, object: Simplygon.spObject, interfaceName: str, methodName: str, errorType: int, errorText: str):
        print('Error (%s:%s): %s' %(interfaceName, methodName, errorText))

customErrorHandler = CustomErrorHandler()

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
    # Set the custom error handler to the Simplygon interface. 
    sg.SetErrorHandler(customErrorHandler)
    
    # Create the reduction pipeline. 
    sgReductionPipeline = sg.CreateReductionPipeline()
    
    # To be able to use the T-Junction remover, the welder has to be enabled so this will trigger an 
    # error. 
    sgRepairSettings = sgReductionPipeline.GetRepairSettings()
    sgRepairSettings.SetUseWelding( False )
    sgRepairSettings.SetUseTJunctionRemover( True )
    
    # Start the reduction pipeline and the faulty settings will cause an error.     
    print("Start the reduction pipeline and the faulty settings will cause an error.")
    sgReductionPipeline.RunSceneFromFile('../../../Assets/SimplygonMan/SimplygonMan.obj', 'output.fbx', Simplygon.EPipelineRunMode_RunInNewProcess)
    
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

