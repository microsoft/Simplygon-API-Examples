// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

#include <string>
#include <stdlib.h>
#include <filesystem>
#include <future>
#include "SimplygonLoader.h"


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
	std::string outputScenePath = std::string("output\\") + std::string("ComputeCasting") + std::string("_") + std::string(path);
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
			printf("%s\n", "Errors:");
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
		printf("%s\n", "No errors.");
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
			printf("%s\n", "Warnings:");
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
		printf("%s\n", "No warnings.");
	}
}

void SetupCastingCodeInMaterial(Simplygon::ISimplygon* sg, Simplygon::spMaterial sgMaterial)
{
	// Create an evaluation shader, and add to the material. 
	Simplygon::spMaterialEvaluationShader sgMaterialEvaluationShader = sg->CreateMaterialEvaluationShader();
	sgMaterial->SetMaterialEvaluationShader( sgMaterialEvaluationShader );
	Simplygon::spShaderEvaluationFunctionTable sgShaderEvaluationFunctionTable = sgMaterialEvaluationShader->GetShaderEvaluationFunctionTable();
	Simplygon::spMaterialEvaluationShaderAttributeTable sgMaterialEvaluationShaderAttributeTable = sgMaterialEvaluationShader->GetMaterialEvaluationShaderAttributeTable();
	
	// Create an evaluation function, for channel 'Diffuse', add to the shader. 
	Simplygon::spShaderEvaluationFunction sgShaderEvaluationFunction = sg->CreateShaderEvaluationFunction();
	sgShaderEvaluationFunction->SetName( "Diffuse" );
	sgShaderEvaluationFunction->SetChannel( "Diffuse" );
	sgShaderEvaluationFunction->SetEntryPoint( "Diffuse" );
	sgShaderEvaluationFunctionTable->AddShaderEvaluationFunction(sgShaderEvaluationFunction);
	
	// Set up a needed vertex attribute from the source geometry: 'TexCoords', to read from. 
	Simplygon::spMaterialEvaluationShaderAttribute sgMaterialEvaluationShaderAttribute = sg->CreateMaterialEvaluationShaderAttribute();
	sgMaterialEvaluationShaderAttribute->SetName( "TexCoord" );
	sgMaterialEvaluationShaderAttribute->SetFieldType( Simplygon::EGeometryDataFieldType::TexCoords );
	sgMaterialEvaluationShaderAttribute->SetFieldFormat( Simplygon::EAttributeFormat::F32vec2 );
	sgMaterialEvaluationShaderAttributeTable->AddAttribute(sgMaterialEvaluationShaderAttribute);
	
	// Set the shader code to run. This example shader code returns the texture coords as red and green 
	// channels. Use GLSL shader language. 
	const char *shaderCode = R"(
vec4 Diffuse()
{
	return vec4(TexCoord.x,TexCoord.y,0,1);
}
)";
	sgMaterialEvaluationShader->SetShaderCode(shaderCode);
	sgMaterialEvaluationShader->SetShaderLanguage(Simplygon::EShaderLanguage::GLSL);
}

void SetupCastingCodeInScene(Simplygon::ISimplygon* sg, Simplygon::spScene sgScene)
{
	// Get the material table from the scene 
	Simplygon::spMaterialTable sgMaterialTable = sgScene->GetMaterialTable();
	
	// Enumerate all materials, and assign a custom shader to the Diffuse channel 
	auto materialsCount = sgMaterialTable->GetMaterialsCount();
	for (auto materialIndex = 0U; materialIndex < materialsCount; ++materialIndex)
	{
		auto material = sgMaterialTable->GetMaterial((int)materialIndex);
		SetupCastingCodeInMaterial(sg, material);
	}
}

void ComputeCasting(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	
	// Add additional scene setup for the casting. 
	SetupCastingCodeInScene(sg, sgScene);
	
	// Create the aggregation processor. 
	Simplygon::spAggregationProcessor sgAggregationProcessor = sg->CreateAggregationProcessor();
	sgAggregationProcessor->SetScene( sgScene );
	Simplygon::spAggregationSettings sgAggregationSettings = sgAggregationProcessor->GetAggregationSettings();
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgAggregationProcessor->GetMappingImageSettings();
	
	// Merge all geometries into a single geometry. 
	sgAggregationSettings->SetMergeGeometries( true );
	
	// Generates a mapping image which is used after the aggregation to cast new materials to the new 
	// aggregated object. 
	sgMappingImageSettings->SetGenerateMappingImage( true );
	sgMappingImageSettings->SetApplyNewMaterialIds( true );
	sgMappingImageSettings->SetGenerateTangents( true );
	sgMappingImageSettings->SetUseFullRetexturing( true );
	Simplygon::spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings->GetOutputMaterialSettings(0);
	
	// Setting the size of the output material for the mapping image. This will be the output size of the 
	// textures when we do material casting in a later stage. 
	sgOutputMaterialSettings->SetTextureWidth( 2048 );
	sgOutputMaterialSettings->SetTextureHeight( 2048 );
	
	// Start the aggregation process. 	
	printf("%s\n", "Start the aggregation process.");
	sgAggregationProcessor->RunProcessing();
	
	// Setup and run the compute shader material casting as a custom output to the diffuse channel. 
	Simplygon::spComputeCaster sgDiffuseCaster = sg->CreateComputeCaster();
	sgDiffuseCaster->SetMappingImage( sgAggregationProcessor->GetMappingImage() );
	sgDiffuseCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgDiffuseCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgDiffuseCaster->SetOutputFilePath( "DiffuseTexture" );

	Simplygon::spComputeCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster->GetComputeCasterSettings();
	sgDiffuseCasterSettings->SetMaterialChannel( "Diffuse" );
	sgDiffuseCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );

	sgDiffuseCaster->RunProcessing();
	std::string diffuseTextureFilePath = sgDiffuseCaster->GetOutputFilePath().c_str();
	
	// Update scene with new casted texture. 
	Simplygon::spMaterialTable sgMaterialTable = sg->CreateMaterialTable();
	Simplygon::spTextureTable sgTextureTable = sg->CreateTextureTable();
	Simplygon::spMaterial sgMaterial = sg->CreateMaterial();
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

	ComputeCasting(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

