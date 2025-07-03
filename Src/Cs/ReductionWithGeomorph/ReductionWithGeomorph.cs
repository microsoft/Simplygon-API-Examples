// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

using System;
using System.IO;
using System.Threading.Tasks;

public class Program
{
    static Simplygon.spScene LoadScene(Simplygon.ISimplygon sg, string path)
    {
        // Create scene importer 
        using Simplygon.spSceneImporter sgSceneImporter = sg.CreateSceneImporter();
        sgSceneImporter.SetImportFilePath(path);
        
        // Run scene importer. 
        var importResult = sgSceneImporter.Run();
        if (Simplygon.Simplygon.Failed(importResult))
        {
            throw new System.Exception("Failed to load scene.");
        }
        Simplygon.spScene sgScene = sgSceneImporter.GetScene();
        return sgScene;
    }

    static void SaveScene(Simplygon.ISimplygon sg, Simplygon.spScene sgScene, string path)
    {
        // Create scene exporter. 
        using Simplygon.spSceneExporter sgSceneExporter = sg.CreateSceneExporter();
        string outputScenePath = string.Join("", new string[] { "output\\", "ReductionWithGeomorph", "_", path });
        sgSceneExporter.SetExportFilePath(outputScenePath);
        sgSceneExporter.SetScene(sgScene);
        
        // Run scene exporter. 
        var exportResult = sgSceneExporter.Run();
        if (Simplygon.Simplygon.Failed(exportResult))
        {
            throw new System.Exception("Failed to save scene.");
        }
    }

    static void ArrayBlend(Simplygon.spRealArray targetArray, Simplygon.spRealArray origArray, Simplygon.spRealArray morphArray, float blend)
    {
        var itemCount = targetArray.GetItemCount();
        
        // Morph the geometry 
        for (uint i = 0; i < itemCount; ++i)
        {
            float origValue = origArray.GetItem((int)i);
            float morphValue = morphArray.GetItem((int)i);
            float blendResult = morphValue * blend + origValue * (1.0f-blend);
            targetArray.SetItem((int)i, blendResult);
        }
    }

    static void MorphGeometry(Simplygon.spSceneMesh originalSceneMesh, Simplygon.spSceneMesh morphSceneMesh, Simplygon.spSceneMesh blendSceneMesh, float blend)
    {
        Simplygon.spGeometryData originalGeometry = originalSceneMesh.GetGeometry();
        Simplygon.spGeometryData morphGeometry = morphSceneMesh.GetGeometry();
        Simplygon.spGeometryData blendGeometry = blendSceneMesh.GetGeometry();
        Simplygon.spRealArray origArray = originalGeometry.GetCoords();
        Simplygon.spRealArray morphArray = morphGeometry.GetCoords();
        Simplygon.spRealArray targetArray = blendGeometry.GetCoords();
        Simplygon.spRidArray origVertexIdsArray = originalGeometry.GetVertexIds();
        Simplygon.spRidArray morphVertexIdsArray = morphGeometry.GetVertexIds();
        Simplygon.spRidArray targetVertexIdsArray = blendGeometry.GetVertexIds();
        var itemCount = targetVertexIdsArray.GetItemCount();
        
        // For each corner: 
        for (uint c = 0; c < itemCount; ++c)
        {
            var origVertexId = origVertexIdsArray.GetItem((int)c);
            var morphVertexId = morphVertexIdsArray.GetItem((int)c);
            var targetVertexId = targetVertexIdsArray.GetItem((int)c);
            
            // For each x,z,y value in each coordinate: 
            for (uint d = 0; d < 3; ++d)
            {
                int dInt = (int)d;
                float origValue = origArray.GetItem(origVertexId * 3 + dInt);
                float morphValue = morphArray.GetItem(morphVertexId * 3 + dInt);
                float blendResult = morphValue * blend + origValue * (1.0f-blend);
                targetArray.SetItem(targetVertexId * 3 + dInt, blendResult);
            }
        }
        
        // Morph the UVs 
        targetArray = blendGeometry.GetTexCoords(0);
        origArray = originalGeometry.GetTexCoords(0);
        morphArray = morphGeometry.GetTexCoords(0);
        ArrayBlend(targetArray, origArray, morphArray, blend);
        
        // Morph the Normals 
        targetArray = blendGeometry.GetNormals();
        origArray = originalGeometry.GetNormals();
        morphArray = morphGeometry.GetNormals();
        ArrayBlend(targetArray, origArray, morphArray, blend);
    }

