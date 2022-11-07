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
        string outputScenePath = string.Join("", new string[] { "output\\", "ReductionWithVisibilityCamera", "_", path });
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
                Console.WriteLine("Errors:");
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
            Console.WriteLine("No errors.");
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
                Console.WriteLine("Warnings:");
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
            Console.WriteLine("No warnings.");
        }
    }
    static void RunReduction(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/Teapot.obj");
        
        // Create the reduction processor. 
        using Simplygon.spReductionProcessor sgReductionProcessor = sg.CreateReductionProcessor();
        sgReductionProcessor.SetScene( sgScene );
        using Simplygon.spReductionSettings sgReductionSettings = sgReductionProcessor.GetReductionSettings();
        using Simplygon.spVisibilitySettings sgVisibilitySettings = sgReductionProcessor.GetVisibilitySettings();
        
        // Set reduction target to triangle ratio with a ratio of 50%. 
        sgReductionSettings.SetReductionTargets( Simplygon.EStopCondition.All, true, false, false, false );
        sgReductionSettings.SetReductionTargetTriangleRatio( 0.5f );
        
        // Add a camera to the scene. We'll use this later as a visibility camera. 
        using Simplygon.spSelectionSetTable sgSceneSelectionSetTable = sgScene.GetSelectionSetTable();
        using Simplygon.spSelectionSet sgCameraSelectionSet = sg.CreateSelectionSet();
        sgCameraSelectionSet.SetName("Camera");
        using Simplygon.spSceneCamera sgCameraSceneCamera = sg.CreateSceneCamera();
        sgCameraSceneCamera.SetCustomSphereCameraPath(4, 90, 180, 90);
        sgScene.GetRootNode().AddChild(sgCameraSceneCamera);
        sgCameraSelectionSet.AddItem(sgCameraSceneCamera.GetNodeGUID());
        sgSceneSelectionSetTable.AddSelectionSet(sgCameraSelectionSet);
        
        // Use the camera previously added. 
        sgVisibilitySettings.SetCameraSelectionSetName( "Camera" );
        
        // Enabled GPU based visibility calculations. 
        sgVisibilitySettings.SetComputeVisibilityMode( Simplygon.EComputeVisibilityMode.DirectX );
        
        // Disabled conservative mode. 
        sgVisibilitySettings.SetConservativeMode( false );
        
        // Remove all non visible geometry. 
        sgVisibilitySettings.SetCullOccludedGeometry( true );
        
        // Skip filling nonvisible regions. 
        sgVisibilitySettings.SetFillNonVisibleAreaThreshold( 0.0f );
        
        // Don't remove non occluding triangles. 
        sgVisibilitySettings.SetRemoveTrianglesNotOccludingOtherTriangles( false );
        
        // Remove all back facing triangles. 
        sgVisibilitySettings.SetUseBackfaceCulling( true );
        
        // Don't use visibility weights. 
        sgVisibilitySettings.SetUseVisibilityWeightsInReducer( false );
        
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
