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
    outputScenePath = ''.join(['output\\', 'ReductionWithGeomorph', '_', path])
    sgSceneExporter.SetExportFilePath(outputScenePath)
    sgSceneExporter.SetScene(sgScene)
    
    # Run scene exporter. 
    exportResult = sgSceneExporter.Run()
    if Simplygon.Failed(exportResult):
        raise Exception('Failed to save scene.')

def ArrayBlend(targetArray: Simplygon.spRealArray, origArray: Simplygon.spRealArray, morphArray: Simplygon.spRealArray, blend):
    itemCount = targetArray.GetItemCount()
    
    # Morph the geometry 
    for i in range(itemCount):
        origValue = origArray.GetItem(i)
        morphValue = morphArray.GetItem(i)
        blendResult = morphValue * blend + origValue * (1.0-blend)
        targetArray.SetItem(i, blendResult)

def MorphGeometry(originalSceneMesh: Simplygon.spSceneMesh, morphSceneMesh: Simplygon.spSceneMesh, blendSceneMesh: Simplygon.spSceneMesh, blend):
    originalGeometry = originalSceneMesh.GetGeometry()
    morphGeometry = morphSceneMesh.GetGeometry()
    blendGeometry = blendSceneMesh.GetGeometry()
    origArray = originalGeometry.GetCoords()
    morphArray = morphGeometry.GetCoords()
    targetArray = blendGeometry.GetCoords()
    origVertexIdsArray = originalGeometry.GetVertexIds()
    morphVertexIdsArray = morphGeometry.GetVertexIds()
    targetVertexIdsArray = blendGeometry.GetVertexIds()
    itemCount = targetVertexIdsArray.GetItemCount()
    
    # For each corner: 
    for c in range(itemCount):
        origVertexId = origVertexIdsArray.GetItem(c)
        morphVertexId = morphVertexIdsArray.GetItem(c)
        targetVertexId = targetVertexIdsArray.GetItem(c)
        
        # For each x,z,y value in each coordinate: 
        for d in range(3):
            dInt = d
            origValue = origArray.GetItem(origVertexId * 3 + dInt)
            morphValue = morphArray.GetItem(morphVertexId * 3 + dInt)
            blendResult = morphValue * blend + origValue * (1.0-blend)
            targetArray.SetItem(targetVertexId * 3 + dInt, blendResult)
    
    # Morph the UVs 
    targetArray = blendGeometry.GetTexCoords(0)
    origArray = originalGeometry.GetTexCoords(0)
    morphArray = morphGeometry.GetTexCoords(0)
    ArrayBlend(targetArray, origArray, morphArray, blend)
    
    # Morph the Normals 
    targetArray = blendGeometry.GetNormals()
    origArray = originalGeometry.GetNormals()
    morphArray = morphGeometry.GetNormals()
    ArrayBlend(targetArray, origArray, morphArray, blend)

def RunReductionWithGeomorph(sg: Simplygon.ISimplygon):
    # Load scene to process.     
    print("Load scene to process.")
    sgScene = LoadScene(sg, '../../../Assets/SimplygonMan/SimplygonMan.obj')
    
    # Create the reduction processor. 
    sgReductionProcessor = sg.CreateReductionProcessor()
    
    # Select Mesh Nodes 
    selectionSetId = sgScene.SelectNodes('ISceneMesh')
    
    # Keep a copy of the original scene to be able to blend between this and the morph geometry. 
    originalScene = sgScene.NewCopy()
    blendScene = sgScene.NewCopy()
    sgReductionProcessor.SetScene( sgScene )
    sgReductionSettings = sgReductionProcessor.GetReductionSettings()
    
    # Set reduction target to triangle ratio with a ratio of 50%. Enable geomorph creation and 
    # disable symmetry quad retriangulator. 
    sgReductionSettings.SetReductionTargets( Simplygon.EStopCondition_All, True, False, False, False )
    sgReductionSettings.SetReductionTargetTriangleRatio( 0.5 )
    sgReductionSettings.SetCreateGeomorphGeometry( True )
    sgReductionSettings.SetUseSymmetryQuadRetriangulator( False )
    sgRepairSettings = sgReductionProcessor.GetRepairSettings()
    
    # Disable TJunctionRemover. 
    sgRepairSettings.SetWeldDist( 0.0 )
    sgRepairSettings.SetUseTJunctionRemover( False )
    sgNormalCalculationSettings = sgReductionProcessor.GetNormalCalculationSettings()
    
    # Replace normals. 
    sgNormalCalculationSettings.SetReplaceNormals( True )
    sgNormalCalculationSettings.SetHardEdgeAngle( 60.0 )
    
    # Start the reduction process.     
    print("Start the reduction process.")
    sgReductionProcessor.RunProcessing()
    set = sgScene.GetSelectionSetTable().GetSelectionSet(selectionSetId)

    
    # Selection set to collect geomorph nodes in scene. 
    geomorphSelectionSet = sg.CreateSelectionSet()
    
    # Morph the geometry 
    for blendValue in range(11):
        geometryCount = set.GetItemCount()
        
        # The LOD scene contains morphed geometries, placed as children of their respective reduced 
        # geometry mesh node. 
        # The name of a morph mesh node is the same as its parent, with "_geomorph" added. 
        for geomIndex in range(geometryCount):
            guid = set.GetItem(geomIndex)
            lodSceneNode = sgScene.GetNodeByGUID(guid)
            originalSceneNode = originalScene.GetNodeByGUID(guid)
            blendSceneNode = blendScene.GetNodeByGUID(guid)
            lodSceneMesh = Simplygon.spSceneMesh.SafeCast(lodSceneNode)
            originalSceneMesh = Simplygon.spSceneMesh.SafeCast(originalSceneNode)
            blendSceneMesh = Simplygon.spSceneMesh.SafeCast(blendSceneNode)
            childCount = lodSceneNode.GetChildCount()
            for childIndex in range(childCount):
                childNode = lodSceneNode.GetChild(childIndex)
                childName = childNode.GetName()
                childNodeString = ''.join([childName])
                name = lodSceneNode.GetName()
                geomorphName = ''.join(['_geomorph'])
                if geomorphName in childNodeString:
                    morphSceneMesh = Simplygon.spSceneMesh.SafeCast(childNode)
                    morphGUID = morphSceneMesh.GetNodeGUID()
                    geomorphSelectionSet.AddItem(morphGUID)
                    MorphGeometry(originalSceneMesh, morphSceneMesh, blendSceneMesh, blendValue*0.1)
        outputName = ''.join(['Geomorph_BlendedGeometry', str(blendValue), '.obj'])
        
        # Save processed scene.         
        print("Save processed scene.")
        SaveScene(sg, blendScene, outputName)

if __name__ == '__main__':
        sg = simplygon_loader.init_simplygon()
        if sg is None:
            exit(Simplygon.GetLastInitializationError())

        RunReductionWithGeomorph(sg)

        sg = None
        gc.collect()

