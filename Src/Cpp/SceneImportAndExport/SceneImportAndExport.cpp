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

void ImportExport(Simplygon::ISimplygon* sg)
{
	// Load obj scene. 	
	printf("%s\n", "Load obj scene.");
	Simplygon::spScene sgSceneObj = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	
	// Save obj scene. 	
	printf("%s\n", "Save obj scene.");
	SaveScene(sg, sgSceneObj, "Output.obj");
	
	// Load fbx scene. 	
	printf("%s\n", "Load fbx scene.");
	Simplygon::spScene sgSceneFbx = LoadScene(sg, "../../../Assets/RiggedSimplygonMan/RiggedSimplygonMan.fbx");
	
	// Save fbx scene. 	
	printf("%s\n", "Save fbx scene.");
	SaveScene(sg, sgSceneFbx, "Output.fbx");
	
	// Load glb scene. 	
	printf("%s\n", "Load glb scene.");
	Simplygon::spScene sgSceneGlb = LoadScene(sg, "../../../Assets/RiggedSimplygonMan/RiggedSimplygonMan.glb");
	
	// Save glb scene. 	
	printf("%s\n", "Save glb scene.");
	SaveScene(sg, sgSceneGlb, "Output.glb");
	
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

	ImportExport(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

