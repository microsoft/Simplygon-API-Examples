// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

#include <string>
#include <stdlib.h>
#include <filesystem>
#include <future>
#include "SimplygonLoader.h"

class CustomObserver : public Simplygon::Observer
{
	public:
	Simplygon::ShadingColor OnShadingCustomNodeEvaluate(Simplygon::spObject subject) override
	{
		Simplygon::ShadingColor outputValue( 0.0f, 0.0f, 0.0f, 1.0f );
		if (!subject.IsNull())
		{
			Simplygon::spShadingCustomNode customNode = Simplygon::spShadingCustomNode::SafeCast(subject);
			if (!customNode.IsNull())
			{
				Simplygon::ShadingColor inputValue = customNode->GetInputValue(0);
				outputValue.r = inputValue.r * 0.393f + inputValue.g * 0.769f + inputValue.b * 0.189f;
				outputValue.g = inputValue.r * 0.349f + inputValue.g * 0.686f + inputValue.b * 0.168f;
				outputValue.b = inputValue.r * 0.272f + inputValue.g * 0.534f + inputValue.b * 0.131f;
			}
		}
		return outputValue;
	}
	bool OnShadingCustomNodeGenerateShaderCode(Simplygon::spObject subject, Simplygon::EShaderLanguage shaderLanguage) override
	{
		if (!subject.IsNull())
		{
			Simplygon::spShadingCustomNode customNode = Simplygon::spShadingCustomNode::SafeCast(subject);
			if (!customNode.IsNull())
			{
				customNode->SetCustomShaderCode( "result = float4(dot(rgba_custom_input_0, float3(0.393f, 0.769f, 0.189f)), dot(rgba_custom_input_0, float3(0.349f, 0.686f, 0.168f)), dot(rgba_custom_input_0, float3(0.272f, 0.534f, 0.131f)), 1.0f);");
				return true;
			}
		}
		return false;
	}
} customObserver;


Simplygon::spScene LoadScene(Simplygon::ISimplygon* sg, const char* path)
{
	// Create scene importer 
	Simplygon::spSceneImporter sgSceneImporter = sg->CreateSceneImporter();
	sgSceneImporter->SetImportFilePath(path);
	
	// Run scene importer. 
	auto importResult = sgSceneImporter->Run();
	if (Simplygon::Failed(importResult))
	{
		throw std::exception("Failed to load scene.");
	}
	Simplygon::spScene sgScene = sgSceneImporter->GetScene();
	return sgScene;
}

void SaveScene(Simplygon::ISimplygon* sg, Simplygon::spScene sgScene, const char* path)
{
	// Create scene exporter. 
	Simplygon::spSceneExporter sgSceneExporter = sg->CreateSceneExporter();
	std::string outputScenePath = std::string("output\\") + std::string("ShadingNetworksWithCustomShadingNode") + std::string("_") + std::string(path);
	sgSceneExporter->SetExportFilePath(outputScenePath.c_str());
	sgSceneExporter->SetScene(sgScene);
	
	// Run scene exporter. 
	auto exportResult = sgSceneExporter->Run();
	if (Simplygon::Failed(exportResult))
	{
		throw std::exception("Failed to save scene.");
	}
}

void CheckLog(Simplygon::ISimplygon* sg)
{
	// Check if any errors occurred. 
	bool hasErrors = sg->ErrorOccurred();
	if (hasErrors)
	{
		Simplygon::spStringArray errors = sg->CreateStringArray();
		sg->GetErrorMessages(errors);
		auto errorCount = errors->GetItemCount();
		if (errorCount > 0)
		{
			printf("%s\n", "CheckLog: Errors:");
			for (auto errorIndex = 0U; errorIndex < errorCount; ++errorIndex)
			{
				Simplygon::spString errorString = errors->GetItem((int)errorIndex);
				printf("%s\n", errorString.c_str());
			}
			sg->ClearErrorMessages();
		}
	}
	else
	{
		printf("%s\n", "CheckLog: No errors.");
	}
	
	// Check if any warnings occurred. 
	bool hasWarnings = sg->WarningOccurred();
	if (hasWarnings)
	{
		Simplygon::spStringArray warnings = sg->CreateStringArray();
		sg->GetWarningMessages(warnings);
		auto warningCount = warnings->GetItemCount();
		if (warningCount > 0)
		{
			printf("%s\n", "CheckLog: Warnings:");
			for (auto warningIndex = 0U; warningIndex < warningCount; ++warningIndex)
			{
				Simplygon::spString warningString = warnings->GetItem((int)warningIndex);
				printf("%s\n", warningString.c_str());
			}
			sg->ClearWarningMessages();
		}
	}
	else
	{
		printf("%s\n", "CheckLog: No warnings.");
	}
	
	// Error out if Simplygon has errors. 
	if (hasErrors)
	{
		throw std::exception("Processing failed with an error");
	}
	
	// Error out if Simplygon has errors. 
	if (hasErrors)
	{
		throw std::exception("Processing failed with an error");
	}
}

