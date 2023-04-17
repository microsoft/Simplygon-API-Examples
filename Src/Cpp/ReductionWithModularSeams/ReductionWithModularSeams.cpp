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
	std::string outputScenePath = std::string("output\\") + std::string("ReductionWithModularSeams") + std::string("_") + std::string(path);
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

Simplygon::spGeometryDataCollection ExtractGeometriesInScene(Simplygon::ISimplygon* sg, Simplygon::spScene sgModularAssetsScene)
{
	// Extract all geometries in the scene into individual geometries 
	Simplygon::spGeometryDataCollection sgGeometryDataCollection = sg->CreateGeometryDataCollection();
	int id = sgModularAssetsScene->SelectNodes("ISceneMesh");
	auto set = sgModularAssetsScene->GetSelectionSetTable()->GetSelectionSet(id);
	auto geometryCount = set->GetItemCount();
	for (auto geomIndex = 0U; geomIndex < geometryCount; ++geomIndex)
	{
		auto guid = set->GetItem(geomIndex);
		Simplygon::spSceneNode sgSceneNode = sgModularAssetsScene->GetNodeByGUID(guid);
		Simplygon::spSceneMesh sgSceneMesh = Simplygon::spSceneMesh::SafeCast(sgSceneNode);
		Simplygon::spGeometryData geom = sgSceneMesh->GetGeometry();
		sgGeometryDataCollection->AddGeometryData(geom);
	}
	return sgGeometryDataCollection;
}

void DebugModularSeams(Simplygon::ISimplygon* sg, bool outputDebugInfo, Simplygon::spModularSeams sgModularSeams)
{
	if (outputDebugInfo)
	{
		// Optional but helpful to be able to see what the analyzer found. 
		// Each unique modular seam can be extracted as a geometry. If the analyzer ran with 
		// IsTranslationIndependent=false then the seam geometry should be exactly located at the same 
		// place as the modular seams in the original scene. 
		// Each modular seam also has a string array with all the names of the geometries that have that 
		// specific modular seam. 
		auto seamCount = sgModularSeams->GetModularSeamCount();
		for (auto seamIndex = 0U; seamIndex < seamCount; ++seamIndex)
		{
			Simplygon::spGeometryData debugGeom = sgModularSeams->NewDebugModularSeamGeometry((int)seamIndex);
			Simplygon::spStringArray geometryNames = sgModularSeams->NewModularSeamGeometryStringArray((int)seamIndex);
			

			Simplygon::spScene debugScene = sg->CreateScene();
			debugScene->GetRootNode()->CreateChildMesh(debugGeom);
			std::string fileName = std::string("output\\") + std::string("ReductionWithModularSeams_seam_") + std::string(std::to_string(seamIndex)) + std::string(".obj");
			

			Simplygon::spSceneExporter sgSceneExporter = sg->CreateSceneExporter();
			sgSceneExporter->SetExportFilePath( fileName.c_str() );
			sgSceneExporter->SetScene( debugScene );
			sgSceneExporter->Run();
			

			auto vertexCount = debugGeom->GetVertexCount();
			auto geometryNamesCount = geometryNames->GetItemCount();
			std::string outputText = std::string("Seam ") + std::string(std::to_string(seamIndex)) + std::string(" consists of ") + std::string(std::to_string(vertexCount)) + std::string(" vertices and is shared among ") + std::string(std::to_string(geometryNamesCount)) + std::string(" geometries:");
			printf("%s\n", outputText.c_str());
			for (auto geomIndex = 0U; geomIndex < geometryNamesCount; ++geomIndex)
			{
				Simplygon::spString geometryName = geometryNames->GetItem((int)geomIndex);
				std::string geometryNameOutput = std::string(" geom ") + std::string(std::to_string(geomIndex)) + std::string(": ") + std::string(geometryName.c_str());
				printf("%s\n", geometryNameOutput.c_str());
			}
		}
	}
}

