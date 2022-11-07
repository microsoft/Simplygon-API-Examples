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
	std::string outputScenePath = std::string("output\\") + std::string("AggregationWithVisibilityCamera") + std::string("_") + std::string(path);
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

void RunAggregation(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/Teapot.obj");
	
	// Create the aggregation processor. 
	Simplygon::spAggregationProcessor sgAggregationProcessor = sg->CreateAggregationProcessor();
	sgAggregationProcessor->SetScene( sgScene );
	Simplygon::spAggregationSettings sgAggregationSettings = sgAggregationProcessor->GetAggregationSettings();
	Simplygon::spVisibilitySettings sgVisibilitySettings = sgAggregationProcessor->GetVisibilitySettings();
	
	// Merge all geometries into a single geometry. 
	sgAggregationSettings->SetMergeGeometries( true );
	
	// Add a camera to the scene. We'll use this later as a visibility camera. 
	Simplygon::spSelectionSetTable sgSceneSelectionSetTable = sgScene->GetSelectionSetTable();
	Simplygon::spSelectionSet sgCameraSelectionSet = sg->CreateSelectionSet();
	sgCameraSelectionSet->SetName("Camera");
	Simplygon::spSceneCamera sgCameraSceneCamera = sg->CreateSceneCamera();
	sgCameraSceneCamera->SetCustomSphereCameraPath(4, 90, 180, 90);
	sgScene->GetRootNode()->AddChild(sgCameraSceneCamera);
	sgCameraSelectionSet->AddItem(sgCameraSceneCamera->GetNodeGUID());
	sgSceneSelectionSetTable->AddSelectionSet(sgCameraSelectionSet);
	
	// Use the camera previously added. 
	sgVisibilitySettings->SetCameraSelectionSetName( "Camera" );
	
	// Enabled GPU based visibility calculations. 
	sgVisibilitySettings->SetComputeVisibilityMode( Simplygon::EComputeVisibilityMode::DirectX );
	
	// Disabled conservative mode. 
	sgVisibilitySettings->SetConservativeMode( false );
	
	// Remove all non visible geometry. 
	sgVisibilitySettings->SetCullOccludedGeometry( true );
	
	// Skip filling nonvisible regions. 
	sgVisibilitySettings->SetFillNonVisibleAreaThreshold( 0.0f );
	
	// Don't remove non occluding triangles. 
	sgVisibilitySettings->SetRemoveTrianglesNotOccludingOtherTriangles( false );
	
	// Remove all back facing triangles. 
	sgVisibilitySettings->SetUseBackfaceCulling( true );
	
	// Don't use visibility weights. 
	sgVisibilitySettings->SetUseVisibilityWeightsInReducer( false );
	
	// Start the aggregation process. 	
	printf("%s\n", "Start the aggregation process.");
	sgAggregationProcessor->RunProcessing();
	
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

	RunAggregation(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

