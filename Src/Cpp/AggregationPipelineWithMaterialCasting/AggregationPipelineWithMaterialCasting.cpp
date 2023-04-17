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
	std::string outputScenePath = std::string("output\\") + std::string("AggregationPipelineWithMaterialCasting") + std::string("_") + std::string(path);
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
	
	// Error out if Simplygon has errors. 
	if (hasErrors)
	{
		throw std::exception("Processing failed with an error");
	}
}

void RunAggregationWithMaterialCasting(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	
	// Create the aggregation pipeline. 
	Simplygon::spAggregationPipeline sgAggregationPipeline = sg->CreateAggregationPipeline();
	Simplygon::spAggregationSettings sgAggregationSettings = sgAggregationPipeline->GetAggregationSettings();
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgAggregationPipeline->GetMappingImageSettings();
	
	// Merge all geometries into a single geometry. 
	sgAggregationSettings->SetMergeGeometries( true );
	
	// Generates a mapping image which is used after the aggregation to cast new materials to the new 
	// aggregated object. 
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
	
	// Add diffuse material caster to pipeline. 
	Simplygon::spColorCaster sgDiffuseCaster = sg->CreateColorCaster();

	Simplygon::spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster->GetColorCasterSettings();
	sgDiffuseCasterSettings->SetMaterialChannel( "Diffuse" );
	sgDiffuseCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );

	sgAggregationPipeline->AddMaterialCaster( sgDiffuseCaster, 0 );
	
	// Add normals material caster to pipeline. 
	Simplygon::spNormalCaster sgNormalsCaster = sg->CreateNormalCaster();

	Simplygon::spNormalCasterSettings sgNormalsCasterSettings = sgNormalsCaster->GetNormalCasterSettings();
	sgNormalsCasterSettings->SetMaterialChannel( "Normals" );
	sgNormalsCasterSettings->SetGenerateTangentSpaceNormals( true );
	sgNormalsCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );

	sgAggregationPipeline->AddMaterialCaster( sgNormalsCaster, 0 );
	
	// Start the aggregation pipeline. 	
	printf("%s\n", "Start the aggregation pipeline.");
	sgAggregationPipeline->RunScene(sgScene, Simplygon::EPipelineRunMode::RunInThisProcess);
	
	// Get the processed scene. 
	Simplygon::spScene sgProcessedScene = sgAggregationPipeline->GetProcessedScene();
	
	// Save processed scene. 	
	printf("%s\n", "Save processed scene.");
	SaveScene(sg, sgProcessedScene, "Output.fbx");
	
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

	RunAggregationWithMaterialCasting(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

