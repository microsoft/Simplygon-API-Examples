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

void RunBillboardCloudPipeline(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/Cages/Cages.obj");
	
	// For all materials in the scene set the blend mode to blend (instead of opaque) 
	for (int i = 0; i < (int)sgScene->GetMaterialTable()->GetMaterialsCount(); ++i)
	{
		sgScene->GetMaterialTable()->GetMaterial(i)->SetBlendMode(Simplygon::EMaterialBlendMode::Blend);
	}
	
	// Create the Impostor processor. 
	Simplygon::spBillboardCloudPipeline sgBillboardCloudPipeline = sg->CreateBillboardCloudPipeline();
	Simplygon::spBillboardCloudSettings sgBillboardCloudSettings = sgBillboardCloudPipeline->GetBillboardCloudSettings();
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgBillboardCloudPipeline->GetMappingImageSettings();
	
	// Set billboard cloud mode to Outer shell. 
	sgBillboardCloudSettings->SetBillboardMode( Simplygon::EBillboardMode::OuterShell );
	sgBillboardCloudSettings->SetBillboardDensity( 0.5f );
	sgBillboardCloudSettings->SetGeometricComplexity( 0.9f );
	sgBillboardCloudSettings->SetMaxPlaneCount( 20 );
	sgBillboardCloudSettings->SetTwoSided( false );
	sgMappingImageSettings->SetMaximumLayers( 10 );
	Simplygon::spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings->GetOutputMaterialSettings(0);
	
	// Setting the size of the output material for the mapping image. This will be the output size of the 
	// textures when we do material casting in a later stage. 
	sgOutputMaterialSettings->SetTextureWidth( 1024 );
	sgOutputMaterialSettings->SetTextureHeight( 1024 );
	sgOutputMaterialSettings->SetMultisamplingLevel( 2 );
	
	// Add diffuse material caster to pipeline. 
	Simplygon::spColorCaster sgDiffuseCaster = sg->CreateColorCaster();

	Simplygon::spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster->GetColorCasterSettings();
	sgDiffuseCasterSettings->SetMaterialChannel( "Diffuse" );
	sgDiffuseCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgDiffuseCasterSettings->SetBakeOpacityInAlpha( false );
	sgDiffuseCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R8G8B8 );
	sgDiffuseCasterSettings->SetDilation( 10 );
	sgDiffuseCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::Interpolate );

	sgBillboardCloudPipeline->AddMaterialCaster( sgDiffuseCaster, 0 );
	
	// Add specular material caster to pipeline. 
	Simplygon::spColorCaster sgSpecularCaster = sg->CreateColorCaster();

	Simplygon::spColorCasterSettings sgSpecularCasterSettings = sgSpecularCaster->GetColorCasterSettings();
	sgSpecularCasterSettings->SetMaterialChannel( "Specular" );
	sgSpecularCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgSpecularCasterSettings->SetDilation( 10 );
	sgSpecularCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::Interpolate );

	sgBillboardCloudPipeline->AddMaterialCaster( sgSpecularCaster, 0 );
	
	// Add normals material caster to pipeline. 
	Simplygon::spNormalCaster sgNormalsCaster = sg->CreateNormalCaster();

	Simplygon::spNormalCasterSettings sgNormalsCasterSettings = sgNormalsCaster->GetNormalCasterSettings();
	sgNormalsCasterSettings->SetMaterialChannel( "Normals" );
	sgNormalsCasterSettings->SetGenerateTangentSpaceNormals( true );
	sgNormalsCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgNormalsCasterSettings->SetDilation( 10 );
	sgNormalsCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::Interpolate );

	sgBillboardCloudPipeline->AddMaterialCaster( sgNormalsCaster, 0 );
	
	// Setup and run the opacity material casting. Make sure the there is no dilation or fill. 
	Simplygon::spOpacityCaster sgOpacityCaster = sg->CreateOpacityCaster();

	Simplygon::spOpacityCasterSettings sgOpacityCasterSettings = sgOpacityCaster->GetOpacityCasterSettings();
	sgOpacityCasterSettings->SetMaterialChannel( "Opacity" );
	sgOpacityCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgOpacityCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::NoFill );
	sgOpacityCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R8 );

	sgBillboardCloudPipeline->AddMaterialCaster( sgOpacityCaster, 0 );
	
	// Start the impostor pipeline. 	
	printf("%s\n", "Start the impostor pipeline.");
	sgBillboardCloudPipeline->RunScene(sgScene, Simplygon::EPipelineRunMode::RunInThisProcess);
	
	// Get the processed scene. 
	Simplygon::spScene sgProcessedScene = sgBillboardCloudPipeline->GetProcessedScene();
	
	// Save processed scene. 	
	printf("%s\n", "Save processed scene.");
	SaveScene(sg, sgProcessedScene, "Output.glb");
	
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

	RunBillboardCloudPipeline(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

