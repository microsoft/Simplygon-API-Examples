// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

#include <string>
#include <stdlib.h>
#include <filesystem>
#include <future>
#include "SimplygonLoader.h"

class CustomErrorHandler : public Simplygon::ErrorHandler
{
	public:
	void HandleError(Simplygon::spObject object, const char* interfaceName, const char* methodName, Simplygon::rid errorType , const char* errorText) override
	{
		printf("Error (%s:%s): %s\n", interfaceName, methodName, errorText);
	}
} customErrorHandler;


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

void RunReduction(Simplygon::ISimplygon* sg)
{
	// Set the custom error handler to the Simplygon interface. 
	sg->SetErrorHandler(&customErrorHandler);

	
	// Create the reduction pipeline. 
	Simplygon::spReductionPipeline sgReductionPipeline = sg->CreateReductionPipeline();
	
	// To be able to use the T-Junction remover, the welder has to be enabled so this will trigger an 
	// error. 
	Simplygon::spRepairSettings sgRepairSettings = sgReductionPipeline->GetRepairSettings();
	sgRepairSettings->SetUseWelding( false );
	sgRepairSettings->SetUseTJunctionRemover( true );
	
	// Start the reduction pipeline and the faulty settings will cause an error. 	
	printf("%s\n", "Start the reduction pipeline and the faulty settings will cause an error.");
	sgReductionPipeline->RunSceneFromFile("../../../Assets/SimplygonMan/SimplygonMan.obj", "Output.fbx", Simplygon::EPipelineRunMode::RunInNewProcess);
	
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

