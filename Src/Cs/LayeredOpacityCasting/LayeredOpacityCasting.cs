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
    static void RunOpacityCasting(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/Console/Console.obj");
        
        // Create the remeshing processor. 
        using Simplygon.spRemeshingProcessor sgRemeshingProcessor = sg.CreateRemeshingProcessor();
        sgRemeshingProcessor.SetScene( sgScene );
        using Simplygon.spRemeshingSettings sgRemeshingSettings = sgRemeshingProcessor.GetRemeshingSettings();
        using Simplygon.spMappingImageSettings sgMappingImageSettings = sgRemeshingProcessor.GetMappingImageSettings();
        
        // Set on-screen size target for remeshing. 
        sgRemeshingSettings.SetOnScreenSize( 300 );
        
        // Generates a mapping image which is used after the remeshing to cast new materials to the new 
        // remeshed object. 
        sgMappingImageSettings.SetGenerateMappingImage( true );
        sgMappingImageSettings.SetApplyNewMaterialIds( true );
        sgMappingImageSettings.SetGenerateTangents( true );
        sgMappingImageSettings.SetUseFullRetexturing( true );
        using Simplygon.spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0);
        
        // Setting the size of the output material for the mapping image. This will be the output size of the 
        // textures when we do material casting in a later stage. 
        sgOutputMaterialSettings.SetTextureWidth( 2048 );
        sgOutputMaterialSettings.SetTextureHeight( 2048 );
        
        // Start the remeshing process.         
        Console.WriteLine("Start the remeshing process.");
        sgRemeshingProcessor.RunProcessing();
        
        // Setup and run the layered diffuse material casting using the selected opacity channel.         
        Console.WriteLine("Setup and run the layered diffuse material casting using the selected opacity channel.");
        string diffuseTextureFilePath;
        using Simplygon.spColorCaster sgDiffuseCaster = sg.CreateColorCaster();
        sgDiffuseCaster.SetMappingImage( sgRemeshingProcessor.GetMappingImage() );
        sgDiffuseCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgDiffuseCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgDiffuseCaster.SetOutputFilePath( "DiffuseTexture" );

        using Simplygon.spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings();
        sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" );
        sgDiffuseCasterSettings.SetOpacityChannel( "Opacity" );
        sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );

        sgDiffuseCaster.RunProcessing();
        diffuseTextureFilePath = sgDiffuseCaster.GetOutputFilePath();
        
        // Update scene with new casted textures. 
        using Simplygon.spMaterialTable sgMaterialTable = sg.CreateMaterialTable();
        using Simplygon.spTextureTable sgTextureTable = sg.CreateTextureTable();
        using Simplygon.spMaterial sgMaterial = sg.CreateMaterial();
        using Simplygon.spTexture sgDiffuseTexture = sg.CreateTexture();
        sgDiffuseTexture.SetName( "Diffuse" );
        sgDiffuseTexture.SetFilePath( diffuseTextureFilePath );
        sgTextureTable.AddTexture( sgDiffuseTexture );

        using Simplygon.spShadingTextureNode sgDiffuseTextureShadingNode = sg.CreateShadingTextureNode();
        sgDiffuseTextureShadingNode.SetTexCoordLevel( 0 );
        sgDiffuseTextureShadingNode.SetTextureName( "Diffuse" );

        sgMaterial.AddMaterialChannel( "Diffuse" );
        sgMaterial.SetShadingNetwork( "Diffuse", sgDiffuseTextureShadingNode );

        sgMaterialTable.AddMaterial( sgMaterial );

        sgScene.GetTextureTable().Clear();
        sgScene.GetMaterialTable().Clear();
        sgScene.GetTextureTable().Copy(sgTextureTable);
        sgScene.GetMaterialTable().Copy(sgMaterialTable);
        
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
        RunOpacityCasting(sg);

        return 0;
    }
}
