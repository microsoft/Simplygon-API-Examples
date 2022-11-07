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
    outputScenePath = ''.join(['output\\', 'ReductionWithModularSeams', '_', path])
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

def ExtractGeometriesInScene(sg: Simplygon.ISimplygon, sgModularAssetsScene: Simplygon.spScene):
    # Extract all geometries in the scene into individual geometries 
    sgGeometryDataCollection = sg.CreateGeometryDataCollection()
    id = sgModularAssetsScene.SelectNodes("ISceneMesh")
    set = sgModularAssetsScene.GetSelectionSetTable().GetSelectionSet(id)

    geometryCount = set.GetItemCount()
    for geomIndex in range(geometryCount):
        guid = set.GetItem(geomIndex)
        sgSceneNode = sgModularAssetsScene.GetNodeByGUID(guid)
        sgSceneMesh = Simplygon.spSceneMesh.SafeCast(sgSceneNode)
        geom = sgSceneMesh.GetGeometry()
        sgGeometryDataCollection.AddGeometryData(geom)
    return sgGeometryDataCollection

def DebugModularSeams(sg: Simplygon.ISimplygon, outputDebugInfo, sgModularSeams: Simplygon.spModularSeams):
    if outputDebugInfo:
        # Optional but helpful to be able to see what the analyzer found. 
        # Each unique modular seam can be extracted as a geometry. If the analyzer ran with 
        # IsTranslationIndependent=false then the seam geometry should be exactly located at the same 
        # place as the modular seams in the original scene. 
        # Each modular seam also has a string array with all the names of the geometries that have that 
        # specific modular seam. 
        seamCount = sgModularSeams.GetModularSeamCount()
        for seamIndex in range(seamCount):
            debugGeom = sgModularSeams.NewDebugModularSeamGeometry(seamIndex)
            geometryNames = sgModularSeams.NewModularSeamGeometryStringArray(seamIndex)
            

            debugScene = sg.CreateScene()
            debugScene.GetRootNode().CreateChildMesh(debugGeom)

            fileName = ''.join(['output\\', 'ReductionWithModularSeams_seam_', str(seamIndex), '.obj'])
            

            sgSceneExporter = sg.CreateSceneExporter()
            sgSceneExporter.SetExportFilePath( fileName )
            sgSceneExporter.SetScene( debugScene )
            sgSceneExporter.Run()
            

            vertexCount = debugGeom.GetVertexCount()
            geometryNamesCount = geometryNames.GetItemCount()
            outputText = ''.join(['Seam ', str(seamIndex), ' consists of ', str(vertexCount), ' vertices and is shared among ', str(geometryNamesCount), ' geometries:'])
            print(outputText)
            for geomIndex in range(geometryNamesCount):
                geometryName = geometryNames.GetItem(geomIndex)
                geometryNameOutput = ''.join([' geom ', str(geomIndex), ': ', geometryName])
                print(geometryNameOutput)

def ModifyReductionSettings(sgReductionSettings: Simplygon.spReductionSettings, triangleRatio, maxDeviation):
    sgReductionSettings.SetKeepSymmetry( True )
    sgReductionSettings.SetUseAutomaticSymmetryDetection( True )
    sgReductionSettings.SetUseHighQualityNormalCalculation( True )
    sgReductionSettings.SetReductionHeuristics( Simplygon.EReductionHeuristics_Consistent )
    
    # The importances can be changed here to allow the features to be weighed differently both during 
    # regular reduction and during the analyzing of modular seam 
    sgReductionSettings.SetEdgeSetImportance( 1.0 )
    sgReductionSettings.SetGeometryImportance( 1.0 )
    sgReductionSettings.SetGroupImportance( 1.0 )
    sgReductionSettings.SetMaterialImportance( 1.0 )
    sgReductionSettings.SetShadingImportance( 1.0 )
    sgReductionSettings.SetSkinningImportance( 1.0 )
    sgReductionSettings.SetTextureImportance( 1.0 )
    sgReductionSettings.SetVertexColorImportance( 1.0 )
    
    # The reduction targets below are only used for the regular reduction, not the modular seam 
    # analyzer 
    sgReductionSettings.SetReductionTargetTriangleRatio( triangleRatio )
    sgReductionSettings.SetReductionTargetMaxDeviation( maxDeviation )
    sgReductionSettings.SetReductionTargets(Simplygon.EStopCondition_All, True, False, True, False)

