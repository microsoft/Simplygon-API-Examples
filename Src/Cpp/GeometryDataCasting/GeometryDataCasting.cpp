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

void RunGeometryDataDasting(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	
	// Create the remeshing processor. 
	Simplygon::spRemeshingProcessor sgRemeshingProcessor = sg->CreateRemeshingProcessor();
	sgRemeshingProcessor->SetScene( sgScene );
	Simplygon::spRemeshingSettings sgRemeshingSettings = sgRemeshingProcessor->GetRemeshingSettings();
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgRemeshingProcessor->GetMappingImageSettings();
	
	// Set on-screen size target for remeshing. 
	sgRemeshingSettings->SetOnScreenSize( 300 );
	
	// Generates a mapping image which is used after the remeshing to cast new materials to the new 
	// remeshed object. 
	sgMappingImageSettings->SetGenerateMappingImage( true );
	sgMappingImageSettings->SetApplyNewMaterialIds( true );
	sgMappingImageSettings->SetGenerateTangents( true );
	sgMappingImageSettings->SetUseFullRetexturing( true );
	Simplygon::spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings->GetOutputMaterialSettings(0);
	
	// Setting the size of the output material for the mapping image. This will be the output size of the 
	// textures when we do material casting in a later stage. 
	sgOutputMaterialSettings->SetTextureWidth( 2048 );
	sgOutputMaterialSettings->SetTextureHeight( 2048 );
	
	// Start the remeshing process. 	
	printf("%s\n", "Start the remeshing process.");
	sgRemeshingProcessor->RunProcessing();
	
	// Setup and run the geometry data caster casting Coords to a texture. 	
	printf("%s\n", "Setup and run the geometry data caster casting Coords to a texture.");
	Simplygon::spGeometryDataCaster sgGeometryData_CoordsCaster = sg->CreateGeometryDataCaster();
	sgGeometryData_CoordsCaster->SetMappingImage( sgRemeshingProcessor->GetMappingImage() );
	sgGeometryData_CoordsCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgGeometryData_CoordsCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgGeometryData_CoordsCaster->SetOutputFilePath( "GeometryData_CoordsTexture" );

	Simplygon::spGeometryDataCasterSettings sgGeometryData_CoordsCasterSettings = sgGeometryData_CoordsCaster->GetGeometryDataCasterSettings();
	sgGeometryData_CoordsCasterSettings->SetMaterialChannel( "GeometryData_Coords" );
	sgGeometryData_CoordsCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgGeometryData_CoordsCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R16G16B16 );
	sgGeometryData_CoordsCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::NoFill );
	sgGeometryData_CoordsCasterSettings->SetGeometryDataFieldType( Simplygon::EGeometryDataFieldType::Coords );
	sgGeometryData_CoordsCasterSettings->SetGeometryDataFieldIndex( 0 );

	sgGeometryData_CoordsCaster->RunProcessing();
	std::string geometrydata_coordsTextureFilePath = sgGeometryData_CoordsCaster->GetOutputFilePath().c_str();
	
	// Setup and run the geometry data caster casting Normals to a texture. 	
	printf("%s\n", "Setup and run the geometry data caster casting Normals to a texture.");
	Simplygon::spGeometryDataCaster sgGeometryData_NormalsCaster = sg->CreateGeometryDataCaster();
	sgGeometryData_NormalsCaster->SetMappingImage( sgRemeshingProcessor->GetMappingImage() );
	sgGeometryData_NormalsCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgGeometryData_NormalsCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgGeometryData_NormalsCaster->SetOutputFilePath( "GeometryData_NormalsTexture" );

	Simplygon::spGeometryDataCasterSettings sgGeometryData_NormalsCasterSettings = sgGeometryData_NormalsCaster->GetGeometryDataCasterSettings();
	sgGeometryData_NormalsCasterSettings->SetMaterialChannel( "GeometryData_Normals" );
	sgGeometryData_NormalsCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgGeometryData_NormalsCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R16G16B16 );
	sgGeometryData_NormalsCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::NoFill );
	sgGeometryData_NormalsCasterSettings->SetGeometryDataFieldType( Simplygon::EGeometryDataFieldType::Normals );
	sgGeometryData_NormalsCasterSettings->SetGeometryDataFieldIndex( 0 );

	sgGeometryData_NormalsCaster->RunProcessing();
	std::string geometrydata_normalsTextureFilePath = sgGeometryData_NormalsCaster->GetOutputFilePath().c_str();
	
	// Setup and run the geometry data caster casting MaterialIds to a texture. 	
	printf("%s\n", "Setup and run the geometry data caster casting MaterialIds to a texture.");
	Simplygon::spGeometryDataCaster sgGeometryData_MaterialIdsCaster = sg->CreateGeometryDataCaster();
	sgGeometryData_MaterialIdsCaster->SetMappingImage( sgRemeshingProcessor->GetMappingImage() );
	sgGeometryData_MaterialIdsCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgGeometryData_MaterialIdsCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgGeometryData_MaterialIdsCaster->SetOutputFilePath( "GeometryData_MaterialIdsTexture" );

	Simplygon::spGeometryDataCasterSettings sgGeometryData_MaterialIdsCasterSettings = sgGeometryData_MaterialIdsCaster->GetGeometryDataCasterSettings();
	sgGeometryData_MaterialIdsCasterSettings->SetMaterialChannel( "GeometryData_MaterialIds" );
	sgGeometryData_MaterialIdsCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgGeometryData_MaterialIdsCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R8 );
	sgGeometryData_MaterialIdsCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::NoFill );
	sgGeometryData_MaterialIdsCasterSettings->SetGeometryDataFieldType( Simplygon::EGeometryDataFieldType::MaterialIds );
	sgGeometryData_MaterialIdsCasterSettings->SetGeometryDataFieldIndex( 0 );

	sgGeometryData_MaterialIdsCaster->RunProcessing();
	std::string geometrydata_materialidsTextureFilePath = sgGeometryData_MaterialIdsCaster->GetOutputFilePath().c_str();
	
	// Update scene with new casted textures. 
	Simplygon::spMaterialTable sgMaterialTable = sg->CreateMaterialTable();
	Simplygon::spTextureTable sgTextureTable = sg->CreateTextureTable();
	Simplygon::spMaterial sgMaterial = sg->CreateMaterial();
	Simplygon::spTexture sgGeometryData_CoordsTexture = sg->CreateTexture();
	sgGeometryData_CoordsTexture->SetName( "GeometryData_Coords" );
	sgGeometryData_CoordsTexture->SetFilePath( geometrydata_coordsTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgGeometryData_CoordsTexture );

	Simplygon::spShadingTextureNode sgGeometryData_CoordsTextureShadingNode = sg->CreateShadingTextureNode();
	sgGeometryData_CoordsTextureShadingNode->SetTexCoordLevel( 0 );
	sgGeometryData_CoordsTextureShadingNode->SetTextureName( "GeometryData_Coords" );

	sgMaterial->AddMaterialChannel( "GeometryData_Coords" );
	sgMaterial->SetShadingNetwork( "GeometryData_Coords", sgGeometryData_CoordsTextureShadingNode );
	Simplygon::spTexture sgGeometryData_NormalsTexture = sg->CreateTexture();
	sgGeometryData_NormalsTexture->SetName( "GeometryData_Normals" );
	sgGeometryData_NormalsTexture->SetFilePath( geometrydata_normalsTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgGeometryData_NormalsTexture );

	Simplygon::spShadingTextureNode sgGeometryData_NormalsTextureShadingNode = sg->CreateShadingTextureNode();
	sgGeometryData_NormalsTextureShadingNode->SetTexCoordLevel( 0 );
	sgGeometryData_NormalsTextureShadingNode->SetTextureName( "GeometryData_Normals" );

	sgMaterial->AddMaterialChannel( "GeometryData_Normals" );
	sgMaterial->SetShadingNetwork( "GeometryData_Normals", sgGeometryData_NormalsTextureShadingNode );
	Simplygon::spTexture sgGeometryData_MaterialIdsTexture = sg->CreateTexture();
	sgGeometryData_MaterialIdsTexture->SetName( "GeometryData_MaterialIds" );
	sgGeometryData_MaterialIdsTexture->SetFilePath( geometrydata_materialidsTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgGeometryData_MaterialIdsTexture );

	Simplygon::spShadingTextureNode sgGeometryData_MaterialIdsTextureShadingNode = sg->CreateShadingTextureNode();
	sgGeometryData_MaterialIdsTextureShadingNode->SetTexCoordLevel( 0 );
	sgGeometryData_MaterialIdsTextureShadingNode->SetTextureName( "GeometryData_MaterialIds" );

	sgMaterial->AddMaterialChannel( "GeometryData_MaterialIds" );
	sgMaterial->SetShadingNetwork( "GeometryData_MaterialIds", sgGeometryData_MaterialIdsTextureShadingNode );

	sgMaterialTable->AddMaterial( sgMaterial );

	sgScene->GetTextureTable()->Clear();
	sgScene->GetMaterialTable()->Clear();
	sgScene->GetTextureTable()->Copy(sgTextureTable);
	sgScene->GetMaterialTable()->Copy(sgMaterialTable);
	
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

	RunGeometryDataDasting(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