void ModifyReductionSettings(Simplygon::spReductionSettings sgReductionSettings, float triangleRatio, float maxDeviation)
{
	sgReductionSettings->SetKeepSymmetry( true );
	sgReductionSettings->SetUseAutomaticSymmetryDetection( true );
	sgReductionSettings->SetUseHighQualityNormalCalculation( true );
	sgReductionSettings->SetReductionHeuristics( Simplygon::EReductionHeuristics::Consistent );
	
	// The importances can be changed here to allow the features to be weighed differently both during 
	// regular reduction and during the analyzing of modular seam 
	sgReductionSettings->SetEdgeSetImportance( 1.0f );
	sgReductionSettings->SetGeometryImportance( 1.0f );
	sgReductionSettings->SetGroupImportance( 1.0f );
	sgReductionSettings->SetMaterialImportance( 1.0f );
	sgReductionSettings->SetShadingImportance( 1.0f );
	sgReductionSettings->SetSkinningImportance( 1.0f );
	sgReductionSettings->SetTextureImportance( 1.0f );
	sgReductionSettings->SetVertexColorImportance( 1.0f );
	
	// The reduction targets below are only used for the regular reduction, not the modular seam 
	// analyzer 
	sgReductionSettings->SetReductionTargetTriangleRatio( triangleRatio );
	sgReductionSettings->SetReductionTargetMaxDeviation( maxDeviation );
	sgReductionSettings->SetReductionTargets(Simplygon::EStopCondition::All, true, false, true, false);
}

void GenerateModularSeams(Simplygon::ISimplygon* sg, Simplygon::spScene sgModularAssetsScene)
{
	Simplygon::spGeometryDataCollection sgGeometryDataCollection = ExtractGeometriesInScene(sg, sgModularAssetsScene);
	
	// Figure out a small value in relation to the scene that will be the tolerance for the modular seams 
	// if a coordinate is moved a distance smaller than the tolerance, then it is regarded as the same 
	// coordinate so two vertices are the at the same place if the distance between them is smaller than 
	// radius * smallValue 
	sgModularAssetsScene->CalculateExtents();
	float smallValue = 0.0001f;
	float sceneRadius = sgModularAssetsScene->GetRadius();
	float tolerance = sceneRadius * smallValue;
	Simplygon::spReductionSettings sgReductionSettings = sg->CreateReductionSettings();
	
	// The triangleRatio and maxDeviation are not important here and will not be used, only the 
	// relative importances and settings 
	ModifyReductionSettings(sgReductionSettings, 0.0f, 0.0f);
	
	// Create the modular seam analyzer. 
	Simplygon::spModularSeamAnalyzer sgModularSeamAnalyzer = sg->CreateModularSeamAnalyzer();
	sgModularSeamAnalyzer->SetTolerance( tolerance );
	sgModularSeamAnalyzer->SetIsTranslationIndependent( false );
	auto modularGeometryCount = sgGeometryDataCollection->GetItemCount();
	
	// Add the geometries to the analyzer 
	for (auto modularGeometryId = 0U; modularGeometryId < modularGeometryCount; ++modularGeometryId)
	{
		auto modularGeometryObject = sgGeometryDataCollection->GetItemAsObject(modularGeometryId);
		Simplygon::spGeometryData modularGeometry = Simplygon::spGeometryData::SafeCast(modularGeometryObject);
		sgModularSeamAnalyzer->AddGeometry(modularGeometry);
	}
	
	// The analyzer needs to know the different reduction settings importances and such because it 
	// runs the reduction as far as possible for all the seams and stores the order and max deviations 
	// for future reductions of assets with the same seams 
	sgModularSeamAnalyzer->Analyze(sgReductionSettings);
	
	// Fetch the modular seams. These can be stored to file and used later 
	Simplygon::spModularSeams sgModularSeams = sgModularSeamAnalyzer->GetModularSeams();
	std::string modularSeamsPath = std::string("output\\") + std::string("ModularAssets.modseam");
	sgModularSeams->SaveToFile(modularSeamsPath.c_str());
}

Simplygon::spModularSeams LoadModularSeams(Simplygon::ISimplygon* sg)
{
	// Load pre-generated modular seams 
	Simplygon::spModularSeams sgModularSeams = sg->CreateModularSeams();
	std::string modularSeamsPath = std::string("output\\") + std::string("ModularAssets.modseam");
	sgModularSeams->LoadFromFile(modularSeamsPath.c_str());
	return sgModularSeams;
}

