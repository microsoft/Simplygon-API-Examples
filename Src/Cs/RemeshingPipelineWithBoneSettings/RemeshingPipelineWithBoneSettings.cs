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
        string outputScenePath = string.Join("", new string[] { "output\\", "RemeshingPipelineWithBoneSettings", "_", path });
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

    static void RunRemeshing(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/RiggedSimplygonMan/RiggedSimplygonMan.glb");
        
        // Create the remeshing pipeline. 
        using Simplygon.spRemeshingPipeline sgRemeshingPipeline = sg.CreateRemeshingPipeline();
        using Simplygon.spRemeshingSettings sgRemeshingSettings = sgRemeshingPipeline.GetRemeshingSettings();
        using Simplygon.spBoneSettings sgBoneSettings = sgRemeshingPipeline.GetBoneSettings();
        
        // Set on-screen size target for remeshing. 
        sgRemeshingSettings.SetOnScreenSize( 300 );
        
        // Enable bone reducer. 
        sgBoneSettings.SetUseBoneReducer( true );
        
        // Set bone reduction target to bone ratio with a ratio of 50%. 
        sgBoneSettings.SetBoneReductionTargets( Simplygon.EStopCondition.All, true, false, false, false );
        sgBoneSettings.SetBoneReductionTargetBoneRatio( 0.5f );
        
        // Set bones per vertex limitations. 
        sgBoneSettings.SetLimitBonesPerVertex( true );
        sgBoneSettings.SetMaxBonePerVertex( 8 );
        
        // Remove unused bones. 
        sgBoneSettings.SetRemoveUnusedBones( true );
        
        // Start the remeshing pipeline.         
        Console.WriteLine("Start the remeshing pipeline.");
        sgRemeshingPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode.RunInThisProcess);
        
        // Get the processed scene. 
        using Simplygon.spScene sgProcessedScene = sgRemeshingPipeline.GetProcessedScene();
        
        // Since we are not casting any materials in this example, add a default material to silence 
        // validation warnings in the exporter. 
        sgScene.GetMaterialTable().AddMaterial( sg.CreateMaterial() );
        
        // Save processed scene.         
        Console.WriteLine("Save processed scene.");
        SaveScene(sg, sgProcessedScene, "Output.glb");
        
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
        RunRemeshing(sg);

        return 0;
    }

}