void RunReductionWithShadingNetworks(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	Simplygon::spReductionProcessor sgReductionProcessor = sg->CreateReductionProcessor();
	sgReductionProcessor->SetScene( sgScene );
	Simplygon::spReductionSettings sgReductionSettings = sgReductionProcessor->GetReductionSettings();
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgReductionProcessor->GetMappingImageSettings();
	
	// Generates a mapping image which is used after the reduction to cast new materials to the new 
	// reduced object. 
	sgMappingImageSettings->SetGenerateMappingImage( true );
	sgMappingImageSettings->SetApplyNewMaterialIds( true );
	sgMappingImageSettings->SetGenerateTangents( true );
	sgMappingImageSettings->SetUseFullRetexturing( true );
	
	// Inject a sepia filter as a custom shading node into the shading network for the diffuse channel 
	// for each material in the scene. 
	int materialCount = (int)sgScene->GetMaterialTable()->GetMaterialsCount();
	for (int i = 0; i < materialCount; ++i)
	{
		Simplygon::spMaterial sgSepiaMaterial = sgScene->GetMaterialTable()->GetMaterial(i);

		Simplygon::spShadingNode sgMaterialShadingNode = sgSepiaMaterial->GetShadingNetwork("Diffuse");
		Simplygon::spShadingCustomNode sgSepiaNode = sg->CreateShadingCustomNode();

		// Add the custom observer to our custom shading node.
		sgSepiaNode->AddObserver(&customObserver);
		
		// Set the number of input slots to the custom node. In this case we only use the diffuse color from the loaded material as input.
		sgSepiaNode->SetInputCount(1);


		sgSepiaNode->SetInput(0, sgMaterialShadingNode);

		sgSepiaMaterial->SetShadingNetwork("Diffuse", sgSepiaNode);
	}
	
	// Start the reduction process. 	
	printf("%s\n", "Start the reduction process.");
	sgReductionProcessor->RunProcessing();
	
	// Setup and run the diffuse material casting. 	
	printf("%s\n", "Setup and run the diffuse material casting.");
	Simplygon::spColorCaster sgDiffuseCaster = sg->CreateColorCaster();
	sgDiffuseCaster->SetMappingImage( sgReductionProcessor->GetMappingImage() );
	sgDiffuseCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgDiffuseCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgDiffuseCaster->SetOutputFilePath( "DiffuseTexture" );

	Simplygon::spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster->GetColorCasterSettings();
	sgDiffuseCasterSettings->SetMaterialChannel( "Diffuse" );
	sgDiffuseCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );

	sgDiffuseCaster->RunProcessing();
	std::string diffuseTextureFilePath = sgDiffuseCaster->GetOutputFilePath().c_str();
	
	// Update scene with new casted texture. 
	Simplygon::spMaterialTable sgMaterialTable = sg->CreateMaterialTable();
	Simplygon::spTextureTable sgTextureTable = sg->CreateTextureTable();
	Simplygon::spMaterial sgMaterial = sg->CreateMaterial();
	sgMaterial->SetName("OutputMaterial");
	Simplygon::spTexture sgDiffuseTexture = sg->CreateTexture();
	sgDiffuseTexture->SetName( "Diffuse" );
	sgDiffuseTexture->SetFilePath( diffuseTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgDiffuseTexture );

	Simplygon::spShadingTextureNode sgDiffuseTextureShadingNode = sg->CreateShadingTextureNode();
	sgDiffuseTextureShadingNode->SetTexCoordLevel( 0 );
	sgDiffuseTextureShadingNode->SetTextureName( "Diffuse" );

	sgMaterial->AddMaterialChannel( "Diffuse" );
	sgMaterial->SetShadingNetwork( "Diffuse", sgDiffuseTextureShadingNode );

	sgMaterialTable->AddMaterial( sgMaterial );

	sgScene->GetTextureTable()->Clear();
	sgScene->GetMaterialTable()->Clear();
	sgScene->GetTextureTable()->Copy(sgTextureTable);
	sgScene->GetMaterialTable()->Copy(sgMaterialTable);
	
	// Save processed scene. 	
	printf("%s\n", "Save processed scene.");
	SaveScene(sg, sgScene, "Output.fbx");
	
	// Check log for any warnings or errors. 	
	printf("%s\n", "Check log for any warnings or errors.");
	CheckLog(sg);
}

int main()
{
	Simplygon::ISimplygon* sg = NULL;
	Simplygon::EErrorCodes initval = Simplygon::Initialize( &sg );
	if( initval != Simplygon::EErrorCodes::NoError )
	{
		printf( "Failed to initialize Simplygon: ErrorCode(%d)", (int)initval );
		return int(initval);
	}

	RunReductionWithShadingNetworks(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

