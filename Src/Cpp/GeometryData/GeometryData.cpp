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
	std::string outputScenePath = std::string("output\\") + std::string("GeometryData") + std::string("_") + std::string(path);
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

void RunExample1(Simplygon::ISimplygon* sg)
{
	// 4 separate triangles, with 3 vertices each and 3 sets of UV coordinates each. They make up 2 
	// quads, where each quad has the same set of UV coordinates. 
	const int vertexCount = 12;
	const int triangleCount = 4;
	const int cornerCount = triangleCount * 3;
	
	// 4 triangles x 3 indices ( or 3 corners ). 
	int cornerIds[] = 
	{
		0, 1, 2, 
		3, 4, 5, 
		6, 7, 8, 
		9, 10, 11
	};
	
	// 12 vertices with values for the x, y and z coordinates. 
	float vertexCoordinates[] = 
	{
		0.0f, 0.0f, 0.0f, 
		1.0f, 0.0f, 0.0f, 
		1.0f, 1.0f, 0.0f, 
		1.0f, 1.0f, 0.0f, 
		0.0f, 1.0f, 0.0f, 
		0.0f, 0.0f, 0.0f, 
		1.0f, 0.0f, 0.0f, 
		2.0f, 0.0f, 0.0f, 
		2.0f, 1.0f, 0.0f, 
		2.0f, 1.0f, 0.0f, 
		1.0f, 1.0f, 0.0f, 
		1.0f, 0.0f, 0.0f
	};
	
	// UV coordinates for all 12 corners. 
	float textureCoordinates[] = 
	{
		0.0f, 0.0f, 
		1.0f, 0.0f, 
		1.0f, 1.0f, 
		1.0f, 1.0f, 
		0.0f, 1.0f, 
		0.0f, 0.0f, 
		0.0f, 0.0f, 
		1.0f, 0.0f, 
		1.0f, 1.0f, 
		1.0f, 1.0f, 
		0.0f, 1.0f, 
		0.0f, 0.0f
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
	
	// Must add texture channel before adding data to it. 
	sgGeometryData->AddTexCoords(0);
	Simplygon::spRealArray sgTexcoords = sgGeometryData->GetTexCoords(0);
	
	// Add vertex-coordinates array to the Geometry. 
	sgCoords->SetData(vertexCoordinates, vertexCount * 3);
	
	// Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
	// uses. 
	sgVertexIds->SetData(cornerIds, cornerCount);
	
	// Add texture-coordinates array to the Geometry. 
	sgTexcoords->SetData(textureCoordinates, cornerCount * 2);
	
	// Create a scene and a SceneMesh node with the geometry. 
	Simplygon::spScene sgScene = sg->CreateScene();
	Simplygon::spSceneMesh sgSceneMesh = sg->CreateSceneMesh();
	sgSceneMesh->SetName("Mesh1");
	sgSceneMesh->SetGeometry(sgGeometryData);
	sgScene->GetRootNode()->AddChild(sgSceneMesh);
	
	// Save example1 scene to Example1.obj. 	
	printf("%s\n", "Save example1 scene to Example1.obj.");
	SaveScene(sg, sgScene, "Example1.obj");
	
	// Check log for any warnings or errors. 	
	printf("%s\n", "Check log for any warnings or errors.");
	CheckLog(sg);
}

void RunExample2(Simplygon::ISimplygon* sg)
{
	// Same as RunExample1, but now the vertices are shared among the triangles. 
	const int vertexCount = 6;
	const int triangleCount = 4;
	const int cornerCount = triangleCount * 3;
	
	// 4 triangles x 3 indices ( or 3 corners ). 
	int cornerIds[] = 
	{
		0, 1, 2, 
		0, 2, 3, 
		1, 4, 5, 
		1, 5, 2
	};
	
	// 6 vertices with values for the x, y and z coordinates. 
	float vertexCoordinates[] = 
	{
		0.0f, 0.0f, 0.0f, 
		1.0f, 0.0f, 0.0f, 
		1.0f, 1.0f, 0.0f, 
		0.0f, 1.0f, 0.0f, 
		2.0f, 0.0f, 0.0f, 
		2.0f, 1.0f, 0.0f
	};
	
	// UV coordinates for all 12 corners. 
	float textureCoordinates[] = 
	{
		0.0f, 0.0f, 
		1.0f, 0.0f, 
		1.0f, 1.0f, 
		1.0f, 1.0f, 
		0.0f, 1.0f, 
		0.0f, 0.0f, 
		0.0f, 0.0f, 
		1.0f, 0.0f, 
		1.0f, 1.0f, 
		1.0f, 1.0f, 
		0.0f, 1.0f, 
		0.0f, 0.0f
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
	
	// Must add texture channel before adding data to it. 
	sgGeometryData->AddTexCoords(0);
	Simplygon::spRealArray sgTexcoords = sgGeometryData->GetTexCoords(0);
	
	// Add vertex-coordinates array to the Geometry. 
	sgCoords->SetData(vertexCoordinates, vertexCount * 3);
	
	// Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
	// uses. 
	sgVertexIds->SetData(cornerIds, cornerCount);
	
	// Add texture-coordinates array to the Geometry. 
	sgTexcoords->SetData(textureCoordinates, cornerCount * 2);
	
	// Create a scene and a SceneMesh node with the geometry. 
	Simplygon::spScene sgScene = sg->CreateScene();
	Simplygon::spSceneMesh sgSceneMesh = sg->CreateSceneMesh();
	sgSceneMesh->SetName("Mesh2");
	sgSceneMesh->SetGeometry(sgGeometryData);
	sgScene->GetRootNode()->AddChild(sgSceneMesh);
	
	// Save example2 scene to Example2.obj. 	
	printf("%s\n", "Save example2 scene to Example2.obj.");
	SaveScene(sg, sgScene, "Example2.obj");
	
	// Check log for any warnings or errors. 	
	printf("%s\n", "Check log for any warnings or errors.");
	CheckLog(sg);
}

void RunExample3(Simplygon::ISimplygon* sg)
{
	// Same as RunExample1, but now all corner-data is stored as vertex-data, in a packet format. 
	// Since the 2 vertices where the quads meet don't share same UV, they will be 2 separate vertices, 
	// so 4 vertices / quad as opposed to 6 / quad in RunExample1, and only 6 for whole mesh in 
	// RunExample2. 
	const int vertexCount = 8;
	const int triangleCount = 4;
	const int cornerCount = triangleCount * 3;
	
	// 4 triangles x 3 indices ( or 3 corners ). 
	int cornerIds[] = 
	{
		0, 1, 2, 
		0, 2, 3, 
		4, 5, 6, 
		4, 6, 7
	};
	
	// 8 vertices with values for the x, y and z coordinates. 
	float vertexCoordinates[] = 
	{
		0.0f, 0.0f, 0.0f, 
		1.0f, 0.0f, 0.0f, 
		1.0f, 1.0f, 0.0f, 
		0.0f, 1.0f, 0.0f, 
		1.0f, 0.0f, 0.0f, 
		2.0f, 0.0f, 0.0f, 
		2.0f, 1.0f, 0.0f, 
		1.0f, 1.0f, 0.0f
	};
	
	// UV coordinates for all 8 vertices. 
	float textureCoordinates[] = 
	{
		0.0f, 0.0f, 
		1.0f, 0.0f, 
		1.0f, 1.0f, 
		0.0f, 1.0f, 
		0.0f, 0.0f, 
		1.0f, 0.0f, 
		1.0f, 1.0f, 
		0.0f, 1.0f
	};
	
	// Create the PackedGeometry. All geometry data will be loaded into this object. 
	Simplygon::spPackedGeometryData sgPackedGeometryData = sg->CreatePackedGeometryData();
	
	// Set vertex- and triangle-counts for the Geometry. 
	// NOTE: The number of vertices and triangles has to be set before vertex- and triangle-data is 
	// loaded into the GeometryData. 
	sgPackedGeometryData->SetVertexCount(vertexCount);
	sgPackedGeometryData->SetTriangleCount(triangleCount);
	
	// Array with vertex-coordinates. Will contain 3 real-values for each vertex in the geometry. 
	Simplygon::spRealArray sgCoords = sgPackedGeometryData->GetCoords();
	
	// Array with triangle-data. Will contain 3 ids for each corner of each triangle, so the triangles 
	// know what vertices to use. 
	Simplygon::spRidArray sgVertexIds = sgPackedGeometryData->GetVertexIds();
	
	// Must add texture channel before adding data to it. 
	sgPackedGeometryData->AddTexCoords(0);
	Simplygon::spRealArray sgTexcoords = sgPackedGeometryData->GetTexCoords(0);
	
	// Add vertex-coordinates array to the Geometry. 
	sgCoords->SetData(vertexCoordinates, vertexCount * 3);
	
	// Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
	// uses. 
	sgVertexIds->SetData(cornerIds, cornerCount);
	
	// Add texture-coordinates array to the Geometry. 
	sgTexcoords->SetData(textureCoordinates, cornerCount * 2);
	
	// Create a scene and a SceneMesh node with the geometry. 
	Simplygon::spScene sgScene = sg->CreateScene();
	Simplygon::spSceneMesh sgSceneMesh = sg->CreateSceneMesh();
	sgSceneMesh->SetName("Mesh3");
	Simplygon::spGeometryData sgGeometryData = sgPackedGeometryData->NewUnpackedCopy();
	sgSceneMesh->SetGeometry(sgGeometryData);
	sgScene->GetRootNode()->AddChild(sgSceneMesh);
	
	// Save example3 scene to Example3.obj. 	
	printf("%s\n", "Save example3 scene to Example3.obj.");
	SaveScene(sg, sgScene, "Example3.obj");
	
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

	RunExample1(sg);
	RunExample2(sg);
	RunExample3(sg);

	Simplygon::Deinitialize(sg);

	return 0;
}

