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
	std::string outputScenePath = std::string("output\\") + std::string("ReductionWithGeomorph") + std::string("_") + std::string(path);
	sgSceneExporter->SetExportFilePath(outputScenePath.c_str());
	sgSceneExporter->SetScene(sgScene);
	
	// Run scene exporter. 
	auto exportResult = sgSceneExporter->Run();
	if (Simplygon::Failed(exportResult))
	{
		throw std::exception("Failed to save scene.");
	}
}

void ArrayBlend(Simplygon::spRealArray targetArray, Simplygon::spRealArray origArray, Simplygon::spRealArray morphArray, float blend)
{
	auto itemCount = targetArray->GetItemCount();
	
	// Morph the geometry 
	for (auto i = 0U; i < itemCount; ++i)
	{
		float origValue = origArray->GetItem((int)i);
		float morphValue = morphArray->GetItem((int)i);
		float blendResult = morphValue * blend + origValue * (1.0f-blend);
		targetArray->SetItem((int)i, blendResult);
	}
}

void MorphGeometry(Simplygon::spSceneMesh originalSceneMesh, Simplygon::spSceneMesh morphSceneMesh, Simplygon::spSceneMesh blendSceneMesh, float blend)
{
	Simplygon::spGeometryData originalGeometry = originalSceneMesh->GetGeometry();
	Simplygon::spGeometryData morphGeometry = morphSceneMesh->GetGeometry();
	Simplygon::spGeometryData blendGeometry = blendSceneMesh->GetGeometry();
	Simplygon::spRealArray origArray = originalGeometry->GetCoords();
	Simplygon::spRealArray morphArray = morphGeometry->GetCoords();
	Simplygon::spRealArray targetArray = blendGeometry->GetCoords();
	Simplygon::spRidArray origVertexIdsArray = originalGeometry->GetVertexIds();
	Simplygon::spRidArray morphVertexIdsArray = morphGeometry->GetVertexIds();
	Simplygon::spRidArray targetVertexIdsArray = blendGeometry->GetVertexIds();
	auto itemCount = targetVertexIdsArray->GetItemCount();
	
	// For each corner: 
	for (auto c = 0U; c < itemCount; ++c)
	{
		auto origVertexId = origVertexIdsArray->GetItem((int)c);
		auto morphVertexId = morphVertexIdsArray->GetItem((int)c);
		auto targetVertexId = targetVertexIdsArray->GetItem((int)c);
		
		// For each x,z,y value in each coordinate: 
		for (auto d = 0U; d < 3; ++d)
		{
			int dInt = (int)d;
			float origValue = origArray->GetItem(origVertexId * 3 + dInt);
			float morphValue = morphArray->GetItem(morphVertexId * 3 + dInt);
			float blendResult = morphValue * blend + origValue * (1.0f-blend);
			targetArray->SetItem(targetVertexId * 3 + dInt, blendResult);
		}
	}
	
	// Morph the UVs 
	targetArray = blendGeometry->GetTexCoords(0);
	origArray = originalGeometry->GetTexCoords(0);
	morphArray = morphGeometry->GetTexCoords(0);
	ArrayBlend(targetArray, origArray, morphArray, blend);
	
	// Morph the Normals 
	targetArray = blendGeometry->GetNormals();
	origArray = originalGeometry->GetNormals();
	morphArray = morphGeometry->GetNormals();
	ArrayBlend(targetArray, origArray, morphArray, blend);
}

