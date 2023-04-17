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

def RunReduction(sg: Simplygon.ISimplygon, inputFile, outputFile):
    # Create the reduction pipeline. 
    sgReductionPipeline = sg.CreateReductionPipeline()
    sgReductionSettings = sgReductionPipeline.GetReductionSettings()
    
    # Set reduction target to triangle ratio with a ratio of 50%. 
    sgReductionSettings.SetReductionTargets( Simplygon.EStopCondition_All, True, False, False, False )
    sgReductionSettings.SetReductionTargetTriangleRatio( 0.5 )
    
    # Start the reduction pipeline.     
    print("Start the reduction pipeline.")
    sgReductionPipeline.RunSceneFromFile(inputFile, outputFile, Simplygon.EPipelineRunMode_RunDistributedUsingSimplygonGrid)
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        jobs = list()
        inputFiles = glob.glob('../../../Assets/' + '/**/*.glb', recursive=True)
        for inputFile in inputFiles:
            outputFile = os.path.join('output', 'BatchingWithSimplygonGrid_' + Path(inputFile).stem + '_LOD' + Path(inputFile).suffix)
            job = threading.Thread(target=RunReduction, args=(sg, inputFile, outputFile))
            jobs.append(job)
            job.start()

        for job in jobs:
            job.join()

        sg = None
        gc.collect()

