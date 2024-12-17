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
	std::string outputScenePath = std::string("output\\") + std::string("AttributeTessellation") + std::string("_") + std::string(path);
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

void RunRemeshingWithTessellatedAttributes(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	
	// Create the remeshing pipeline. 
	Simplygon::spRemeshingPipeline sgRemeshingPipeline = sg->CreateRemeshingPipeline();
	
	// Fetch all the needed settings objects for the processing, including the attribute 
	// tessellation settings, which we will use to set up the attribute tessellation on the processed 
	// mesh. 
	Simplygon::spRemeshingSettings sgRemeshingSettings = sgRemeshingPipeline->GetRemeshingSettings();
	Simplygon::spAttributeTessellationSettings sgAttributeTessellationSettings = sgRemeshingPipeline->GetAttributeTessellationSettings();
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgRemeshingPipeline->GetMappingImageSettings();
	
	// Set on-screen size target for remeshing. 
	sgRemeshingSettings->SetOnScreenSize( 500 );
	sgRemeshingSettings->SetGeometricalAccuracy( 2.0f );
	
	// Get the attribute tessellation settings. The displacement data will be cast into a 
	// tessellated displacement attribute. In this example we use relative area as the density 
	// setting, which means that triangles are tessellated based on the size of the triangle, so that 
	// the tessellated attributes roughly take up the same area. The value is normalized and scale 
	// independent, so the total area of all the subvalues will add up to the normalized value 1. We set 
	// the maximum area per value to 1/1000000, which means that there will be at least 1000000 values 
	// total in the scene, unless we cap the total number of values with MaxTotalValuesCount or 
	// MaxTessellationLevel. 
	sgAttributeTessellationSettings->SetEnableAttributeTessellation( true );
	sgAttributeTessellationSettings->SetAttributeTessellationDensityMode( Simplygon::EAttributeTessellationDensityMode::RelativeArea );
	sgAttributeTessellationSettings->SetMaxAreaOfTessellatedValue( 0.000001f );
	sgAttributeTessellationSettings->SetOnlyAllowOneLevelOfDifference( true );
	sgAttributeTessellationSettings->SetMinTessellationLevel( 0 );
	sgAttributeTessellationSettings->SetMaxTessellationLevel( 5 );
	sgAttributeTessellationSettings->SetMaxTotalValuesCount( 1000000 );
	
	// Set up the process to generate a mapping image which will be used after the reduction to cast new 
	// materials to the new reduced object, and also to cast the displacement data from the original 
	// object into the tessellated attributes of the processed mesh. 
	sgMappingImageSettings->SetGenerateMappingImage( true );
	sgMappingImageSettings->SetGenerateTexCoords( true );
	sgMappingImageSettings->SetApplyNewMaterialIds( true );
	sgMappingImageSettings->SetGenerateTangents( true );
	sgMappingImageSettings->SetUseFullRetexturing( true );
	sgMappingImageSettings->SetTexCoordGeneratorType( Simplygon::ETexcoordGeneratorType::ChartAggregator );
	Simplygon::spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings->GetOutputMaterialSettings(0);
	
	// Set the size of the mapping image in the output material. This will be the output size of the 
	// textures when we do the material casting in the pipeline. 
	sgOutputMaterialSettings->SetTextureWidth( 2048 );
	sgOutputMaterialSettings->SetTextureHeight( 2048 );
	sgOutputMaterialSettings->SetMultisamplingLevel( 2 );
	
	// Add a diffuse texture caster to the pipeline. This will cast the diffuse color (aka base 
	// color/albedo) in the original scene into a texture map in the output scene. 
	Simplygon::spColorCaster sgDiffuseCaster = sg->CreateColorCaster();

	Simplygon::spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster->GetColorCasterSettings();
	sgDiffuseCasterSettings->SetMaterialChannel( "Diffuse" );
	sgDiffuseCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );

	sgRemeshingPipeline->AddMaterialCaster( sgDiffuseCaster, 0 );
	
	// Add a normals texture caster to the pipeline. This will cast the normals in the original scene 
	// into a normal map in the output scene. 
	Simplygon::spNormalCaster sgNormalsCaster = sg->CreateNormalCaster();

	Simplygon::spNormalCasterSettings sgNormalsCasterSettings = sgNormalsCaster->GetNormalCasterSettings();
	sgNormalsCasterSettings->SetMaterialChannel( "Normals" );
	sgNormalsCasterSettings->SetGenerateTangentSpaceNormals( true );
	sgNormalsCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );

	sgRemeshingPipeline->AddMaterialCaster( sgNormalsCaster, 0 );
	
	// Add a displacement caster to the pipeline. This will cast the displacement values, but instead 
	// of casting to a texture, it will cast into the tessellated attributes. 
	Simplygon::spDisplacementCaster sgDisplacementCaster = sg->CreateDisplacementCaster();

	sgDisplacementCaster->SetScene( sgScene );
	Simplygon::spDisplacementCasterSettings sgDisplacementCasterSettings = sgDisplacementCaster->GetDisplacementCasterSettings();
	sgDisplacementCasterSettings->SetMaterialChannel( "Displacement" );
	sgDisplacementCasterSettings->SetDilation( 10 );
	sgDisplacementCasterSettings->SetOutputToTessellatedAttributes( true );
	sgDisplacementCasterSettings->GetAttributeTessellationSamplingSettings()->SetSourceMaterialId( 0 );
	sgDisplacementCasterSettings->GetAttributeTessellationSamplingSettings()->SetAttributeFormat( Simplygon::EAttributeFormat::U16 );
	sgDisplacementCasterSettings->GetAttributeTessellationSamplingSettings()->SetSupersamplingCount( 16 );
	sgDisplacementCasterSettings->GetAttributeTessellationSamplingSettings()->SetBlendOperation( Simplygon::EBlendOperation::Mean );

	sgRemeshingPipeline->AddMaterialCaster( sgDisplacementCaster, 0 );
	
	// Start the remeshing pipeline. 	
	printf("%s\n", "Start the remeshing pipeline.");
	sgRemeshingPipeline->RunScene(sgScene, Simplygon::EPipelineRunMode::RunInThisProcess);
	
	// Save processed scene. 	
	printf("%s\n", "Save processed scene.");
	SaveScene(sg, sgScene, "RemeshedOutput.gltf");
	
	// Create an attribute tessellation tool object. 
	Simplygon::spAttributeTessellation sgAttributeTessellation = sg->CreateAttributeTessellation();
	
	// Generate a tessellated copy of the scene. 	
	printf("%s\n", "Generate a tessellated copy of the scene.");
	Simplygon::spScene sgTessellatedScene = sgAttributeTessellation->NewTessellatedScene(sgScene);
	
	// Save the tessellated copy of the scene. 	
	printf("%s\n", "Save the tessellated copy of the scene.");
	SaveScene(sg, sgTessellatedScene, "RemeshedTessellatedOutput.obj");
	
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

	RunRemeshingWithTessellatedAttributes(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