    static void RunReductionWithGeomorph(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
        
        // Create the reduction processor. 
        using Simplygon.spReductionProcessor sgReductionProcessor = sg.CreateReductionProcessor();
        
        // Select Mesh Nodes 
        int selectionSetId = sgScene.SelectNodes("ISceneMesh");
        
        // Keep a copy of the original scene to be able to blend between this and the morph geometry. 
        Simplygon.spScene originalScene = sgScene.NewCopy();
        Simplygon.spScene blendScene = sgScene.NewCopy();
        sgReductionProcessor.SetScene( sgScene );
        using Simplygon.spReductionSettings sgReductionSettings = sgReductionProcessor.GetReductionSettings();
        
        // Set reduction target to triangle ratio with a ratio of 50%. Enable geomorph creation and 
        // disable symmetry quad retriangulator. 
        sgReductionSettings.SetReductionTargets( Simplygon.EStopCondition.All, true, false, false, false );
        sgReductionSettings.SetReductionTargetTriangleRatio( 0.5f );
        sgReductionSettings.SetCreateGeomorphGeometry( true );
        sgReductionSettings.SetUseSymmetryQuadRetriangulator( false );
        using Simplygon.spRepairSettings sgRepairSettings = sgReductionProcessor.GetRepairSettings();
        
        // Disable TJunctionRemover. 
        sgRepairSettings.SetWeldDist( 0.0f );
        sgRepairSettings.SetUseTJunctionRemover( false );
        using Simplygon.spNormalCalculationSettings sgNormalCalculationSettings = sgReductionProcessor.GetNormalCalculationSettings();
        
        // Replace normals. 
        sgNormalCalculationSettings.SetReplaceNormals( true );
        sgNormalCalculationSettings.SetHardEdgeAngle( 60.0f );
        
        // Start the reduction process.         
        Console.WriteLine("Start the reduction process.");
        sgReductionProcessor.RunProcessing();
        var set = sgScene.GetSelectionSetTable().GetSelectionSet(selectionSetId);

        
        // Selection set to collect geomorph nodes in scene. 
        using Simplygon.spSelectionSet geomorphSelectionSet = sg.CreateSelectionSet();
        
        // Morph the geometry 
        for (uint blendValue = 0; blendValue < 11; ++blendValue)
        {
            var geometryCount = set.GetItemCount();
            
            // The LOD scene contains morphed geometries, placed as children of their respective reduced 
            // geometry mesh node. 
            // The name of a morph mesh node is the same as its parent, with "_geomorph" added. 
            for (uint geomIndex = 0; geomIndex < geometryCount; ++geomIndex)
            {
                var guid = set.GetItem(geomIndex);
                Simplygon.spSceneNode lodSceneNode = sgScene.GetNodeByGUID(guid);
                Simplygon.spSceneNode originalSceneNode = originalScene.GetNodeByGUID(guid);
                Simplygon.spSceneNode blendSceneNode = blendScene.GetNodeByGUID(guid);
                Simplygon.spSceneMesh lodSceneMesh = Simplygon.spSceneMesh.SafeCast(lodSceneNode);
                Simplygon.spSceneMesh originalSceneMesh = Simplygon.spSceneMesh.SafeCast(originalSceneNode);
                Simplygon.spSceneMesh blendSceneMesh = Simplygon.spSceneMesh.SafeCast(blendSceneNode);
                var childCount = lodSceneNode.GetChildCount();
                for (uint childIndex = 0; childIndex < childCount; ++childIndex)
                {
                    Simplygon.spSceneNode childNode = lodSceneNode.GetChild((int)childIndex);
                    var childName = childNode.GetName();
                    string childNodeString = string.Join("", new string[] { childName });
                    var name = lodSceneNode.GetName();
                    string geomorphName = string.Join("", new string[] { "_geomorph" });
                    if (childNodeString.Contains(geomorphName) )
                    {
                        Simplygon.spSceneMesh morphSceneMesh = Simplygon.spSceneMesh.SafeCast(childNode);
                        var morphGUID = morphSceneMesh.GetNodeGUID();
                        geomorphSelectionSet.AddItem(morphGUID);
                        MorphGeometry(originalSceneMesh, morphSceneMesh, blendSceneMesh, blendValue*0.1f);
                    }
                }
            }
            string outputName = string.Join("", new string[] { "Geomorph_BlendedGeometry", blendValue.ToString(), ".obj" });
            
            // Save processed scene.             
            Console.WriteLine("Save processed scene.");
            SaveScene(sg, blendScene, outputName);
        }
    }

    static int Main(string[] args)
    {
        using var sg = Simplygon.Loader.InitSimplygon(out var errorCode, out var errorMessage);
        if (errorCode != Simplygon.EErrorCodes.NoError)
        {
            Console.WriteLine( $"Failed to initialize Simplygon: ErrorCode({(int)errorCode}) {errorMessage}" );
            return (int)errorCode;
        }
        RunReductionWithGeomorph(sg);

        return 0;
    }

}
