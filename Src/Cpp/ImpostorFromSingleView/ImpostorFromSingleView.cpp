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
	std::string outputScenePath = std::string("output\\") + std::string("ImpostorFromSingleView") + std::string("_") + std::string(path);
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
	
	// Error out if Simplygon has errors. 
	if (hasErrors)
	{
		throw std::exception("Processing failed with an error");
	}
}

void RunImpostorFromSingleView(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/Bush/Bush.fbx");
	
	// For all materials in the scene set the blend mode to blend (instead of opaque) 
	for (int i = 0; i < (int)sgScene->GetMaterialTable()->GetMaterialsCount(); ++i)
	{
		sgScene->GetMaterialTable()->GetMaterial(i)->SetBlendMode(Simplygon::EMaterialBlendMode::Blend);
	}
	
	// For all materials in the scene set the opacity mode to Opacity. 
	for (int i = 0; i < (int)sgScene->GetMaterialTable()->GetMaterialsCount(); ++i)
	{
		sgScene->GetMaterialTable()->GetMaterial(i)->SetOpacityType(Simplygon::EOpacityType::Opacity);
	}
	
	// Create the Impostor processor. 
	Simplygon::spImpostorProcessor sgImpostorProcessor = sg->CreateImpostorProcessor();
	sgImpostorProcessor->SetScene( sgScene );
	Simplygon::spImpostorSettings sgImpostorSettings = sgImpostorProcessor->GetImpostorSettings();
	
	// Set impostor type to From single view. 
	sgImpostorSettings->SetImpostorType( Simplygon::EImpostorType::FromSingleView );
	Simplygon::real viewDir[] = {0.0f, 0.0f, -1.0f};

	Simplygon::spImpostorFromSingleViewSettings sgImpostorFromSingleViewSettings = sgImpostorSettings->GetImpostorFromSingleViewSettings();
	sgImpostorFromSingleViewSettings->SetUseTightFitting( true );
	sgImpostorFromSingleViewSettings->SetTightFittingDepthOffset( 1.0f );
	sgImpostorFromSingleViewSettings->SetTexCoordPadding( 0.01f );
	sgImpostorFromSingleViewSettings->SetViewDirection( viewDir );
	// Once geometry and settings are set, you can calculate the aspect ratio for the textures.
	Simplygon::real aspect = sgImpostorProcessor->GetSingleViewAspectRatio();
	if (aspect < 0.0f)
		throw std::exception("Failed to compute aspect ratio!");
	int xDim = int(aspect * 512);
	int yDim = int(1.f * 512);
	Simplygon::spMappingImageSettings sgMappingImageSettings = sgImpostorProcessor->GetMappingImageSettings();
	sgMappingImageSettings->SetMaximumLayers( 10 );
	Simplygon::spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings->GetOutputMaterialSettings(0);
	
	// Setting the size of the output material for the mapping image. This will be the output size of the 
	// textures when we do material casting in a later stage. 
	sgOutputMaterialSettings->SetTextureWidth( xDim );
	sgOutputMaterialSettings->SetTextureHeight( yDim );
	sgOutputMaterialSettings->SetMultisamplingLevel( 2 );
	
	// Start the impostor process. 	
	printf("%s\n", "Start the impostor process.");
	sgImpostorProcessor->RunProcessing();
	
	// Setup and run the diffuse material casting. 	
	printf("%s\n", "Setup and run the diffuse material casting.");
	Simplygon::spColorCaster sgDiffuseCaster = sg->CreateColorCaster();
	sgDiffuseCaster->SetMappingImage( sgImpostorProcessor->GetMappingImage() );
	sgDiffuseCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgDiffuseCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgDiffuseCaster->SetOutputFilePath( "DiffuseTexture" );

	Simplygon::spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster->GetColorCasterSettings();
	sgDiffuseCasterSettings->SetMaterialChannel( "Diffuse" );
	sgDiffuseCasterSettings->SetOpacityChannel( "Opacity" );
	sgDiffuseCasterSettings->SetOpacityChannelComponent( Simplygon::EColorComponent::Alpha );
	sgDiffuseCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgDiffuseCasterSettings->SetBakeOpacityInAlpha( false );
	sgDiffuseCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R8G8B8 );
	sgDiffuseCasterSettings->SetDilation( 10 );
	sgDiffuseCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::Interpolate );

	sgDiffuseCaster->RunProcessing();
	std::string diffuseTextureFilePath = sgDiffuseCaster->GetOutputFilePath().c_str();
	
	// Setup and run the specular material casting. 	
	printf("%s\n", "Setup and run the specular material casting.");
	Simplygon::spColorCaster sgSpecularCaster = sg->CreateColorCaster();
	sgSpecularCaster->SetMappingImage( sgImpostorProcessor->GetMappingImage() );
	sgSpecularCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgSpecularCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgSpecularCaster->SetOutputFilePath( "SpecularTexture" );

	Simplygon::spColorCasterSettings sgSpecularCasterSettings = sgSpecularCaster->GetColorCasterSettings();
	sgSpecularCasterSettings->SetMaterialChannel( "Specular" );
	sgSpecularCasterSettings->SetOpacityChannel( "Opacity" );
	sgSpecularCasterSettings->SetOpacityChannelComponent( Simplygon::EColorComponent::Alpha );
	sgSpecularCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgSpecularCasterSettings->SetDilation( 10 );
	sgSpecularCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::Interpolate );

	sgSpecularCaster->RunProcessing();
	std::string specularTextureFilePath = sgSpecularCaster->GetOutputFilePath().c_str();
	
	// Setup and run the normals material casting. 	
	printf("%s\n", "Setup and run the normals material casting.");
	Simplygon::spNormalCaster sgNormalsCaster = sg->CreateNormalCaster();
	sgNormalsCaster->SetMappingImage( sgImpostorProcessor->GetMappingImage() );
	sgNormalsCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgNormalsCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgNormalsCaster->SetOutputFilePath( "NormalsTexture" );

	Simplygon::spNormalCasterSettings sgNormalsCasterSettings = sgNormalsCaster->GetNormalCasterSettings();
	sgNormalsCasterSettings->SetMaterialChannel( "Normals" );
	sgNormalsCasterSettings->SetOpacityChannel( "Opacity" );
	sgNormalsCasterSettings->SetOpacityChannelComponent( Simplygon::EColorComponent::Alpha );
	sgNormalsCasterSettings->SetGenerateTangentSpaceNormals( true );
	sgNormalsCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgNormalsCasterSettings->SetDilation( 10 );
	sgNormalsCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::Interpolate );

	sgNormalsCaster->RunProcessing();
	std::string normalsTextureFilePath = sgNormalsCaster->GetOutputFilePath().c_str();
	
	// Setup and run the opacity material casting. Make sure there is no dilation or fill. 	
	printf("%s\n", "Setup and run the opacity material casting. Make sure there is no dilation or fill.");
	Simplygon::spOpacityCaster sgOpacityCaster = sg->CreateOpacityCaster();
	sgOpacityCaster->SetMappingImage( sgImpostorProcessor->GetMappingImage() );
	sgOpacityCaster->SetSourceMaterials( sgScene->GetMaterialTable() );
	sgOpacityCaster->SetSourceTextures( sgScene->GetTextureTable() );
	sgOpacityCaster->SetOutputFilePath( "OpacityTexture" );

	Simplygon::spOpacityCasterSettings sgOpacityCasterSettings = sgOpacityCaster->GetOpacityCasterSettings();
	sgOpacityCasterSettings->SetMaterialChannel( "Opacity" );
	sgOpacityCasterSettings->SetOpacityChannel( "Opacity" );
	sgOpacityCasterSettings->SetOpacityChannelComponent( Simplygon::EColorComponent::Alpha );
	sgOpacityCasterSettings->SetOutputImageFileFormat( Simplygon::EImageOutputFormat::PNG );
	sgOpacityCasterSettings->SetDilation( 0 );
	sgOpacityCasterSettings->SetFillMode( Simplygon::EAtlasFillMode::NoFill );
	sgOpacityCasterSettings->SetOutputPixelFormat( Simplygon::EPixelFormat::R8 );

	sgOpacityCaster->RunProcessing();
	std::string opacityTextureFilePath = sgOpacityCaster->GetOutputFilePath().c_str();
	
	// Update scene with new casted textures. 
	Simplygon::spMaterialTable sgMaterialTable = sg->CreateMaterialTable();
	Simplygon::spTextureTable sgTextureTable = sg->CreateTextureTable();
	Simplygon::spMaterial sgMaterial = sg->CreateMaterial();
	sgMaterial->SetName("OutputMaterial");
	Simplygon::spTexture sgDiffuseTexture = sg->CreateTexture();
	sgDiffuseTexture->SetName( "Diffuse" );
	sgDiffuseTexture->SetFilePath( diffuseTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgDiffuseTexture );

	Simplygon::spShadingTextureNode sgDiffuseTextureShadingNode = sg->CreateShadingTextureNode();
	sgDiffuseTextureShadingNode->SetTexCoordLevel( 0 );
	sgDiffuseTextureShadingNode->SetTextureName( "Diffuse" );

	sgMaterial->AddMaterialChannel( "Diffuse" );
	sgMaterial->SetShadingNetwork( "Diffuse", sgDiffuseTextureShadingNode );
	Simplygon::spTexture sgSpecularTexture = sg->CreateTexture();
	sgSpecularTexture->SetName( "Specular" );
	sgSpecularTexture->SetFilePath( specularTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgSpecularTexture );

	Simplygon::spShadingTextureNode sgSpecularTextureShadingNode = sg->CreateShadingTextureNode();
	sgSpecularTextureShadingNode->SetTexCoordLevel( 0 );
	sgSpecularTextureShadingNode->SetTextureName( "Specular" );

	sgMaterial->AddMaterialChannel( "Specular" );
	sgMaterial->SetShadingNetwork( "Specular", sgSpecularTextureShadingNode );
	Simplygon::spTexture sgNormalsTexture = sg->CreateTexture();
	sgNormalsTexture->SetName( "Normals" );
	sgNormalsTexture->SetFilePath( normalsTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgNormalsTexture );

	Simplygon::spShadingTextureNode sgNormalsTextureShadingNode = sg->CreateShadingTextureNode();
	sgNormalsTextureShadingNode->SetTexCoordLevel( 0 );
	sgNormalsTextureShadingNode->SetTextureName( "Normals" );

	sgMaterial->AddMaterialChannel( "Normals" );
	sgMaterial->SetShadingNetwork( "Normals", sgNormalsTextureShadingNode );
	Simplygon::spTexture sgOpacityTexture = sg->CreateTexture();
	sgOpacityTexture->SetName( "Opacity" );
	sgOpacityTexture->SetFilePath( opacityTextureFilePath.c_str() );
	sgTextureTable->AddTexture( sgOpacityTexture );

	Simplygon::spShadingTextureNode sgOpacityTextureShadingNode = sg->CreateShadingTextureNode();
	sgOpacityTextureShadingNode->SetTexCoordLevel( 0 );
	sgOpacityTextureShadingNode->SetTextureName( "Opacity" );

	sgMaterial->AddMaterialChannel( "Opacity" );
	sgMaterial->SetShadingNetwork( "Opacity", sgOpacityTextureShadingNode );
	sgMaterial->SetBlendMode(Simplygon::EMaterialBlendMode::Blend);

	sgMaterialTable->AddMaterial( sgMaterial );

	sgScene->GetTextureTable()->Clear();
	sgScene->GetMaterialTable()->Clear();
	sgScene->GetTextureTable()->Copy(sgTextureTable);
	sgScene->GetMaterialTable()->Copy(sgMaterialTable);
	Simplygon::spScene sgImpostorScene = sg->CreateScene();
	Simplygon::spGeometryData sgImpostorGeometry = sgImpostorProcessor->GetImpostorGeometryFromSingleView();
	sgImpostorScene->GetRootNode()->CreateChildMesh(sgImpostorGeometry);
	sgImpostorScene->GetMaterialTable()->Copy(sgScene->GetMaterialTable());
	sgImpostorScene->GetTextureTable()->Copy(sgScene->GetTextureTable());
	
	// Save processed scene. 	
	printf("%s\n", "Save processed scene.");
	SaveScene(sg, sgImpostorScene, "Output.glb");
	
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

	RunImpostorFromSingleView(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

