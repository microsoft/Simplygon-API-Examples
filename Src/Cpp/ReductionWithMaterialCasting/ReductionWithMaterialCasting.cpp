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
	bool importResult = sgSceneImporter->RunImport();
	if (!importResult)
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
	sgSceneExporter->SetExportFilePath(path);
	sgSceneExporter->SetScene(sgScene);
	
	// Run scene exporter. 
	bool exportResult = sgSceneExporter->RunExport();
	if (!exportResult)
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

void RunReductionWithMaterialCasting(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	
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
	sgMappingImageSettings->SetTexCoordGeneratorType( Simplygon::ETexcoordGeneratorType::ChartAggregator );
	Simplygon::spChartAggregatorSettings sgChartAggregatorSettings = sgMappingImageSettings->GetChartAggregatorSettings();
	
	// Enable the chart aggregator and reuse UV space. 
	sgChartAggregatorSettings->SetChartAggregatorMode( Simplygon::EChartAggregatorMode::SurfaceArea );
	sgChartAggregatorSettings->SetSeparateOverlappingCharts( false );
	Simplygon::spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings->GetOutputMaterialSettings(0);
	
	// Setting the size of the output material for the mapping image. This will be the output size of the 
	// textures when we do material casting in a later stage. 
	sgOutputMaterialSettings->SetTextureWidth( 2048 );
	sgOutputMaterialSettings->SetTextureHeight( 2048 );
	
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
	
	// Setup and run the normals material casting. 	
	printf("%s\n", "Setup and run the normals material casting.");
	Simplygon::spNormalCaster sgNormalsCaster = sg->CreateNormalCaster();
	sgNormalsCaster->SetMappingImage( sgReductionProcessor->GetMappingImage() );
	sgNormalsCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgNormalsCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgNormalsCaster->SetOutputFilePath( "NormalsTexture" );

	Simplygon::spNormalCasterSettings sgNormalsCasterSettings = sgNormalsCaster->GetNormalCasterSettings();
	sgNormalsCasterSettings->SetMaterialChannel( "Normals" );
	sgNormalsCasterSettings->SetGenerateTangentSpaceNormals( true );
	sgNormalsCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );

	sgNormalsCaster->RunProcessing();
	std::string normalsTextureFilePath = sgNormalsCaster->GetOutputFilePath().c_str();
	
	// Update scene with new casted textures. 
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
	Simplygon::spTexture sgNormalsTexture = sg->CreateTexture();
	sgNormalsTexture->SetName( "Normals" );
	sgNormalsTexture->SetFilePath( normalsTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgNormalsTexture );

	Simplygon::spShadingTextureNode sgNormalsTextureShadingNode = sg->CreateShadingTextureNode();
	sgNormalsTextureShadingNode->SetTexCoordLevel( 0 );
	sgNormalsTextureShadingNode->SetTextureName( "Normals" );

	sgMaterial->AddMaterialChannel( "Normals" );
	sgMaterial->SetShadingNetwork( "Normals", sgNormalsTextureShadingNode );

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

	RunReductionWithMaterialCasting(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

