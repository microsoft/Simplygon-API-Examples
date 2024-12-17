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
    outputScenePath = ''.join(['output\\', 'RemeshingWithDisplacementTessellation', '_', path])
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

def RunRemeshingWithDisplacementTessellation(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/SimplygonMan/SimplygonMan.obj')
    
    # Create the remeshing processor. 
    sgRemeshingProcessor = sg.CreateRemeshingProcessor()
    sgRemeshingProcessor.SetScene( sgScene )
    
    # Fetch all the needed settings objects for the processing, including the attribute 
    # tessellation settings, which we will set up to receive displacement data in the processed 
    # mesh. 
    sgRemeshingSettings = sgRemeshingProcessor.GetRemeshingSettings()
    sgAttributeTessellationSettings = sgRemeshingProcessor.GetAttributeTessellationSettings()
    
    # Set on-screen size target for remeshing, and tell the remeshing processor to cast 
    # displacement data into the attribute tessellation field of the processed geometry. Note: The 
    # tessellation settings are defined in the section below. 
    sgRemeshingSettings.SetOnScreenSize( 1000 )
    sgRemeshingSettings.SetPopulateAttributeTessellationDisplacement( True )
    sgRemeshingSettings.SetSurfaceTransferMode( Simplygon.ESurfaceTransferMode_Fast )
    
    # Set the tessellation settings. The displacement data will be cast into a tessellated 
    # displacement attribute. In this example we use OnScreenSize as the density setting, which 
    # means that triangles are tessellated based on the size of the rendered object, so that a 
    # triangle is when tessellated roughly the size of a pixel. We also add some additional 
    # constraints, such as only allowing base triangles to tessellate to level 5 (1024 
    # sub-triangles), only allow one level of difference between neighbor base-triangles, and the 
    # total number of sub-triangles should not exceed 1000000. 
    sgAttributeTessellationSettings.SetEnableAttributeTessellation( True )
    sgAttributeTessellationSettings.SetAttributeTessellationDensityMode( Simplygon.EAttributeTessellationDensityMode_OnScreenSize )
    sgAttributeTessellationSettings.SetOnScreenSize( 1000 )
    sgAttributeTessellationSettings.SetOnlyAllowOneLevelOfDifference( True )
    sgAttributeTessellationSettings.SetMinTessellationLevel( 0 )
    sgAttributeTessellationSettings.SetMaxTessellationLevel( 5 )
    sgAttributeTessellationSettings.SetMaxTotalValuesCount( 1000000 )
    
    # Start the remeshing process.     
    print("Start the remeshing process.")
    sgRemeshingProcessor.RunProcessing()
    
    # Replace original materials and textures from the scene with a new empty material, as the 
    # remeshed object has a new UV set.  
    sgScene.GetTextureTable().Clear()
    sgScene.GetMaterialTable().Clear()
    defaultMaterial = sg.CreateMaterial()
    defaultMaterial.SetName("defaultMaterial")
    sgScene.GetMaterialTable().AddMaterial( defaultMaterial )
    
    # Save processed remeshed scene.     
    print("Save processed remeshed scene.")
    SaveScene(sg, sgScene, 'OutputBase.obj')
    
    # We will now create an attribute tessellation tool object, and generate a scene with the 
    # tessellated attribute displacement data generated into real tessellated mesh data, which is 
    # stored into a new scene. 
    sgAttributeTessellation = sg.CreateAttributeTessellation()
    sgTessellatedScene = sgAttributeTessellation.NewTessellatedScene(sgScene)
    
    # Save the tessellated copy of the scene.     
    print("Save the tessellated copy of the scene.")
    SaveScene(sg, sgTessellatedScene, 'OutputTessellation.obj')
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        RunRemeshingWithDisplacementTessellation(sg)

        sg = None
        gc.collect()

