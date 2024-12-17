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
        string outputScenePath = string.Join("", new string[] { "output\\", "RemeshingWithDisplacementTessellation", "_", path });
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

    static void RunRemeshingWithDisplacementTessellation(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
        
        // Create the remeshing processor. 
        using Simplygon.spRemeshingProcessor sgRemeshingProcessor = sg.CreateRemeshingProcessor();
        sgRemeshingProcessor.SetScene( sgScene );
        
        // Fetch all the needed settings objects for the processing, including the attribute 
        // tessellation settings, which we will set up to receive displacement data in the processed 
        // mesh. 
        using Simplygon.spRemeshingSettings sgRemeshingSettings = sgRemeshingProcessor.GetRemeshingSettings();
        using Simplygon.spAttributeTessellationSettings sgAttributeTessellationSettings = sgRemeshingProcessor.GetAttributeTessellationSettings();
        
        // Set on-screen size target for remeshing, and tell the remeshing processor to cast 
        // displacement data into the attribute tessellation field of the processed geometry. Note: The 
        // tessellation settings are defined in the section below. 
        sgRemeshingSettings.SetOnScreenSize( 1000 );
        sgRemeshingSettings.SetPopulateAttributeTessellationDisplacement( true );
        sgRemeshingSettings.SetSurfaceTransferMode( Simplygon.ESurfaceTransferMode.Fast );
        
        // Set the tessellation settings. The displacement data will be cast into a tessellated 
        // displacement attribute. In this example we use OnScreenSize as the density setting, which 
        // means that triangles are tessellated based on the size of the rendered object, so that a 
        // triangle is when tessellated roughly the size of a pixel. We also add some additional 
        // constraints, such as only allowing base triangles to tessellate to level 5 (1024 
        // sub-triangles), only allow one level of difference between neighbor base-triangles, and the 
        // total number of sub-triangles should not exceed 1000000. 
        sgAttributeTessellationSettings.SetEnableAttributeTessellation( true );
        sgAttributeTessellationSettings.SetAttributeTessellationDensityMode( Simplygon.EAttributeTessellationDensityMode.OnScreenSize );
        sgAttributeTessellationSettings.SetOnScreenSize( 1000 );
        sgAttributeTessellationSettings.SetOnlyAllowOneLevelOfDifference( true );
        sgAttributeTessellationSettings.SetMinTessellationLevel( 0 );
        sgAttributeTessellationSettings.SetMaxTessellationLevel( 5 );
        sgAttributeTessellationSettings.SetMaxTotalValuesCount( 1000000 );
        
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
        
        // Save processed remeshed scene.         
        Console.WriteLine("Save processed remeshed scene.");
        SaveScene(sg, sgScene, "OutputBase.obj");
        
        // We will now create an attribute tessellation tool object, and generate a scene with the 
        // tessellated attribute displacement data generated into real tessellated mesh data, which is 
        // stored into a new scene. 
        using Simplygon.spAttributeTessellation sgAttributeTessellation = sg.CreateAttributeTessellation();
        using Simplygon.spScene sgTessellatedScene = sgAttributeTessellation.NewTessellatedScene(sgScene);
        
        // Save the tessellated copy of the scene.         
        Console.WriteLine("Save the tessellated copy of the scene.");
        SaveScene(sg, sgTessellatedScene, "OutputTessellation.obj");
        
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
        RunRemeshingWithDisplacementTessellation(sg);

        return 0;
    }

}
