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
        bool importResult = sgSceneImporter.RunImport();
        if (!importResult)
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
        sgSceneExporter.SetExportFilePath(path);
        sgSceneExporter.SetScene(sgScene);
        
        // Run scene exporter. 
        bool exportResult = sgSceneExporter.RunExport();
        if (!exportResult)
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
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/ObscuredTeapot.obj");
        
        // Create the reduction pipeline. 
        using Simplygon.spReductionPipeline sgReductionPipeline = sg.CreateReductionPipeline();
        using Simplygon.spReductionSettings sgReductionSettings = sgReductionPipeline.GetReductionSettings();
        using Simplygon.spVisibilitySettings sgVisibilitySettings = sgReductionPipeline.GetVisibilitySettings();
        
        // Set reduction target to triangle ratio with a ratio of 50%. 
        sgReductionSettings.SetReductionTargets( Simplygon.EStopCondition.All, true, false, false, false );
        sgReductionSettings.SetReductionTargetTriangleRatio( 0.5f );
        
        // Add a selection set to the scene. We'll use this later as a occluder. 
        using Simplygon.spSelectionSetTable sgSceneSelectionSetTable = sgScene.GetSelectionSetTable();
        using Simplygon.spSelectionSet sgOccluderSelectionSet = sg.CreateSelectionSet();
        sgOccluderSelectionSet.SetName("Occluder");
        Simplygon.spSceneNode sgRootBox002 = sgScene.GetNodeFromPath("Root/Box002");
        if (!sgRootBox002.IsNull())
            sgOccluderSelectionSet.AddItem(sgRootBox002.GetNodeGUID());
        sgSceneSelectionSetTable.AddSelectionSet(sgOccluderSelectionSet);
        
        // Use the occluder previously added. 
        sgVisibilitySettings.SetOccluderSelectionSetName( "Occluder" );
        
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
        
        // Start the reduction pipeline.         
        Console.WriteLine("Start the reduction pipeline.");
        sgReductionPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode.RunInThisProcess);
        
        // Get the processed scene. 
        using Simplygon.spScene sgProcessedScene = sgReductionPipeline.GetProcessedScene();
        
        // Save processed scene.         
        Console.WriteLine("Save processed scene.");
        SaveScene(sg, sgProcessedScene, "Output.fbx");
        
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
