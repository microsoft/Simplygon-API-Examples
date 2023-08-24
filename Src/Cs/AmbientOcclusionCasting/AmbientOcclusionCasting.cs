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

    static void AmbientOcclusionCasting(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
        
        // Create the aggregation processor. 
        using Simplygon.spAggregationProcessor sgAggregationProcessor = sg.CreateAggregationProcessor();
        sgAggregationProcessor.SetScene( sgScene );
        using Simplygon.spAggregationSettings sgAggregationSettings = sgAggregationProcessor.GetAggregationSettings();
        using Simplygon.spMappingImageSettings sgMappingImageSettings = sgAggregationProcessor.GetMappingImageSettings();
        
        // Merge all geometries into a single geometry. 
        sgAggregationSettings.SetMergeGeometries( true );
        
        // Generates a mapping image which is used after the aggregation to cast new materials to the new 
        // aggregated object. 
        sgMappingImageSettings.SetGenerateMappingImage( true );
        sgMappingImageSettings.SetApplyNewMaterialIds( true );
        sgMappingImageSettings.SetGenerateTangents( true );
        sgMappingImageSettings.SetUseFullRetexturing( true );
        using Simplygon.spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0);
        
        // Setting the size of the output material for the mapping image. This will be the output size of the 
        // textures when we do material casting in a later stage. 
        sgOutputMaterialSettings.SetTextureWidth( 2048 );
        sgOutputMaterialSettings.SetTextureHeight( 2048 );
        
        // Start the aggregation process.         
        Console.WriteLine("Start the aggregation process.");
        sgAggregationProcessor.RunProcessing();
        
        // Setup and run the ambient occlusion material casting. 
        string ambientocclusionTextureFilePath;
        using Simplygon.spAmbientOcclusionCaster sgAmbientOcclusionCaster = sg.CreateAmbientOcclusionCaster();
        sgAmbientOcclusionCaster.SetMappingImage( sgAggregationProcessor.GetMappingImage() );
        sgAmbientOcclusionCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgAmbientOcclusionCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgAmbientOcclusionCaster.SetOutputFilePath( "AmbientOcclusionTexture" );

        using Simplygon.spAmbientOcclusionCasterSettings sgAmbientOcclusionCasterSettings = sgAmbientOcclusionCaster.GetAmbientOcclusionCasterSettings();
        sgAmbientOcclusionCasterSettings.SetMaterialChannel( "AmbientOcclusion" );
        sgAmbientOcclusionCasterSettings.SetRaysPerPixel( 64 );
        sgAmbientOcclusionCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );

        sgAmbientOcclusionCaster.RunProcessing();
        ambientocclusionTextureFilePath = sgAmbientOcclusionCaster.GetOutputFilePath();
        
        // Update scene with new casted texture. 
        using Simplygon.spMaterialTable sgMaterialTable = sg.CreateMaterialTable();
        using Simplygon.spTextureTable sgTextureTable = sg.CreateTextureTable();
        using Simplygon.spMaterial sgMaterial = sg.CreateMaterial();
        using Simplygon.spTexture sgAmbientOcclusionTexture = sg.CreateTexture();
        sgAmbientOcclusionTexture.SetName( "AmbientOcclusion" );
        sgAmbientOcclusionTexture.SetFilePath( ambientocclusionTextureFilePath );
        sgTextureTable.AddTexture( sgAmbientOcclusionTexture );

        using Simplygon.spShadingTextureNode sgAmbientOcclusionTextureShadingNode = sg.CreateShadingTextureNode();
        sgAmbientOcclusionTextureShadingNode.SetTexCoordLevel( 0 );
        sgAmbientOcclusionTextureShadingNode.SetTextureName( "AmbientOcclusion" );

        sgMaterial.AddMaterialChannel( "AmbientOcclusion" );
        sgMaterial.SetShadingNetwork( "AmbientOcclusion", sgAmbientOcclusionTextureShadingNode );

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
        AmbientOcclusionCasting(sg);

        return 0;
    }

}
