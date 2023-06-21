// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

#include <string>
#include <stdlib.h>
#include <filesystem>
#include <future>
#include "SimplygonLoader.h"

class CustomObserver : public Simplygon::Observer
{
	public:
	bool OnProgress(Simplygon::spObject subject, Simplygon::real progressPercent) override
	{
		printf("Progress: %f\n", progressPercent);
		// return false to abort the processing
		return true;
	}
} customObserver;


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

void RunReduction(Simplygon::ISimplygon* sg)
{
	// Create the reduction pipeline. 
	Simplygon::spReductionPipeline sgReductionPipeline = sg->CreateReductionPipeline();
	
	// Add the custom observer to the reduction pipeline. 
	sgReductionPipeline->AddObserver(&customObserver);
	std::string outputScenePath = std::string("output\\") + std::string("ProgressEvent") + std::string("_Output.fbx");
	
	// Start the reduction pipeline. 	
	printf("%s\n", "Start the reduction pipeline.");
	sgReductionPipeline->RunSceneFromFile("../../../Assets/SimplygonMan/SimplygonMan.obj", outputScenePath.c_str(), Simplygon::EPipelineRunMode::RunInNewProcess);
	
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

