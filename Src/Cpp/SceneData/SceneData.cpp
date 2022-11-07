// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

#include <string>
#include <stdlib.h>
#include <filesystem>
#include <future>
#include "SimplygonLoader.h"


void SaveScene(Simplygon::ISimplygon* sg, Simplygon::spScene sgScene, const char* path)
{
	// Create scene exporter. 
	Simplygon::spSceneExporter sgSceneExporter = sg->CreateSceneExporter();
	std::string outputScenePath = std::string("output\\") + std::string("SceneData") + std::string("_") + std::string(path);
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
}

Simplygon::spGeometryData CreateCube(Simplygon::ISimplygon* sg, Simplygon::rid materialId)
{
	const int vertexCount = 8;
	const int triangleCount = 12;
	const int cornerCount = triangleCount * 3;
	int cornerIds[] = 
	{
		0, 1, 4, 
		4, 1, 5, 
		5, 1, 6, 
		1, 2, 6, 
		6, 2, 3, 
		6, 3, 7, 
		7, 3, 0, 
		7, 0, 4, 
		0, 2, 1, 
		0, 3, 2, 
		4, 5, 6, 
		4, 6, 7
	};
	float vertexCoordinates[] = 
	{
		1.0f, -1.0f, 1.0f, 
		1.0f, -1.0f, -1.0f, 
		-1.0f, -1.0f, -1.0f, 
		-1.0f, -1.0f, 1.0f, 
		1.0f, 1.0f, 1.0f, 
		1.0f, 1.0f, -1.0f, 
		-1.0f, 1.0f, -1.0f, 
		-1.0f, 1.0f, 1.0f
	};
	
	// Create the Geometry. All geometry data will be loaded into this object. 
	Simplygon::spGeometryData sgGeometryData = sg->CreateGeometryData();
	
	// Set vertex- and triangle-counts for the Geometry. 
	// NOTE: The number of vertices and triangles has to be set before vertex- and triangle-data is 
	// loaded into the GeometryData. 
	sgGeometryData->SetVertexCount(vertexCount);
	sgGeometryData->SetTriangleCount(triangleCount);
	
	// Array with vertex-coordinates. Will contain 3 real-values for each vertex in the geometry. 
	Simplygon::spRealArray sgCoords = sgGeometryData->GetCoords();
	
	// Array with triangle-data. Will contain 3 ids for each corner of each triangle, so the triangles 
	// know what vertices to use. 
	Simplygon::spRidArray sgVertexIds = sgGeometryData->GetVertexIds();
	
	// Add material data. Materials are assigned per triangle. 
	sgGeometryData->AddMaterialIds();
	Simplygon::spRidArray sgMaterialIds = sgGeometryData->GetMaterialIds();
	
	// Add vertex-coordinates array to the Geometry. 
	sgCoords->SetData(vertexCoordinates, vertexCount * 3);
	
	// Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
	// uses. 
	sgVertexIds->SetData(cornerIds, cornerCount);
	
	// Loop through all the triangles an assign material. 
	for (auto triangleIndex = 0U; triangleIndex < triangleCount; ++triangleIndex)
	{
		sgMaterialIds->SetItem((int)triangleIndex, materialId);
	}
	
	// Return created cube geometry data. 
	return sgGeometryData;
}

void RunExample(Simplygon::ISimplygon* sg)
{
	// Create a Simplygon scene. 
	Simplygon::spScene sgScene = sg->CreateScene();
	
	// Get material table from the scene. 
	Simplygon::spMaterialTable sgMaterialTable = sgScene->GetMaterialTable();
	
	// Create a red diffuse material and a red specular material. 
	Simplygon::spShadingColorNode sgRedColorNode = sg->CreateShadingColorNode();
	sgRedColorNode->SetColor(0.5f, 0.0f, 0.0f, 0.0f);
	Simplygon::spMaterial sgDiffuseRedMaterial = sg->CreateMaterial();
	sgDiffuseRedMaterial->SetName("red_diffuse");
	sgDiffuseRedMaterial->AddMaterialChannel(Simplygon::SG_MATERIAL_CHANNEL_DIFFUSE);
	sgDiffuseRedMaterial->SetShadingNetwork(Simplygon::SG_MATERIAL_CHANNEL_DIFFUSE, sgRedColorNode);
	Simplygon::spMaterial sgSpecularRedMaterial = sg->CreateMaterial();
	sgSpecularRedMaterial->SetName("red_specular");
	sgSpecularRedMaterial->AddMaterialChannel(Simplygon::SG_MATERIAL_CHANNEL_SPECULAR);
	sgSpecularRedMaterial->SetShadingNetwork(Simplygon::SG_MATERIAL_CHANNEL_SPECULAR, sgRedColorNode);
	
	// Add the materials to the material table. 
	Simplygon::rid diffuseMaterialId = sgMaterialTable->AddMaterial(sgDiffuseRedMaterial);
	Simplygon::rid specularMaterialId = sgMaterialTable->AddMaterial(sgSpecularRedMaterial);
	
	// Create two scene mesh objects. 
	Simplygon::spSceneMesh sgCubeMesh1 = sg->CreateSceneMesh();
	Simplygon::spSceneMesh sgCubeMesh2 = sg->CreateSceneMesh();
	
	// Set name on the scene meshes. 
	sgCubeMesh1->SetName("Cube1");
	sgCubeMesh1->SetOriginalName("Cube1");
	sgCubeMesh2->SetName("Cube2");
	sgCubeMesh2->SetOriginalName("Cube2");
	
	// Create cube geometry. 
	Simplygon::spGeometryData sgGeometryDataCube1 = CreateCube(sg, diffuseMaterialId);
	Simplygon::spGeometryData sgGeometryDataCube2 = CreateCube(sg, specularMaterialId);
	sgCubeMesh1->SetGeometry(sgGeometryDataCube1);
	sgCubeMesh2->SetGeometry(sgGeometryDataCube2);
	
	// Add the two scene meshes as child to the root node of the scene. 
	sgScene->GetRootNode()->AddChild(sgCubeMesh1);
	sgScene->GetRootNode()->AddChild(sgCubeMesh2);
	
	// Create a transform node. 
	Simplygon::spTransform3 sgTransform = sg->CreateTransform3();
	
	// Set the transform to use premultiply. 
	sgTransform->PreMultiply();
	
	// Add 45 degree rotation and 5 units translation. 
	sgTransform->AddRotation(0.785f, 0.0f, 1.0f, 0.0f);
	sgTransform->AddRotation(0.785f, 1.0f, 0.0f, 0.0f);
	sgTransform->AddTranslation(0.0f, 5.0f, 0.0f);
	
	// Apply transformation on the second cube node. 
	Simplygon::spMatrix4x4 sgTransformMatrix = sgTransform->GetMatrix();
	sgCubeMesh2->GetRelativeTransform()->DeepCopy(sgTransformMatrix);
	
	// Save example scene to output.obj. 	
	printf("%s\n", "Save example scene to output.obj.");
	SaveScene(sg, sgScene, "Output.obj");
	
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

	RunExample(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