def GenerateModularSeams(sg: Simplygon.ISimplygon, sgModularAssetsScene: Simplygon.spScene):
    sgGeometryDataCollection = ExtractGeometriesInScene(sg, sgModularAssetsScene)
    
    # Figure out a small value in relation to the scene that will be the tolerance for the modular seams 
    # if a coordinate is moved a distance smaller than the tolerance, then it is regarded as the same 
    # coordinate so two vertices are the at the same place if the distance between them is smaller than 
    # radius * smallValue 
    sgModularAssetsScene.CalculateExtents()
    smallValue = 0.0001
    sceneRadius = sgModularAssetsScene.GetRadius()
    tolerance = sceneRadius * smallValue
    sgReductionSettings = sg.CreateReductionSettings()
    
    # The triangleRatio and maxDeviation are not important here and will not be used, only the 
    # relative importances and settings 
    ModifyReductionSettings(sgReductionSettings, 0.0, 0.0)
    
    # Create the modular seam analyzer. 
    sgModularSeamAnalyzer = sg.CreateModularSeamAnalyzer()
    sgModularSeamAnalyzer.SetTolerance( tolerance )
    sgModularSeamAnalyzer.SetIsTranslationIndependent( False )
    modularGeometryCount = sgGeometryDataCollection.GetItemCount()
    
    # Add the geometries to the analyzer 
    for modularGeometryId in range(modularGeometryCount):
        modularGeometryObject = sgGeometryDataCollection.GetItemAsObject(modularGeometryId)
        modularGeometry = Simplygon.spGeometryData.SafeCast(modularGeometryObject)
        sgModularSeamAnalyzer.AddGeometry(modularGeometry)
    
    # The analyzer needs to know the different reduction settings importances and such because it 
    # runs the reduction as far as possible for all the seams and stores the order and max deviations 
    # for future reductions of assets with the same seams 
    sgModularSeamAnalyzer.Analyze(sgReductionSettings)
    
    # Fetch the modular seams. These can be stored to file and used later 
    sgModularSeams = sgModularSeamAnalyzer.GetModularSeams()
    modularSeamsPath = ''.join(['output\\', 'ModularAssets.modseam'])
    sgModularSeams.SaveToFile(modularSeamsPath)

def LoadModularSeams(sg: Simplygon.ISimplygon):
    # Load pre-generated modular seams 
    sgModularSeams = sg.CreateModularSeams()
    modularSeamsPath = ''.join(['output\\', 'ModularAssets.modseam'])
    sgModularSeams.LoadFromFile(modularSeamsPath)
    return sgModularSeams

def RunReduction(sg: Simplygon.ISimplygon, sgModularAssetsScene: Simplygon.spScene, sgModularSeams: Simplygon.spModularSeams, triangleRatio, maxDeviation, modularSeamReductionRatio, modularSeamMaxDeviation):
    sgGeometryDataCollection = ExtractGeometriesInScene(sg, sgModularAssetsScene)
    modularGeometryCount = sgGeometryDataCollection.GetItemCount()
    
    # Add the geometries to the analyzer 
    for modularGeometryId in range(modularGeometryCount):
        modularGeometryObject = sgGeometryDataCollection.GetItemAsObject(modularGeometryId)
        modularGeometry = Simplygon.spGeometryData.SafeCast(modularGeometryObject)
        
        # Run reduction on each geometry individually, 
        # feed the modular seams into the reducer with the ModularSeamSettings 
        # so the modular seams are reduced identically and are untouched by the rest of the 
        # geometry reduction 
        sgSingleAssetScene = sgModularAssetsScene.NewCopy()
        
        # Remove all the geometries but keep any textures, materials etc. 
        sgSingleAssetScene.RemoveSceneNodes()
        
        # Add just a copy of the current geometry to the scene 
        modularGeometryCopy = modularGeometry.NewCopy(True)
        sgRootNode = sgSingleAssetScene.GetRootNode()
        sgSceneMesh = sgRootNode.CreateChildMesh(modularGeometryCopy)
        

        sgReductionProcessor = sg.CreateReductionProcessor()
        sgReductionProcessor.SetScene( sgSingleAssetScene )
        sgReductionSettings = sgReductionProcessor.GetReductionSettings()
        sgModularSeamSettings = sgReductionProcessor.GetModularSeamSettings()
        
        # Set the same reduction (importance) settings as the modular seam analyzer for consistent 
        # quality 
        ModifyReductionSettings(sgReductionSettings, triangleRatio, maxDeviation)
        sgModularSeamSettings.SetReductionRatio( modularSeamReductionRatio )
        sgModularSeamSettings.SetMaxDeviation( modularSeamMaxDeviation )
        sgModularSeamSettings.SetStopCondition( Simplygon.EStopCondition_All )
        sgModularSeamSettings.SetModularSeams( sgModularSeams )
        

        sgReductionProcessor.RunProcessing()
        geomName = modularGeometry.GetName()
        outputName = ''.join([geomName, '.obj'])
        SaveScene(sg, sgSingleAssetScene, outputName)

def RunReductionWithModularSeams(sg: Simplygon.ISimplygon):
    # Set reduction targets. Stop condition is set to 'All' 
    triangleRatio = 0.5
    maxDeviation = 0.0
    modularSeamReductionRatio = 0.75
    modularSeamMaxDeviation = 0.0
    
    # Load a scene that has a few modular assets in it as different scene meshes. 
    sgModularAssetsScene = LoadScene(sg, '../../../Assets/ModularAssets/ModularAssets.obj')
    generateNewSeams = True
    if generateNewSeams:
        GenerateModularSeams(sg, sgModularAssetsScene)
    sgModularSeams = LoadModularSeams(sg)
    DebugModularSeams(sg, True, sgModularSeams)
    
    # Run the reduction. The seams are reduced identically and the rest of the geometries are reduced 
    # like normal 
    RunReduction(sg, sgModularAssetsScene, sgModularSeams, triangleRatio, maxDeviation, modularSeamReductionRatio, modularSeamMaxDeviation)
    
    # Check log for any warnings or errors.     
    print("Check log for any warnings or errors.")
    CheckLog(sg)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        RunReductionWithModularSeams(sg)

        sg = None
        gc.collect()

