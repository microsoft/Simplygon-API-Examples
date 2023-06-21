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
	std::string outputScenePath = std::string("output\\") + std::string("RemeshingWithDisplacementTessellation") + std::string("_") + std::string(path);
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
}

void RunRemeshingWithDisplacementTessellation(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	
	// Create the remeshing processor. 
	Simplygon::spRemeshingProcessor sgRemeshingProcessor = sg->CreateRemeshingProcessor();
	sgRemeshingProcessor->SetScene( sgScene );
	
	// Fetch all the needed settings objects for the processing, including the attribute 
	// tessellation settings, which we will set up to receive displacement data in the processed 
	// mesh. 
	Simplygon::spRemeshingSettings sgRemeshingSettings = sgRemeshingProcessor->GetRemeshingSettings();
	Simplygon::spAttributeTessellationSettings sgAttributeTessellationSettings = sgRemeshingProcessor->GetAttributeTessellationSettings();
	
	// Set on-screen size target for remeshing, and tell the remeshing processor to cast 
	// displacement data into the attribute tessellation field of the processed geometry. Note: The 
	// tessellation settings are defined in the section below. 
	sgRemeshingSettings->SetOnScreenSize( 1000 );
	sgRemeshingSettings->SetPopulateAttributeTessellationDisplacement( true );
	sgRemeshingSettings->SetSurfaceTransferMode( Simplygon::ESurfaceTransferMode::Fast );
	
	// Set the tessellation settings. The displacement data will be cast into a tessellated 
	// displacement attribute. In this example we use OnScreenSize as the density setting, which 
	// means that triangles are tessellated based on the size of the rendered object, so that a 
	// triangle is when tessellated roughly the size of a pixel. We also add some additional 
	// constraints, such as only allowing base triangles to tessellate to level 5 (1024 
	// sub-triangles), only allow one level of difference between neighbor base-triangles, and the 
	// total number of sub-triangles should not exceed 1000000. 
	sgAttributeTessellationSettings->SetEnableAttributeTessellation( true );
	sgAttributeTessellationSettings->SetAttributeTessellationDensityMode( Simplygon::EAttributeTessellationDensityMode::OnScreenSize );
	sgAttributeTessellationSettings->SetOnScreenSize( 1000 );
	sgAttributeTessellationSettings->SetOnlyAllowOneLevelOfDifference( true );
	sgAttributeTessellationSettings->SetMinTessellationLevel( 0 );
	sgAttributeTessellationSettings->SetMaxTessellationLevel( 5 );
	sgAttributeTessellationSettings->SetMaxTotalValuesCount( 1000000 );
	
	// Start the remeshing process. 	
	printf("%s\n", "Start the remeshing process.");
	sgRemeshingProcessor->RunProcessing();
	
	// Replace original materials and textures from the scene with a new empty material, as the 
	// remeshed object has a new UV set.  
	sgScene->GetTextureTable()->Clear();
	sgScene->GetMaterialTable()->Clear();
	auto defaultMaterial = sg->CreateMaterial();
	defaultMaterial->SetName("defaultMaterial");
	sgScene->GetMaterialTable()->AddMaterial( defaultMaterial );
	
	// Save processed remeshed scene. 	
	printf("%s\n", "Save processed remeshed scene.");
	SaveScene(sg, sgScene, "OutputBase.obj");
	
	// We will now create an attribute tessellation tool object, and generate a scene with the 
	// tessellated attribute displacement data generated into real tessellated mesh data, which is 
	// stored into a new scene. 
	Simplygon::spAttributeTessellation sgAttributeTessellation = sg->CreateAttributeTessellation();
	Simplygon::spScene sgTessellatedScene = sgAttributeTessellation->NewTessellatedScene(sgScene);
	
	// Save the tessellated copy of the scene. 	
	printf("%s\n", "Save the tessellated copy of the scene.");
	SaveScene(sg, sgTessellatedScene, "OutputTessellation.obj");
	
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

	RunRemeshingWithDisplacementTessellation(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

