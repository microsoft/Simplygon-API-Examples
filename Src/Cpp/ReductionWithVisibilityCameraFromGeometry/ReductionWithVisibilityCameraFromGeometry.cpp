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
	
	// Load camera gemometry 
	Simplygon::spScene sgSceneGeometryCamera = LoadScene(sg, "../../../Assets/ObscuredTeapot/CameraMesh.obj");
	
	// Select Mesh Nodes 
	int selectionSetId = sgSceneGeometryCamera->SelectNodes("ISceneMesh");
	Simplygon::spSelectionSetTable sgSelectionSetsTable = sgSceneGeometryCamera->GetSelectionSetTable();
	Simplygon::spSelectionSet selectionSceneMeshes = sgSelectionSetsTable->GetSelectionSet(selectionSetId);
	auto itemCount = selectionSceneMeshes->GetItemCount();
	Simplygon::spSelectionSet cameraSelectionSet = sg->CreateSelectionSet();
	
	// Copy each mesh from camera scene into a scene and created a camera selection set based on those 
	// ids. 
	for (auto meshIndex = 0U; meshIndex < itemCount; ++meshIndex)
	{
		Simplygon::spString meshNodeId = selectionSceneMeshes->GetItem(meshIndex);
		Simplygon::spSceneNode sceneNode = sgSceneGeometryCamera->GetNodeByGUID(meshNodeId);
		Simplygon::spSceneMesh sceneMesh = Simplygon::spSceneMesh::SafeCast(sceneNode);
		Simplygon::spGeometryData geom = sceneMesh->GetGeometry();
		Simplygon::spSceneMesh cameraMesh = sgScene->GetRootNode()->CreateChildMesh(geom);
		Simplygon::spString nodeId = cameraMesh->GetNodeGUID();
		cameraSelectionSet->AddItem(nodeId);
	}
	int cameraSelectionSetId = sgScene->GetSelectionSetTable()->AddSelectionSet(cameraSelectionSet);
	
	// Create the reduction processor. 
	Simplygon::spReductionProcessor sgReductionProcessor = sg->CreateReductionProcessor();
	
	// Get settings objects 
	Simplygon::spReductionSettings sgReductionSettings = sgReductionProcessor->GetReductionSettings();
	Simplygon::spVisibilitySettings sgVisibilitySettings = sgReductionProcessor->GetVisibilitySettings();
	
	// Set camera selection set id with 
	sgVisibilitySettings->SetCameraSelectionSetID( cameraSelectionSetId );
	
	// Setup visibility setting enable GPU based computation, 
	sgVisibilitySettings->SetUseVisibilityWeightsInReducer( true );
	sgVisibilitySettings->SetUseVisibilityWeightsInTexcoordGenerator( false );
	sgVisibilitySettings->SetComputeVisibilityMode( Simplygon::EComputeVisibilityMode::DirectX );
	sgVisibilitySettings->SetConservativeMode( false );
	sgVisibilitySettings->SetCullOccludedGeometry( true );
	sgVisibilitySettings->SetFillNonVisibleAreaThreshold( 0.0f );
	sgVisibilitySettings->SetRemoveTrianglesNotOccludingOtherTriangles( false );
	sgVisibilitySettings->SetUseBackfaceCulling( true );
	
	// Set reduction target to triangle ratio with a ratio of 50%. 
	sgReductionSettings->SetReductionTargetTriangleRatio( 0.5f );
	sgReductionProcessor->SetScene( sgScene );
	
	// Start the reduction process. 	
	printf("%s\n", "Start the reduction process.");
	sgReductionProcessor->RunProcessing();
	
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

	RunReduction(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

