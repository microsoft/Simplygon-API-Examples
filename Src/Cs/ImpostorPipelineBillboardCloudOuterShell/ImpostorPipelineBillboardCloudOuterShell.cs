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
        string outputScenePath = string.Join("", new string[] { "output\\", "ImpostorPipelineBillboardCloudOuterShell", "_", path });
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
        
        // Error out if Simplygon has errors. 
        if (hasErrors)
        {
            throw new System.Exception("Processing failed with an error");
        }
    }

    static void RunBillboardCloudPipeline(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/Cages/Cages.obj");
        
        // For all materials in the scene set the blend mode to blend (instead of opaque) 
        for (int i = 0; i < (int)sgScene.GetMaterialTable().GetMaterialsCount(); ++i)
        {
            sgScene.GetMaterialTable().GetMaterial(i).SetBlendMode(Simplygon.EMaterialBlendMode.Blend);
        }
        
        // Create the Impostor processor. 
        using Simplygon.spBillboardCloudPipeline sgBillboardCloudPipeline = sg.CreateBillboardCloudPipeline();
        using Simplygon.spBillboardCloudSettings sgBillboardCloudSettings = sgBillboardCloudPipeline.GetBillboardCloudSettings();
        using Simplygon.spMappingImageSettings sgMappingImageSettings = sgBillboardCloudPipeline.GetMappingImageSettings();
        
        // Set billboard cloud mode to Outer shell. 
        sgBillboardCloudSettings.SetBillboardMode( Simplygon.EBillboardMode.OuterShell );
        sgBillboardCloudSettings.SetBillboardDensity( 0.5f );
        sgBillboardCloudSettings.SetGeometricComplexity( 0.9f );
        sgBillboardCloudSettings.SetMaxPlaneCount( 20 );
        sgBillboardCloudSettings.SetTwoSided( false );
        sgMappingImageSettings.SetMaximumLayers( 10 );
        using Simplygon.spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0);
        
        // Setting the size of the output material for the mapping image. This will be the output size of the 
        // textures when we do material casting in a later stage. 
        sgOutputMaterialSettings.SetTextureWidth( 1024 );
        sgOutputMaterialSettings.SetTextureHeight( 1024 );
        sgOutputMaterialSettings.SetMultisamplingLevel( 2 );
        
        // Add diffuse material caster to pipeline. 
        using Simplygon.spColorCaster sgDiffuseCaster = sg.CreateColorCaster();

        using Simplygon.spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings();
        sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" );
        sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgDiffuseCasterSettings.SetBakeOpacityInAlpha( false );
        sgDiffuseCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat.R8G8B8 );
        sgDiffuseCasterSettings.SetDilation( 10 );
        sgDiffuseCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.Interpolate );

        sgBillboardCloudPipeline.AddMaterialCaster( sgDiffuseCaster, 0 );
        
        // Add specular material caster to pipeline. 
        using Simplygon.spColorCaster sgSpecularCaster = sg.CreateColorCaster();

        using Simplygon.spColorCasterSettings sgSpecularCasterSettings = sgSpecularCaster.GetColorCasterSettings();
        sgSpecularCasterSettings.SetMaterialChannel( "Specular" );
        sgSpecularCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgSpecularCasterSettings.SetDilation( 10 );
        sgSpecularCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.Interpolate );

        sgBillboardCloudPipeline.AddMaterialCaster( sgSpecularCaster, 0 );
        
        // Add normals material caster to pipeline. 
        using Simplygon.spNormalCaster sgNormalsCaster = sg.CreateNormalCaster();

        using Simplygon.spNormalCasterSettings sgNormalsCasterSettings = sgNormalsCaster.GetNormalCasterSettings();
        sgNormalsCasterSettings.SetMaterialChannel( "Normals" );
        sgNormalsCasterSettings.SetGenerateTangentSpaceNormals( true );
        sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgNormalsCasterSettings.SetDilation( 10 );
        sgNormalsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.Interpolate );

        sgBillboardCloudPipeline.AddMaterialCaster( sgNormalsCaster, 0 );
        
        // Setup and run the opacity material casting. Make sure the there is no dilation or fill. 
        using Simplygon.spOpacityCaster sgOpacityCaster = sg.CreateOpacityCaster();

        using Simplygon.spOpacityCasterSettings sgOpacityCasterSettings = sgOpacityCaster.GetOpacityCasterSettings();
        sgOpacityCasterSettings.SetMaterialChannel( "Opacity" );
        sgOpacityCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgOpacityCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.NoFill );
        sgOpacityCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat.R8 );

        sgBillboardCloudPipeline.AddMaterialCaster( sgOpacityCaster, 0 );
        
        // Start the impostor pipeline.         
        Console.WriteLine("Start the impostor pipeline.");
        sgBillboardCloudPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode.RunInThisProcess);
        
        // Get the processed scene. 
        using Simplygon.spScene sgProcessedScene = sgBillboardCloudPipeline.GetProcessedScene();
        
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
        RunBillboardCloudPipeline(sg);

        return 0;
    }

}
