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
	std::string outputScenePath = std::string("output\\") + std::string("ShadingNetworks") + std::string("_") + std::string(path);
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
	
	// Inject a sepia filter into the shading network for the diffuse channel for each material in the 
	// scene. 
	int materialCount = (int)sgScene->GetMaterialTable()->GetMaterialsCount();
	for (int i = 0; i < materialCount; ++i)
	{
		Simplygon::spMaterial sgSepiaMaterial = sgScene->GetMaterialTable()->GetMaterial(i);

		Simplygon::spShadingNode sgMaterialShadingNode = sgSepiaMaterial->GetShadingNetwork("Diffuse");
		Simplygon::spShadingColorNode sgSepiaColor1 = sg->CreateShadingColorNode();
		Simplygon::spShadingColorNode sgSepiaColor2 = sg->CreateShadingColorNode();
		Simplygon::spShadingColorNode sgSepiaColor3 = sg->CreateShadingColorNode();
		Simplygon::spShadingColorNode sgRedFilter = sg->CreateShadingColorNode();
		Simplygon::spShadingColorNode sgGreenFilter = sg->CreateShadingColorNode();
		Simplygon::spShadingColorNode sgBlueFilter = sg->CreateShadingColorNode();
		Simplygon::spShadingDot3Node sgSepiaDot1 = sg->CreateShadingDot3Node();
		Simplygon::spShadingDot3Node sgSepiaDot2 = sg->CreateShadingDot3Node();
		Simplygon::spShadingDot3Node sgSepiaDot3 = sg->CreateShadingDot3Node();
		Simplygon::spShadingMultiplyNode sgSepiaMul1 = sg->CreateShadingMultiplyNode();
		Simplygon::spShadingMultiplyNode sgSepiaMul2 = sg->CreateShadingMultiplyNode();
		Simplygon::spShadingMultiplyNode sgSepiaMul3 = sg->CreateShadingMultiplyNode();
		Simplygon::spShadingAddNode sgSepiaAdd1 = sg->CreateShadingAddNode();
		Simplygon::spShadingAddNode sgSepiaAdd2 = sg->CreateShadingAddNode();

		sgSepiaColor1->SetColor(0.393f, 0.769f, 0.189f, 1.0f);
		sgSepiaColor2->SetColor(0.349f, 0.686f, 0.168f, 1.0f);
		sgSepiaColor3->SetColor(0.272f, 0.534f, 0.131f, 1.0f);
		sgRedFilter->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		sgGreenFilter->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
		sgBlueFilter->SetColor(0.0f, 0.0f, 1.0f, 1.0f);

		sgSepiaDot1->SetInput(0, sgSepiaColor1);
		sgSepiaDot1->SetInput(1, sgMaterialShadingNode);
		sgSepiaDot2->SetInput(0, sgSepiaColor2);
		sgSepiaDot2->SetInput(1, sgMaterialShadingNode);
		sgSepiaDot3->SetInput(0, sgSepiaColor3);
		sgSepiaDot3->SetInput(1, sgMaterialShadingNode);
		sgSepiaMul1->SetInput(0, sgSepiaDot1);
		sgSepiaMul1->SetInput(1, sgRedFilter);
		sgSepiaMul2->SetInput(0, sgSepiaDot2);
		sgSepiaMul2->SetInput(1, sgGreenFilter);
		sgSepiaMul3->SetInput(0, sgSepiaDot3);
		sgSepiaMul3->SetInput(1, sgBlueFilter);
		sgSepiaAdd1->SetInput(0, sgSepiaMul1);
		sgSepiaAdd1->SetInput(1, sgSepiaMul2);
		sgSepiaAdd2->SetInput(0, sgSepiaAdd1);
		sgSepiaAdd2->SetInput(1, sgSepiaMul3);

		sgSepiaMaterial->SetShadingNetwork("Diffuse", sgSepiaAdd2);
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

