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

void RunReduction(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/ObscuredTeapot.obj");
	
	// Create the reduction pipeline. 
	Simplygon::spReductionPipeline sgReductionPipeline = sg->CreateReductionPipeline();
	Simplygon::spReductionSettings sgReductionSettings = sgReductionPipeline->GetReductionSettings();
	Simplygon::spVisibilitySettings sgVisibilitySettings = sgReductionPipeline->GetVisibilitySettings();
	
	// Set reduction target to triangle ratio with a ratio of 50%. 
	sgReductionSettings->SetReductionTargets( Simplygon::EStopCondition::All, true, false, false, false );
	sgReductionSettings->SetReductionTargetTriangleRatio( 0.5f );
	
	// Add a selection set to the scene. We'll use this later as a occluder. 
	Simplygon::spSelectionSetTable sgSceneSelectionSetTable = sgScene->GetSelectionSetTable();
	Simplygon::spSelectionSet sgOccluderSelectionSet = sg->CreateSelectionSet();
	sgOccluderSelectionSet->SetName("Occluder");
	Simplygon::spSceneNode sgRootBox002 = sgScene->GetNodeFromPath("Root/Box002");
	if (!sgRootBox002.IsNull())
		sgOccluderSelectionSet->AddItem(sgRootBox002->GetNodeGUID());
	sgSceneSelectionSetTable->AddSelectionSet(sgOccluderSelectionSet);
	
	// Use the occluder previously added. 
	sgVisibilitySettings->SetOccluderSelectionSetName( "Occluder" );
	
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
	
	// Start the reduction pipeline. 	
	printf("%s\n", "Start the reduction pipeline.");
	sgReductionPipeline->RunScene(sgScene, Simplygon::EPipelineRunMode::RunInThisProcess);
	
	// Get the processed scene. 
	Simplygon::spScene sgProcessedScene = sgReductionPipeline->GetProcessedScene();
	
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

	RunReduction(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

