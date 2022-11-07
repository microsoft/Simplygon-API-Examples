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
	std::string outputScenePath = std::string("output\\") + std::string("OpacityCasting") + std::string("_") + std::string(path);
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

void OpacityCasting(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/Console/Console.obj");
	
	// Create the reduction processor. 
	Simplygon::spReductionProcessor sgReductionProcessor = sg->CreateReductionProcessor();
	sgReductionProcessor->SetScene( sgScene );
	Simplygon::spReductionSettings sgReductionSettings = sgReductionProcessor->GetReductionSettings();
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgReductionProcessor->GetMappingImageSettings();
	
	// Set reduction target to triangle ratio with a ratio of 50%. 
	sgReductionSettings->SetReductionTargets( Simplygon::EStopCondition::All, true, false, false, false );
	sgReductionSettings->SetReductionTargetTriangleRatio( 0.5f );
	
	// Generates a mapping image which is used after the reduction to cast new materials to the new 
	// reduced object. 
	sgMappingImageSettings->SetGenerateMappingImage( true );
	sgMappingImageSettings->SetApplyNewMaterialIds( true );
	sgMappingImageSettings->SetGenerateTangents( true );
	sgMappingImageSettings->SetUseFullRetexturing( true );
	Simplygon::spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings->GetOutputMaterialSettings(0);
	
	// Setting the size of the output material for the mapping image. This will be the output size of the 
	// textures when we do material casting in a later stage. 
	sgOutputMaterialSettings->SetTextureWidth( 2048 );
	sgOutputMaterialSettings->SetTextureHeight( 2048 );
	
	// Start the reduction process. 	
	printf("%s\n", "Start the reduction process.");
	sgReductionProcessor->RunProcessing();
	
	// Setup and run the opacity material casting. 
	Simplygon::spOpacityCaster sgOpacityCaster = sg->CreateOpacityCaster();
	sgOpacityCaster->SetMappingImage( sgReductionProcessor->GetMappingImage() );
	sgOpacityCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgOpacityCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgOpacityCaster->SetOutputFilePath( "OpacityTexture" );

	Simplygon::spOpacityCasterSettings sgOpacityCasterSettings = sgOpacityCaster->GetOpacityCasterSettings();
	sgOpacityCasterSettings->SetMaterialChannel( "Opacity" );
	sgOpacityCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgOpacityCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R8 );

	sgOpacityCaster->RunProcessing();
	std::string opacityTextureFilePath = sgOpacityCaster->GetOutputFilePath();
	
	// Update scene with new casted texture. 
	Simplygon::spMaterialTable sgMaterialTable = sg->CreateMaterialTable();
	Simplygon::spTextureTable sgTextureTable = sg->CreateTextureTable();
	Simplygon::spMaterial sgMaterial = sg->CreateMaterial();
	Simplygon::spTexture sgOpacityTexture = sg->CreateTexture();
	sgOpacityTexture->SetName( "Opacity" );
	sgOpacityTexture->SetFilePath( opacityTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgOpacityTexture );

	Simplygon::spShadingTextureNode sgOpacityTextureShadingNode = sg->CreateShadingTextureNode();
	sgOpacityTextureShadingNode->SetTexCoordLevel( 0 );
	sgOpacityTextureShadingNode->SetTextureName( "Opacity" );

	sgMaterial->AddMaterialChannel( "Opacity" );
	sgMaterial->SetShadingNetwork( "Opacity", sgOpacityTextureShadingNode );
	sgMaterial->SetBlendMode(Simplygon::EMaterialBlendMode::Blend);

	sgMaterialTable->AddMaterial( sgMaterial );

	sgScene->GetTextureTable()->Clear();
	sgScene->GetMaterialTable()->Clear();
	sgScene->GetTextureTable()->Copy(sgTextureTable);
	sgScene->GetMaterialTable()->Copy(sgMaterialTable);
	
	// Save processed scene. 	
	printf("%s\n", "Save processed scene.");
	SaveScene(sg, sgScene, "Output.glb");
	
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

	OpacityCasting(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

