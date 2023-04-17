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
        string outputScenePath = string.Join("", new string[] { "output\\", "ComputeCastingMaterialEvaluationShaderXML", "_", path });
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

    static void SaveMaterialEvaluationShaderXML(Simplygon.ISimplygon sg, string xmlFilePath)
    {
        // Create an evaluation shader, and save to xml. 
        using Simplygon.spMaterialEvaluationShader sgMaterialEvaluationShader = sg.CreateMaterialEvaluationShader();
        using Simplygon.spShaderEvaluationFunctionTable sgShaderEvaluationFunctionTable = sgMaterialEvaluationShader.GetShaderEvaluationFunctionTable();
        using Simplygon.spMaterialEvaluationShaderAttributeTable sgMaterialEvaluationShaderAttributeTable = sgMaterialEvaluationShader.GetMaterialEvaluationShaderAttributeTable();
        using Simplygon.spMaterialEvaluationShaderDefineTable sgMaterialEvaluationShaderDefineTable = sgMaterialEvaluationShader.GetMaterialEvaluationShaderDefineTable();
        
        // Create an evaluation function, for channel 'Diffuse', add to the shader. 
        using Simplygon.spShaderEvaluationFunction sgShaderEvaluationFunction = sg.CreateShaderEvaluationFunction();
        sgShaderEvaluationFunction.SetName( "Diffuse" );
        sgShaderEvaluationFunction.SetChannel( "Diffuse" );
        sgShaderEvaluationFunction.SetEntryPoint( "Diffuse" );
        sgShaderEvaluationFunctionTable.AddShaderEvaluationFunction(sgShaderEvaluationFunction);
        
        // Set up a needed vertex attribute from the source geometry: 'TexCoords', to read from. 
        using Simplygon.spMaterialEvaluationShaderAttribute sgMaterialEvaluationShaderAttribute = sg.CreateMaterialEvaluationShaderAttribute();
        sgMaterialEvaluationShaderAttribute.SetName( "TexCoord" );
        sgMaterialEvaluationShaderAttribute.SetFieldType( Simplygon.EGeometryDataFieldType.TexCoords );
        sgMaterialEvaluationShaderAttribute.SetFieldFormat( Simplygon.EAttributeFormat.F32vec2 );
        sgMaterialEvaluationShaderAttributeTable.AddAttribute(sgMaterialEvaluationShaderAttribute);
        
        // Set the shader code to run. This example shader code returns the texture coords as red and green 
        // channels. Use GLSL shader language. 
        string shaderCode = @"
vec4 Diffuse()
{
	#ifdef FLIP_UV
		return vec4(TexCoord.y,TexCoord.x,0,1);
#else
		return vec4(TexCoord.x,TexCoord.y,0,1);
#endif
}
";
        sgMaterialEvaluationShader.SetShaderCode(shaderCode);
        sgMaterialEvaluationShader.SetShaderLanguage(Simplygon.EShaderLanguage.GLSL);
        
        // Set up a shader define to flip uvs. 
        using Simplygon.spMaterialEvaluationShaderDefine sgMaterialEvaluationShaderDefine = sg.CreateMaterialEvaluationShaderDefine();
        sgMaterialEvaluationShaderDefine.SetName( "FLIP_UV" );
        sgMaterialEvaluationShaderDefineTable.AddMaterialEvaluationShaderDefine(sgMaterialEvaluationShaderDefine);
        
        // Save XML. 
        using Simplygon.spMaterialEvaluationShaderSerializer sgMaterialEvaluationShaderSerializer = sg.CreateMaterialEvaluationShaderSerializer();
        sgMaterialEvaluationShaderSerializer.SaveMaterialEvaluationShaderToFile(xmlFilePath, sgMaterialEvaluationShader);
    }

    static void SetupCastingCodeInScene(Simplygon.ISimplygon sg, Simplygon.spScene sgScene)
    {
        // Get the material table from the scene 
        using Simplygon.spMaterialTable sgMaterialTable = sgScene.GetMaterialTable();
        
        // Enumerate all materials, and assign a custom shader to the Diffuse channel 
        var materialsCount = sgMaterialTable.GetMaterialsCount();
        
        // Setup temporary path to XML file. 
        string xmlFilePath = @"./materialEvaluationShader.xml";
        SaveMaterialEvaluationShaderXML(sg, xmlFilePath);
        for (uint materialIndex = 0; materialIndex < materialsCount; ++materialIndex)
        {
            var material = sgMaterialTable.GetMaterial((int)materialIndex);
            
            // Save XML. 
            using Simplygon.spMaterialEvaluationShaderSerializer sgMaterialEvaluationShaderSerializer = sg.CreateMaterialEvaluationShaderSerializer();
            Simplygon.spMaterialEvaluationShader sgMaterialEvaluationShader = sgMaterialEvaluationShaderSerializer.LoadMaterialEvaluationShaderFromFile(xmlFilePath);
            material.SetMaterialEvaluationShader(sgMaterialEvaluationShader);
        }
    }

    static void ComputeCasting(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
        
        // Add additional scene setup for the casting. 
        SetupCastingCodeInScene(sg, sgScene);
        
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
        
        // Setup and run the compute shader material casting as a custom output to the diffuse channel. 
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
        
        // Update scene with new casted texture. 
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
        ComputeCasting(sg);

        return 0;
    }

}
