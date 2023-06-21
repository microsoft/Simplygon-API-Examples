// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

#include <string>
#include <stdlib.h>
#include <filesystem>
#include <future>
#include "SimplygonLoader.h"


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

void RunReduction(Simplygon::ISimplygon* sg, std::string inputFile, std::string outputFile)
{
	// Create the reduction pipeline. 
	Simplygon::spReductionPipeline sgReductionPipeline = sg->CreateReductionPipeline();
	Simplygon::spReductionSettings sgReductionSettings = sgReductionPipeline->GetReductionSettings();
	
	// Set reduction target to triangle ratio with a ratio of 50%. 
	sgReductionSettings->SetReductionTargets( Simplygon::EStopCondition::All, true, false, false, false );
	sgReductionSettings->SetReductionTargetTriangleRatio( 0.5f );
	
	// Start the reduction pipeline. 	
	printf("%s\n", "Start the reduction pipeline.");
	sgReductionPipeline->RunSceneFromFile(inputFile.c_str(), outputFile.c_str(), Simplygon::EPipelineRunMode::RunDistributedUsingSimplygonGrid);
	
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

	std::vector<std::future<void>> jobs;
	for(auto& itr: std::filesystem::recursive_directory_iterator("../../../Assets/"))
	{
		if (itr.path().extension().compare(".glb") != 0)
			continue;
		std::string inputFile = itr.path().generic_u8string();
		std::string outputFile = std::string("output\\") + std::string("BatchingWithSimplygonGrid_") + itr.path().stem().concat("_LOD").concat( itr.path().extension().generic_u8string() ).generic_u8string();

		jobs.push_back( std::async( [](Simplygon::ISimplygon * sg, std::string inputFile, std::string outputFile)
		{
			RunReduction(sg, inputFile, outputFile);
		}, sg, inputFile, outputFile ));

	}
	for(auto& job: jobs)
	{
		job.wait();
	}

	Simplygon::Deinitialize(sg);

	return 0;
}

