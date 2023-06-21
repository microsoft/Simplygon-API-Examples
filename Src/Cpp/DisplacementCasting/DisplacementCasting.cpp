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
	std::string outputScenePath = std::string("output\\") + std::string("DisplacementCasting") + std::string("_") + std::string(path);
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

void DisplacementCasting(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/Wall/wall.obj");
	
	// Create the remeshing processor. 
	Simplygon::spRemeshingProcessor sgRemeshingProcessor = sg->CreateRemeshingProcessor();
	sgRemeshingProcessor->SetScene( sgScene );
	Simplygon::spRemeshingSettings sgRemeshingSettings = sgRemeshingProcessor->GetRemeshingSettings();
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgRemeshingProcessor->GetMappingImageSettings();
	
	// Set on screen size for the remeshing to only 20 pixels. 
	sgRemeshingSettings->SetOnScreenSize( 20 );
	
	// Generates a mapping image which is used after the remeshing to cast new materials to the new 
	// object. 
	sgMappingImageSettings->SetGenerateMappingImage( true );
	sgMappingImageSettings->SetGenerateTexCoords( true );
	sgMappingImageSettings->SetApplyNewMaterialIds( true );
	sgMappingImageSettings->SetGenerateTangents( true );
	sgMappingImageSettings->SetUseFullRetexturing( true );
	Simplygon::spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings->GetOutputMaterialSettings(0);
	
	// Setting the size of the output material for the mapping image. This will be the output size of the 
	// textures when we do material casting in a later stage. 
	sgOutputMaterialSettings->SetTextureWidth( 2048 );
	sgOutputMaterialSettings->SetTextureHeight( 2048 );
	
	// Start the remeshing process. 	
	printf("%s\n", "Start the remeshing process.");
	sgRemeshingProcessor->RunProcessing();
	
	// Setup and run the displacement material casting. 	
	printf("%s\n", "Setup and run the displacement material casting.");
	Simplygon::spDisplacementCaster sgDisplacementCaster = sg->CreateDisplacementCaster();
	sgDisplacementCaster->SetMappingImage( sgRemeshingProcessor->GetMappingImage() );
	sgDisplacementCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgDisplacementCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgDisplacementCaster->SetOutputFilePath( "DisplacementTexture" );

	Simplygon::spDisplacementCasterSettings sgDisplacementCasterSettings = sgDisplacementCaster->GetDisplacementCasterSettings();
	sgDisplacementCasterSettings->SetMaterialChannel( "Displacement" );
	sgDisplacementCasterSettings->SetGenerateScalarDisplacement( true );
	sgDisplacementCasterSettings->SetGenerateTangentSpaceDisplacement( false );
	sgDisplacementCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgDisplacementCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R8G8B8 );

	sgDisplacementCaster->RunProcessing();
	std::string displacementTextureFilePath = sgDisplacementCaster->GetOutputFilePath().c_str();
	
	// Update scene with new casted texture. 
	Simplygon::spMaterialTable sgMaterialTable = sg->CreateMaterialTable();
	Simplygon::spTextureTable sgTextureTable = sg->CreateTextureTable();
	Simplygon::spMaterial sgMaterial = sg->CreateMaterial();
	sgMaterial->SetName("OutputMaterial");
	Simplygon::spTexture sgDisplacementTexture = sg->CreateTexture();
	sgDisplacementTexture->SetName( "Displacement" );
	sgDisplacementTexture->SetFilePath( displacementTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgDisplacementTexture );

	Simplygon::spShadingTextureNode sgDisplacementTextureShadingNode = sg->CreateShadingTextureNode();
	sgDisplacementTextureShadingNode->SetTexCoordLevel( 0 );
	sgDisplacementTextureShadingNode->SetTextureName( "Displacement" );

	sgMaterial->AddMaterialChannel( "Displacement" );
	sgMaterial->SetShadingNetwork( "Displacement", sgDisplacementTextureShadingNode );

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

	DisplacementCasting(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

