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
        string outputScenePath = string.Join("", new string[] { "output\\", "ImpostorBillboardCloudOuterShell", "_", path });
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

    static void RunBillboardCloudOuterShell(Simplygon.ISimplygon sg)
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
        using Simplygon.spImpostorProcessor sgImpostorProcessor = sg.CreateImpostorProcessor();
        sgImpostorProcessor.SetScene( sgScene );
        using Simplygon.spImpostorSettings sgImpostorSettings = sgImpostorProcessor.GetImpostorSettings();
        
        // Set impostor type to Billboard cloud. 
        sgImpostorSettings.SetImpostorType( Simplygon.EImpostorType.BillboardCloud );
        using Simplygon.spBillboardCloudSettings sgBillboardCloudSettings = sgImpostorSettings.GetBillboardCloudSettings();
        
        // Set billboard cloud mode to OuterShell. 
        sgBillboardCloudSettings.SetBillboardMode( Simplygon.EBillboardMode.OuterShell );
        sgBillboardCloudSettings.SetBillboardDensity( 0.5f );
        sgBillboardCloudSettings.SetGeometricComplexity( 0.9f );
        sgBillboardCloudSettings.SetMaxPlaneCount( 20 );
        sgBillboardCloudSettings.SetTwoSided( false );
        using Simplygon.spMappingImageSettings sgMappingImageSettings = sgImpostorProcessor.GetMappingImageSettings();
        using Simplygon.spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0);
        
        // Setting the size of the output material for the mapping image. This will be the output size of the 
        // textures when we do material casting in a later stage. 
        sgOutputMaterialSettings.SetTextureWidth( 1024 );
        sgOutputMaterialSettings.SetTextureHeight( 1024 );
        sgOutputMaterialSettings.SetMultisamplingLevel( 2 );
        
        // Start the impostor process.         
        Console.WriteLine("Start the impostor process.");
        sgImpostorProcessor.RunProcessing();
        
        // Setup and run the diffuse material casting.         
        Console.WriteLine("Setup and run the diffuse material casting.");
        string diffuseTextureFilePath;
        using Simplygon.spColorCaster sgDiffuseCaster = sg.CreateColorCaster();
        sgDiffuseCaster.SetMappingImage( sgImpostorProcessor.GetMappingImage() );
        sgDiffuseCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgDiffuseCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgDiffuseCaster.SetOutputFilePath( "DiffuseTexture" );

        using Simplygon.spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings();
        sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" );
        sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgDiffuseCasterSettings.SetBakeOpacityInAlpha( false );
        sgDiffuseCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat.R8G8B8 );
        sgDiffuseCasterSettings.SetDilation( 10 );
        sgDiffuseCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.Interpolate );

        sgDiffuseCaster.RunProcessing();
        diffuseTextureFilePath = sgDiffuseCaster.GetOutputFilePath();
        
        // Setup and run the specular material casting.         
        Console.WriteLine("Setup and run the specular material casting.");
        string specularTextureFilePath;
        using Simplygon.spColorCaster sgSpecularCaster = sg.CreateColorCaster();
        sgSpecularCaster.SetMappingImage( sgImpostorProcessor.GetMappingImage() );
        sgSpecularCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgSpecularCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgSpecularCaster.SetOutputFilePath( "SpecularTexture" );

        using Simplygon.spColorCasterSettings sgSpecularCasterSettings = sgSpecularCaster.GetColorCasterSettings();
        sgSpecularCasterSettings.SetMaterialChannel( "Specular" );
        sgSpecularCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgSpecularCasterSettings.SetDilation( 10 );
        sgSpecularCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.Interpolate );

        sgSpecularCaster.RunProcessing();
        specularTextureFilePath = sgSpecularCaster.GetOutputFilePath();
        
        // Setup and run the normals material casting.         
        Console.WriteLine("Setup and run the normals material casting.");
        string normalsTextureFilePath;
        using Simplygon.spNormalCaster sgNormalsCaster = sg.CreateNormalCaster();
        sgNormalsCaster.SetMappingImage( sgImpostorProcessor.GetMappingImage() );
        sgNormalsCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgNormalsCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgNormalsCaster.SetOutputFilePath( "NormalsTexture" );

        using Simplygon.spNormalCasterSettings sgNormalsCasterSettings = sgNormalsCaster.GetNormalCasterSettings();
        sgNormalsCasterSettings.SetMaterialChannel( "Normals" );
        sgNormalsCasterSettings.SetGenerateTangentSpaceNormals( true );
        sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgNormalsCasterSettings.SetDilation( 10 );
        sgNormalsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.Interpolate );

        sgNormalsCaster.RunProcessing();
        normalsTextureFilePath = sgNormalsCaster.GetOutputFilePath();
        
        // Setup and run the opacity material casting. Make sure there is no dilation or fill.         
        Console.WriteLine("Setup and run the opacity material casting. Make sure there is no dilation or fill.");
        string opacityTextureFilePath;
        using Simplygon.spOpacityCaster sgOpacityCaster = sg.CreateOpacityCaster();
        sgOpacityCaster.SetMappingImage( sgImpostorProcessor.GetMappingImage() );
        sgOpacityCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgOpacityCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgOpacityCaster.SetOutputFilePath( "OpacityTexture" );

        using Simplygon.spOpacityCasterSettings sgOpacityCasterSettings = sgOpacityCaster.GetOpacityCasterSettings();
        sgOpacityCasterSettings.SetMaterialChannel( "Opacity" );
        sgOpacityCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgOpacityCasterSettings.SetDilation( 0 );
        sgOpacityCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.NoFill );
        sgOpacityCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat.R8 );

        sgOpacityCaster.RunProcessing();
        opacityTextureFilePath = sgOpacityCaster.GetOutputFilePath();
        
        // Update scene with new casted textures. 
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
        using Simplygon.spTexture sgSpecularTexture = sg.CreateTexture();
        sgSpecularTexture.SetName( "Specular" );
        sgSpecularTexture.SetFilePath( specularTextureFilePath );
        sgTextureTable.AddTexture( sgSpecularTexture );

        using Simplygon.spShadingTextureNode sgSpecularTextureShadingNode = sg.CreateShadingTextureNode();
        sgSpecularTextureShadingNode.SetTexCoordLevel( 0 );
        sgSpecularTextureShadingNode.SetTextureName( "Specular" );

        sgMaterial.AddMaterialChannel( "Specular" );
        sgMaterial.SetShadingNetwork( "Specular", sgSpecularTextureShadingNode );
        using Simplygon.spTexture sgNormalsTexture = sg.CreateTexture();
        sgNormalsTexture.SetName( "Normals" );
        sgNormalsTexture.SetFilePath( normalsTextureFilePath );
        sgTextureTable.AddTexture( sgNormalsTexture );

        using Simplygon.spShadingTextureNode sgNormalsTextureShadingNode = sg.CreateShadingTextureNode();
        sgNormalsTextureShadingNode.SetTexCoordLevel( 0 );
        sgNormalsTextureShadingNode.SetTextureName( "Normals" );

        sgMaterial.AddMaterialChannel( "Normals" );
        sgMaterial.SetShadingNetwork( "Normals", sgNormalsTextureShadingNode );
        using Simplygon.spTexture sgOpacityTexture = sg.CreateTexture();
        sgOpacityTexture.SetName( "Opacity" );
        sgOpacityTexture.SetFilePath( opacityTextureFilePath );
        sgTextureTable.AddTexture( sgOpacityTexture );

        using Simplygon.spShadingTextureNode sgOpacityTextureShadingNode = sg.CreateShadingTextureNode();
        sgOpacityTextureShadingNode.SetTexCoordLevel( 0 );
        sgOpacityTextureShadingNode.SetTextureName( "Opacity" );

        sgMaterial.AddMaterialChannel( "Opacity" );
        sgMaterial.SetShadingNetwork( "Opacity", sgOpacityTextureShadingNode );
        sgMaterial.SetBlendMode(Simplygon.EMaterialBlendMode.Blend);

        sgMaterialTable.AddMaterial( sgMaterial );

        sgScene.GetTextureTable().Clear();
        sgScene.GetMaterialTable().Clear();
        sgScene.GetTextureTable().Copy(sgTextureTable);
        sgScene.GetMaterialTable().Copy(sgMaterialTable);
        
        // Save processed scene.         
        Console.WriteLine("Save processed scene.");
        SaveScene(sg, sgScene, "Output.glb");
        
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
        RunBillboardCloudOuterShell(sg);

        return 0;
    }

}