void RunReduction(Simplygon::ISimplygon* sg, Simplygon::spScene sgModularAssetsScene, Simplygon::spModularSeams sgModularSeams, float triangleRatio, float maxDeviation, float modularSeamReductionRatio, float modularSeamMaxDeviation)
{
	Simplygon::spGeometryDataCollection sgGeometryDataCollection = ExtractGeometriesInScene(sg, sgModularAssetsScene);
	auto modularGeometryCount = sgGeometryDataCollection->GetItemCount();
	
	// Add the geometries to the analyzer 
	for (auto modularGeometryId = 0U; modularGeometryId < modularGeometryCount; ++modularGeometryId)
	{
		auto modularGeometryObject = sgGeometryDataCollection->GetItemAsObject(modularGeometryId);
		Simplygon::spGeometryData modularGeometry = Simplygon::spGeometryData::SafeCast(modularGeometryObject);
		
		// Run reduction on each geometry individually, 
		// feed the modular seams into the reducer with the ModularSeamSettings 
		// so the modular seams are reduced identically and are untouched by the rest of the 
		// geometry reduction 
		Simplygon::spScene sgSingleAssetScene = sgModularAssetsScene->NewCopy();
		
		// Remove all the geometries but keep any textures, materials etc. 
		sgSingleAssetScene->RemoveSceneNodes();
		
		// Add just a copy of the current geometry to the scene 
		Simplygon::spGeometryData modularGeometryCopy = modularGeometry->NewCopy(true);
		Simplygon::spSceneNode sgRootNode = sgSingleAssetScene->GetRootNode();
		Simplygon::spSceneMesh sgSceneMesh = sgRootNode->CreateChildMesh(modularGeometryCopy);
		

		Simplygon::spReductionProcessor sgReductionProcessor = sg->CreateReductionProcessor();
		sgReductionProcessor->SetScene( sgSingleAssetScene );
		Simplygon::spReductionSettings sgReductionSettings = sgReductionProcessor->GetReductionSettings();
		Simplygon::spModularSeamSettings sgModularSeamSettings = sgReductionProcessor->GetModularSeamSettings();
		
		// Set the same reduction (importance) settings as the modular seam analyzer for consistent 
		// quality 
		ModifyReductionSettings(sgReductionSettings, triangleRatio, maxDeviation);
		sgModularSeamSettings->SetReductionRatio( modularSeamReductionRatio );
		sgModularSeamSettings->SetMaxDeviation( modularSeamMaxDeviation );
		sgModularSeamSettings->SetStopCondition( Simplygon::EStopCondition::All );
		sgModularSeamSettings->SetModularSeams( sgModularSeams );
		

		sgReductionProcessor->RunProcessing();
		Simplygon::spString geomName = modularGeometry->GetName();
		std::string outputName = std::string(geomName) + std::string(".obj");
		SaveScene(sg, sgSingleAssetScene, outputName.c_str());
	}
}

void RunReductionWithModularSeams(Simplygon::ISimplygon* sg)
{
	// Set reduction targets. Stop condition is set to 'All' 
	float triangleRatio = 0.5f;
	float maxDeviation = 0.0f;
	float modularSeamReductionRatio = 0.75f;
	float modularSeamMaxDeviation = 0.0f;
	
	// Load a scene that has a few modular assets in it as different scene meshes. 
	Simplygon::spScene sgModularAssetsScene = LoadScene(sg, "../../../Assets/ModularAssets/ModularAssets.obj");
	bool generateNewSeams = true;
	if (generateNewSeams)
	{
		GenerateModularSeams(sg, sgModularAssetsScene);
	}
	Simplygon::spModularSeams sgModularSeams = LoadModularSeams(sg);
	DebugModularSeams(sg, true, sgModularSeams);
	
	// Run the reduction. The seams are reduced identically and the rest of the geometries are reduced 
	// like normal 
	RunReduction(sg, sgModularAssetsScene, sgModularSeams, triangleRatio, maxDeviation, modularSeamReductionRatio, modularSeamMaxDeviation);
	
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

	RunReductionWithModularSeams(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