void RunReductionWithGeomorph(Simplygon::ISimplygon* sg)
{
	// Load scene to process. 	
	printf("%s\n", "Load scene to process.");
	Simplygon::spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
	
	// Create the reduction processor. 
	Simplygon::spReductionProcessor sgReductionProcessor = sg->CreateReductionProcessor();
	
	// Select Mesh Nodes 
	int selectionSetId = sgScene->SelectNodes("ISceneMesh");
	
	// Keep a copy of the original scene to be able to blend between this and the morph geometry. 
	Simplygon::spScene originalScene = sgScene->NewCopy();
	Simplygon::spScene blendScene = sgScene->NewCopy();
	sgReductionProcessor->SetScene( sgScene );
	Simplygon::spReductionSettings sgReductionSettings = sgReductionProcessor->GetReductionSettings();
	
	// Set reduction target to triangle ratio with a ratio of 50%. Enable geomorph creation and 
	// disable symmetry quad retriangulator. 
	sgReductionSettings->SetReductionTargets( Simplygon::EStopCondition::All, true, false, false, false );
	sgReductionSettings->SetReductionTargetTriangleRatio( 0.5f );
	sgReductionSettings->SetCreateGeomorphGeometry( true );
	sgReductionSettings->SetUseSymmetryQuadRetriangulator( false );
	Simplygon::spRepairSettings sgRepairSettings = sgReductionProcessor->GetRepairSettings();
	
	// Disable TJunctionRemover. 
	sgRepairSettings->SetWeldDist( 0.0f );
	sgRepairSettings->SetUseTJunctionRemover( false );
	Simplygon::spNormalCalculationSettings sgNormalCalculationSettings = sgReductionProcessor->GetNormalCalculationSettings();
	
	// Replace normals. 
	sgNormalCalculationSettings->SetReplaceNormals( true );
	sgNormalCalculationSettings->SetHardEdgeAngle( 60.0f );
	
	// Start the reduction process. 	
	printf("%s\n", "Start the reduction process.");
	sgReductionProcessor->RunProcessing();
	auto set = sgScene->GetSelectionSetTable()->GetSelectionSet(selectionSetId);
	
	// Selection set to collect geomorph nodes in scene. 
	Simplygon::spSelectionSet geomorphSelectionSet = sg->CreateSelectionSet();
	
	// Morph the geometry 
	for (auto blendValue = 0; blendValue < 11; ++blendValue)
	{
		auto geometryCount = set->GetItemCount();
		
		// The LOD scene contains morphed geometries, placed as children of their respective reduced 
		// geometry mesh node. 
		// The name of a morph mesh node is the same as its parent, with "_geomorph" added. 
		for (auto geomIndex = 0U; geomIndex < geometryCount; ++geomIndex)
		{
			auto guid = set->GetItem(geomIndex);
			Simplygon::spSceneNode lodSceneNode = sgScene->GetNodeByGUID(guid);
			Simplygon::spSceneNode originalSceneNode = originalScene->GetNodeByGUID(guid);
			Simplygon::spSceneNode blendSceneNode = blendScene->GetNodeByGUID(guid);
			Simplygon::spSceneMesh lodSceneMesh = Simplygon::spSceneMesh::SafeCast(lodSceneNode);
			Simplygon::spSceneMesh originalSceneMesh = Simplygon::spSceneMesh::SafeCast(originalSceneNode);
			Simplygon::spSceneMesh blendSceneMesh = Simplygon::spSceneMesh::SafeCast(blendSceneNode);
			auto childCount = lodSceneNode->GetChildCount();
			for (auto childIndex = 0U; childIndex < childCount; ++childIndex)
			{
				Simplygon::spSceneNode childNode = lodSceneNode->GetChild((int)childIndex);
				auto childName = childNode->GetName();
				std::string childNodeString = std::string(childName.c_str());
				auto name = lodSceneNode->GetName();
				std::string geomorphName = std::string("_geomorph");
				if (childNodeString.find(geomorphName) != std::string::npos )
				{
					Simplygon::spSceneMesh morphSceneMesh = Simplygon::spSceneMesh::SafeCast(childNode);
					auto morphGUID = morphSceneMesh->GetNodeGUID();
					geomorphSelectionSet->AddItem(morphGUID);
					MorphGeometry(originalSceneMesh, morphSceneMesh, blendSceneMesh, blendValue*0.1f);
				}
			}
		}
		std::string outputName = std::string("Geomorph_BlendedGeometry") + std::string(std::to_string(blendValue)) + std::string(".obj");
		
		// Save processed scene. 		
		printf("%s\n", "Save processed scene.");
		SaveScene(sg, blendScene, outputName.c_str());
	}
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

	RunReductionWithGeomorph(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

