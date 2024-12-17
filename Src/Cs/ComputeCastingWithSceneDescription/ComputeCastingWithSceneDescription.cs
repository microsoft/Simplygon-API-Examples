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
        string outputScenePath = string.Join("", new string[] { "output\\", "ComputeCastingWithSceneDescription", "_", path });
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

    static void ComputeCasting(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
        
        // Add tangents to the scene. Tangents are needed to cast the tangent-space normal maps, and the 
        // input scene is missing tangents. 
        using Simplygon.spTangentCalculator sgTangentCalculator = sg.CreateTangentCalculator();
        sgTangentCalculator.CalculateTangentsOnScene(sgScene, true);
        
        // Load scene material evaluation description, which will define all evaluation shaders for all 
        // materials, as well as needed textures and custom buffers.         
        Console.WriteLine("Load scene material evaluation description, which will define all evaluation shaders for all materials, as well as needed textures and custom buffers.");
        using Simplygon.spMaterialEvaluationShaderSerializer sgMaterialEvaluationShaderSerializer = sg.CreateMaterialEvaluationShaderSerializer();
        sgMaterialEvaluationShaderSerializer.LoadSceneMaterialEvaluationShadersFromFile("SimplygonManCasting.xml", sgScene);
        
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
        
        // Setup and run the compute caster on the diffuse channel.         
        Console.WriteLine("Setup and run the compute caster on the diffuse channel.");
        string diffuseTextureFilePath;
        using Simplygon.spComputeCaster sgDiffuseCaster = sg.CreateComputeCaster();
        sgDiffuseCaster.SetMappingImage( sgAggregationProcessor.GetMappingImage() );
        sgDiffuseCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgDiffuseCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgDiffuseCaster.SetOutputFilePath( "DiffuseTexture" );

        using Simplygon.spComputeCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster.GetComputeCasterSettings();
        sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" );
        sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );

        sgDiffuseCaster.RunProcessing();
        diffuseTextureFilePath = sgDiffuseCaster.GetOutputFilePath();
        
        // Setup and run another compute shader on the normals channel.         
        Console.WriteLine("Setup and run another compute shader on the normals channel.");
        string normalsTextureFilePath;
        using Simplygon.spComputeCaster sgNormalsCaster = sg.CreateComputeCaster();
        sgNormalsCaster.SetMappingImage( sgAggregationProcessor.GetMappingImage() );
        sgNormalsCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgNormalsCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgNormalsCaster.SetOutputFilePath( "NormalsTexture" );

        using Simplygon.spComputeCasterSettings sgNormalsCasterSettings = sgNormalsCaster.GetComputeCasterSettings();
        sgNormalsCasterSettings.SetMaterialChannel( "Normals" );
        sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );

        sgNormalsCaster.RunProcessing();
        normalsTextureFilePath = sgNormalsCaster.GetOutputFilePath();
        
        // Update scene with new casted texture. 
        using Simplygon.spMaterialTable sgMaterialTable = sg.CreateMaterialTable();
        using Simplygon.spTextureTable sgTextureTable = sg.CreateTextureTable();
        using Simplygon.spMaterial sgMaterial = sg.CreateMaterial();
        sgMaterial.SetName("OutputMaterial");
        using Simplygon.spTexture sgDiffuseTexture = sg.CreateTexture();
        sgDiffuseTexture.SetName( "Diffuse" );
        sgDiffuseTexture.SetFilePath( diffuseTextureFilePath );
        sgTextureTable.AddTexture( sgDiffuseTexture );

        using Simplygon.spShadingTextureNode sgDiffuseTextureShadingNode = sg.CreateShadingTextureNode();
        sgDiffuseTextureShadingNode.SetTexCoordLevel( 0 );
        sgDiffuseTextureShadingNode.SetTextureName( "Diffuse" );

        sgMaterial.AddMaterialChannel( "Diffuse" );
        sgMaterial.SetShadingNetwork( "Diffuse", sgDiffuseTextureShadingNode );
        using Simplygon.spTexture sgNormalsTexture = sg.CreateTexture();
        sgNormalsTexture.SetName( "Normals" );
        sgNormalsTexture.SetFilePath( normalsTextureFilePath );
        sgTextureTable.AddTexture( sgNormalsTexture );

        using Simplygon.spShadingTextureNode sgNormalsTextureShadingNode = sg.CreateShadingTextureNode();
        sgNormalsTextureShadingNode.SetTexCoordLevel( 0 );
        sgNormalsTextureShadingNode.SetTextureName( "Normals" );

        sgMaterial.AddMaterialChannel( "Normals" );
        sgMaterial.SetShadingNetwork( "Normals", sgNormalsTextureShadingNode );

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
        ComputeCasting(sg);

        return 0;
    }

}
