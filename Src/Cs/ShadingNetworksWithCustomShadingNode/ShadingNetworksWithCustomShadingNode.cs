// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

using System;
using System.IO;
using System.Threading.Tasks;

public class CustomObserver : Simplygon.Observer
{
    public override Simplygon.ShadingColor OnShadingCustomNodeEvaluate(Simplygon.spObject subject)
    {
        Simplygon.ShadingColor outputValue = new Simplygon.ShadingColor( 0.0f, 0.0f, 0.0f, 1.0f );
        if (!subject.IsNull())
        {
            Simplygon.spShadingCustomNode customNode = Simplygon.spShadingCustomNode.SafeCast(subject);
            if (!customNode.IsNull())
            {
                Simplygon.ShadingColor inputValue = customNode.GetInputValue(0);
                outputValue.r = inputValue.r * 0.393f + inputValue.g * 0.769f + inputValue.b * 0.189f;
                outputValue.g = inputValue.r * 0.349f + inputValue.g * 0.686f + inputValue.b * 0.168f;
                outputValue.b = inputValue.r * 0.272f + inputValue.g * 0.534f + inputValue.b * 0.131f;
            }
        }
        return outputValue;
    }
    public override bool OnShadingCustomNodeGenerateShaderCode(Simplygon.spObject subject, Simplygon.EShaderLanguage shaderLanguage)
    {
        if (!subject.IsNull())
        {
            Simplygon.spShadingCustomNode customNode = Simplygon.spShadingCustomNode.SafeCast(subject);
            if (!customNode.IsNull())
            {
                customNode.SetCustomShaderCode( "result = float4(dot(rgba_custom_input_0, float3(0.393f, 0.769f, 0.189f)), dot(rgba_custom_input_0, float3(0.349f, 0.686f, 0.168f)), dot(rgba_custom_input_0, float3(0.272f, 0.534f, 0.131f)), 1.0f);");
                return true;
            }
        }
        return false;
    }
}

public class Program
{
    static private CustomObserver customObserver;

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
        string outputScenePath = string.Join("", new string[] { "output\\", "ShadingNetworksWithCustomShadingNode", "_", path });
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

    static void RunReductionWithShadingNetworks(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
        using Simplygon.spReductionProcessor sgReductionProcessor = sg.CreateReductionProcessor();
        sgReductionProcessor.SetScene( sgScene );
        using Simplygon.spReductionSettings sgReductionSettings = sgReductionProcessor.GetReductionSettings();
        using Simplygon.spMappingImageSettings sgMappingImageSettings = sgReductionProcessor.GetMappingImageSettings();
        
        // Generates a mapping image which is used after the reduction to cast new materials to the new 
        // reduced object. 
        sgMappingImageSettings.SetGenerateMappingImage( true );
        sgMappingImageSettings.SetApplyNewMaterialIds( true );
        sgMappingImageSettings.SetGenerateTangents( true );
        sgMappingImageSettings.SetUseFullRetexturing( true );
        
        // Inject a sepia filter as a custom shading node into the shading network for the diffuse channel 
        // for each material in the scene. 
        int materialCount = (int)sgScene.GetMaterialTable().GetMaterialsCount();
        for (int i = 0; i < materialCount; ++i)
        {
            Simplygon.spMaterial sgSepiaMaterial = sgScene.GetMaterialTable().GetMaterial(i);

            using Simplygon.spShadingNode sgMaterialShadingNode = sgSepiaMaterial.GetShadingNetwork("Diffuse");
            using Simplygon.spShadingCustomNode sgSepiaNode = sg.CreateShadingCustomNode();
            // Add the custom observer to our custom shading node.
            sgSepiaNode.AddObserver(customObserver);
            
            // Set the number of input slots to the custom node. In this case we only use the diffuse color from the loaded material as input.
            sgSepiaNode.SetInputCount(1);


            sgSepiaNode.SetInput(0, sgMaterialShadingNode);

            sgSepiaMaterial.SetShadingNetwork("Diffuse", sgSepiaNode);
        }
        
        // Start the reduction process.         
        Console.WriteLine("Start the reduction process.");
        sgReductionProcessor.RunProcessing();
        
        // Setup and run the diffuse material casting.         
        Console.WriteLine("Setup and run the diffuse material casting.");
        string diffuseTextureFilePath;
        using Simplygon.spColorCaster sgDiffuseCaster = sg.CreateColorCaster();
        sgDiffuseCaster.SetMappingImage( sgReductionProcessor.GetMappingImage() );
        sgDiffuseCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgDiffuseCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgDiffuseCaster.SetOutputFilePath( "DiffuseTexture" );

        using Simplygon.spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings();
        sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" );
        sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );

        sgDiffuseCaster.RunProcessing();
        diffuseTextureFilePath = sgDiffuseCaster.GetOutputFilePath();
        
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
        customObserver = new CustomObserver();
        RunReductionWithShadingNetworks(sg);

        return 0;
    }

}
