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

void RunRemeshing(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/ObscuredTeapot/ObscuredTeapot.obj");
	
	// Create the remeshing processor. 
	Simplygon::spRemeshingProcessor sgRemeshingProcessor = sg->CreateRemeshingProcessor();
	sgRemeshingProcessor->SetScene( sgScene );
	Simplygon::spRemeshingSettings sgRemeshingSettings = sgRemeshingProcessor->GetRemeshingSettings();
	
	// Add a selection set to the scene with all nodes which should be remeshed. 
	Simplygon::spSelectionSetTable sgSceneSelectionSetTable = sgScene->GetSelectionSetTable();
	Simplygon::spSelectionSet sgRemeshingTargetSelectionSet = sg->CreateSelectionSet();
	sgRemeshingTargetSelectionSet->SetName("RemeshingTarget");
	Simplygon::spSceneNode sgRootTeapot001 = sgScene->GetNodeFromPath("Root/Teapot001");
	if (!sgRootTeapot001.IsNull())
		sgRemeshingTargetSelectionSet->AddItem(sgRootTeapot001->GetNodeGUID());
	sgSceneSelectionSetTable->AddSelectionSet(sgRemeshingTargetSelectionSet);
	
	// Set on-screen size target for remeshing. 
	sgRemeshingSettings->SetOnScreenSize( 300 );
	
	// Use the selection set created earlier. 
	sgRemeshingSettings->SetProcessSelectionSetName( "RemeshingTarget" );
	
	// Start the remeshing process. 	
	printf("%s\n", "Start the remeshing process.");
	sgRemeshingProcessor->RunProcessing();
	
	// Replace original materials and textures from the scene with a new empty material, as the 
	// remeshed object has a new UV set.  
	sgScene->GetTextureTable()->Clear();
	sgScene->GetMaterialTable()->Clear();
	sgScene->GetMaterialTable()->AddMaterial( sg->CreateMaterial() );
	
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

	RunRemeshing(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

