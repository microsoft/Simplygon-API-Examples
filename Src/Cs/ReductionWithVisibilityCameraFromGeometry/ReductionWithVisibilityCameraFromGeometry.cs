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
        string outputScenePath = string.Join("", new string[] { "output\\", "ReductionWithVisibilityCameraFromGeometry", "_", path });
        sgSceneExporter.SetExportFilePath(outputScenePath);
        sgSceneExporter.SetScene(sgScene);
        
        // Run scene exporter. 
        var exportResult = sgSceneExporter.Run();
        if (Simplygon.Simplygon.Failed(exportResult))
        {
            throw new System.Exception("Failed to save scene.");
        }
    }

    static void CheckLog(Simplygon.ISimplygon sg)
    {
        // Check if any errors occurred. 
        bool hasErrors = sg.ErrorOccurred();
        if (hasErrors)
        {
            Simplygon.spStringArray errors = sg.CreateStringArray();
            sg.GetErrorMessages(errors);
            var errorCount = errors.GetItemCount();
            if (errorCount > 0)
            {
                Console.WriteLine("CheckLog: Errors:");
                for (uint errorIndex = 0; errorIndex < errorCount; ++errorIndex)
                {
                    string errorString = errors.GetItem((int)errorIndex);
                    Console.WriteLine(errorString);
                }
                sg.ClearErrorMessages();
            }
        }
        else
        {
            Console.WriteLine("CheckLog: No errors.");
        }
        
        // Check if any warnings occurred. 
        bool hasWarnings = sg.WarningOccurred();
        if (hasWarnings)
        {
            Simplygon.spStringArray warnings = sg.CreateStringArray();
            sg.GetWarningMessages(warnings);
            var warningCount = warnings.GetItemCount();
            if (warningCount > 0)
            {
                Console.WriteLine("CheckLog: Warnings:");
                for (uint warningIndex = 0; warningIndex < warningCount; ++warningIndex)
                {
                    string warningString = warnings.GetItem((int)warningIndex);
                    Console.WriteLine(warningString);
                }
                sg.ClearWarningMessages();
            }
        }
        else
        {
            Console.WriteLine("CheckLog: No warnings.");
        }
        
        // Error out if Simplygon has errors. 
        if (hasErrors)
        {
            throw new System.Exception("Processing failed with an error");
        }
    }

    static void RunReduction(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/ObscuredTeapot.obj");
        
        // Load camera gemometry 
        Simplygon.spScene sgSceneGeometryCamera = LoadScene(sg, "../../../Assets/ObscuredTeapot/CameraMesh.obj");
        
        // Select Mesh Nodes 
        int selectionSetId = sgSceneGeometryCamera.SelectNodes("ISceneMesh");
        Simplygon.spSelectionSetTable sgSelectionSetsTable = sgSceneGeometryCamera.GetSelectionSetTable();
        Simplygon.spSelectionSet selectionSceneMeshes = sgSelectionSetsTable.GetSelectionSet(selectionSetId);
        var itemCount = selectionSceneMeshes.GetItemCount();
        Simplygon.spSelectionSet cameraSelectionSet = sg.CreateSelectionSet();
        
        // Copy each mesh from camera scene into a scene and created a camera selection set based on those 
        // ids. 
        for (uint meshIndex = 0; meshIndex < itemCount; ++meshIndex)
        {
            string meshNodeId = selectionSceneMeshes.GetItem(meshIndex);
            Simplygon.spSceneNode sceneNode = sgSceneGeometryCamera.GetNodeByGUID(meshNodeId);
            Simplygon.spSceneMesh sceneMesh = Simplygon.spSceneMesh.SafeCast(sceneNode);
            Simplygon.spGeometryData geom = sceneMesh.GetGeometry();
            Simplygon.spSceneMesh cameraMesh = sgScene.GetRootNode().CreateChildMesh(geom);

            string nodeId = cameraMesh.GetNodeGUID();
            cameraSelectionSet.AddItem(nodeId);
        }
        int cameraSelectionSetId = sgScene.GetSelectionSetTable().AddSelectionSet(cameraSelectionSet);

        
        // Create the reduction processor. 
        using Simplygon.spReductionProcessor sgReductionProcessor = sg.CreateReductionProcessor();
        
        // Get settings objects 
        using Simplygon.spReductionSettings sgReductionSettings = sgReductionProcessor.GetReductionSettings();
        using Simplygon.spVisibilitySettings sgVisibilitySettings = sgReductionProcessor.GetVisibilitySettings();
        
        // Set camera selection set id with 
        sgVisibilitySettings.SetCameraSelectionSetID( cameraSelectionSetId );
        
        // Setup visibility setting enable GPU based computation, 
        sgVisibilitySettings.SetUseVisibilityWeightsInReducer( true );
        sgVisibilitySettings.SetUseVisibilityWeightsInTexcoordGenerator( false );
        sgVisibilitySettings.SetComputeVisibilityMode( Simplygon.EComputeVisibilityMode.DirectX );
        sgVisibilitySettings.SetConservativeMode( false );
        sgVisibilitySettings.SetCullOccludedGeometry( true );
        sgVisibilitySettings.SetFillNonVisibleAreaThreshold( 0.0f );
        sgVisibilitySettings.SetRemoveTrianglesNotOccludingOtherTriangles( false );
        sgVisibilitySettings.SetUseBackfaceCulling( true );
        
        // Set reduction target to triangle ratio with a ratio of 50%. 
        sgReductionSettings.SetReductionTargetTriangleRatio( 0.5f );
        sgReductionProcessor.SetScene( sgScene );
        
        // Start the reduction process.         
        Console.WriteLine("Start the reduction process.");
        sgReductionProcessor.RunProcessing();
        
        // Save processed scene.         
        Console.WriteLine("Save processed scene.");
        SaveScene(sg, sgScene, "Output.fbx");
        
        // Check log for any warnings or errors.         
        Console.WriteLine("Check log for any warnings or errors.");
        CheckLog(sg);
    }

    static int Main(string[] args)
    {
        using var sg = Simplygon.Loader.InitSimplygon(out var errorCode, out var errorMessage);
        if (errorCode != Simplygon.EErrorCodes.NoError)
        {
            Console.WriteLine( $"Failed to initialize Simplygon: ErrorCode({(int)errorCode}) {errorMessage}" );
            return (int)errorCode;
        }
        RunReduction(sg);

        return 0;
    }

}
