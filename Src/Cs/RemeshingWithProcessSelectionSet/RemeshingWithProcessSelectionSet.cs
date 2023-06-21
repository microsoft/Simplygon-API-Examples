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
        string outputScenePath = string.Join("", new string[] { "output\\", "RemeshingWithProcessSelectionSet", "_", path });
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
        
        // Error out if Simplygon has errors. 
        if (hasErrors)
        {
            throw new System.Exception("Processing failed with an error");
        }
    }

    static void RunRemeshing(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/ObscuredTeapot.obj");
        
        // Create the remeshing processor. 
        using Simplygon.spRemeshingProcessor sgRemeshingProcessor = sg.CreateRemeshingProcessor();
        sgRemeshingProcessor.SetScene( sgScene );
        using Simplygon.spRemeshingSettings sgRemeshingSettings = sgRemeshingProcessor.GetRemeshingSettings();
        
        // Add a selection set to the scene with all nodes which should be remeshed. 
        using Simplygon.spSelectionSetTable sgSceneSelectionSetTable = sgScene.GetSelectionSetTable();
        using Simplygon.spSelectionSet sgRemeshingTargetSelectionSet = sg.CreateSelectionSet();
        sgRemeshingTargetSelectionSet.SetName("RemeshingTarget");
        Simplygon.spSceneNode sgRootTeapot001 = sgScene.GetNodeFromPath("Root/Teapot001");
        if (!sgRootTeapot001.IsNull())
            sgRemeshingTargetSelectionSet.AddItem(sgRootTeapot001.GetNodeGUID());
        sgSceneSelectionSetTable.AddSelectionSet(sgRemeshingTargetSelectionSet);
        
        // Set on-screen size target for remeshing. 
        sgRemeshingSettings.SetOnScreenSize( 300 );
        
        // Use the selection set created earlier. 
        sgRemeshingSettings.SetProcessSelectionSetName( "RemeshingTarget" );
        
        // Start the remeshing process.         
        Console.WriteLine("Start the remeshing process.");
        sgRemeshingProcessor.RunProcessing();
        
        // Replace original materials and textures from the scene with a new empty material, as the 
        // remeshed object has a new UV set.  
        sgScene.GetTextureTable().Clear();
        sgScene.GetMaterialTable().Clear();
        var defaultMaterial = sg.CreateMaterial();
        defaultMaterial.SetName("defaultMaterial");
        sgScene.GetMaterialTable().AddMaterial( defaultMaterial );
        
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
        RunRemeshing(sg);

        return 0;
    }

}
