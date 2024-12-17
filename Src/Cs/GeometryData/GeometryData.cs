// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

using System;
using System.IO;
using System.Threading.Tasks;

public class Program
{
    static void SaveScene(Simplygon.ISimplygon sg, Simplygon.spScene sgScene, string path)
    {
        // Create scene exporter. 
        using Simplygon.spSceneExporter sgSceneExporter = sg.CreateSceneExporter();
        string outputScenePath = string.Join("", new string[] { "output\\", "GeometryData", "_", path });
        sgSceneExporter.SetExportFilePath(outputScenePath);
        sgSceneExporter.SetScene(sgScene);
        
        // Run scene exporter. 
        var exportResult = sgSceneExporter.Run();
        if (Simplygon.Simplygon.Failed(exportResult))
        {
            throw new System.Exception("Failed to save scene.");
        }
    }

    static void CheckLog(Simplygon.ISimplygon sg)
    {
        // Check if any errors occurred. 
        bool hasErrors = sg.ErrorOccurred();
        if (hasErrors)
        {
            Simplygon.spStringArray errors = sg.CreateStringArray();
            sg.GetErrorMessages(errors);
            var errorCount = errors.GetItemCount();
            if (errorCount > 0)
            {
                Console.WriteLine("CheckLog: Errors:");
                for (uint errorIndex = 0; errorIndex < errorCount; ++errorIndex)
                {
                    string errorString = errors.GetItem((int)errorIndex);
                    Console.WriteLine(errorString);
                }
                sg.ClearErrorMessages();
            }
        }
        else
        {
            Console.WriteLine("CheckLog: No errors.");
        }
        
        // Check if any warnings occurred. 
        bool hasWarnings = sg.WarningOccurred();
        if (hasWarnings)
        {
            Simplygon.spStringArray warnings = sg.CreateStringArray();
            sg.GetWarningMessages(warnings);
            var warningCount = warnings.GetItemCount();
            if (warningCount > 0)
            {
                Console.WriteLine("CheckLog: Warnings:");
                for (uint warningIndex = 0; warningIndex < warningCount; ++warningIndex)
                {
                    string warningString = warnings.GetItem((int)warningIndex);
                    Console.WriteLine(warningString);
                }
                sg.ClearWarningMessages();
            }
        }
        else
        {
            Console.WriteLine("CheckLog: No warnings.");
        }
        
        // Error out if Simplygon has errors. 
        if (hasErrors)
        {
            throw new System.Exception("Processing failed with an error");
        }
    }

    static void RunExample1(Simplygon.ISimplygon sg)
    {
        // 4 separate triangles, with 3 vertices each and 3 sets of UV coordinates each. They make up 2 
        // quads, where each quad has the same set of UV coordinates. 
        const int vertexCount = 12;
        const int triangleCount = 4;
        const int cornerCount = triangleCount * 3;
        
        // 4 triangles x 3 indices ( or 3 corners ). 
        int[] cornerIds = new int[]
        {
            0, 1, 2, 
            3, 4, 5, 
            6, 7, 8, 
            9, 10, 11
        };
        
        // 12 vertices with values for the x, y and z coordinates. 
        float[] vertexCoordinates = new float[]
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
        float[] textureCoordinates = new float[]
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
        Simplygon.spGeometryData sgGeometryData = sg.CreateGeometryData();
        
        // Set vertex- and triangle-counts for the Geometry. 
        // NOTE: The number of vertices and triangles has to be set before vertex- and triangle-data is 
        // loaded into the GeometryData. 
        sgGeometryData.SetVertexCount(vertexCount);
        sgGeometryData.SetTriangleCount(triangleCount);
        
        // Array with vertex-coordinates. Will contain 3 real-values for each vertex in the geometry. 
        Simplygon.spRealArray sgCoords = sgGeometryData.GetCoords();
        
        // Array with triangle-data. Will contain 3 ids for each corner of each triangle, so the triangles 
        // know what vertices to use. 
        Simplygon.spRidArray sgVertexIds = sgGeometryData.GetVertexIds();
        
        // Must add texture channel before adding data to it. 
        sgGeometryData.AddTexCoords(0);
        Simplygon.spRealArray sgTexcoords = sgGeometryData.GetTexCoords(0);
        
        // Add vertex-coordinates array to the Geometry. 
        sgCoords.SetData(vertexCoordinates, vertexCount * 3);
        
        // Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
        // uses. 
        sgVertexIds.SetData(cornerIds, cornerCount);
        
        // Add texture-coordinates array to the Geometry. 
        sgTexcoords.SetData(textureCoordinates, cornerCount * 2);
        
        // Create a scene and a SceneMesh node with the geometry. 
        Simplygon.spScene sgScene = sg.CreateScene();
        Simplygon.spSceneMesh sgSceneMesh = sg.CreateSceneMesh();
        sgSceneMesh.SetName("Mesh1");
        sgSceneMesh.SetGeometry(sgGeometryData);
        sgScene.GetRootNode().AddChild(sgSceneMesh);

        
        // Save example1 scene to Example1.obj.         
        Console.WriteLine("Save example1 scene to Example1.obj.");
        SaveScene(sg, sgScene, "Example1.obj");
        
        // Check log for any warnings or errors.         
        Console.WriteLine("Check log for any warnings or errors.");
        CheckLog(sg);
    }

    static void RunExample2(Simplygon.ISimplygon sg)
    {
        // Same as RunExample1, but now the vertices are shared among the triangles. 
        const int vertexCount = 6;
        const int triangleCount = 4;
        const int cornerCount = triangleCount * 3;
        
        // 4 triangles x 3 indices ( or 3 corners ). 
        int[] cornerIds = new int[]
        {
            0, 1, 2, 
            0, 2, 3, 
            1, 4, 5, 
            1, 5, 2
        };
        
        // 6 vertices with values for the x, y and z coordinates. 
        float[] vertexCoordinates = new float[]
        {
            0.0f, 0.0f, 0.0f, 
            1.0f, 0.0f, 0.0f, 
            1.0f, 1.0f, 0.0f, 
            0.0f, 1.0f, 0.0f, 
            2.0f, 0.0f, 0.0f, 
            2.0f, 1.0f, 0.0f
        };
        
        // UV coordinates for all 12 corners. 
        float[] textureCoordinates = new float[]
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
        Simplygon.spGeometryData sgGeometryData = sg.CreateGeometryData();
        
        // Set vertex- and triangle-counts for the Geometry. 
        // NOTE: The number of vertices and triangles has to be set before vertex- and triangle-data is 
        // loaded into the GeometryData. 
        sgGeometryData.SetVertexCount(vertexCount);
        sgGeometryData.SetTriangleCount(triangleCount);
        
        // Array with vertex-coordinates. Will contain 3 real-values for each vertex in the geometry. 
        Simplygon.spRealArray sgCoords = sgGeometryData.GetCoords();
        
        // Array with triangle-data. Will contain 3 ids for each corner of each triangle, so the triangles 
        // know what vertices to use. 
        Simplygon.spRidArray sgVertexIds = sgGeometryData.GetVertexIds();
        
        // Must add texture channel before adding data to it. 
        sgGeometryData.AddTexCoords(0);
        Simplygon.spRealArray sgTexcoords = sgGeometryData.GetTexCoords(0);
        
        // Add vertex-coordinates array to the Geometry. 
        sgCoords.SetData(vertexCoordinates, vertexCount * 3);
        
        // Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
        // uses. 
        sgVertexIds.SetData(cornerIds, cornerCount);
        
        // Add texture-coordinates array to the Geometry. 
        sgTexcoords.SetData(textureCoordinates, cornerCount * 2);
        
        // Create a scene and a SceneMesh node with the geometry. 
        Simplygon.spScene sgScene = sg.CreateScene();
        Simplygon.spSceneMesh sgSceneMesh = sg.CreateSceneMesh();
        sgSceneMesh.SetName("Mesh2");
        sgSceneMesh.SetGeometry(sgGeometryData);
        sgScene.GetRootNode().AddChild(sgSceneMesh);

        
        // Save example2 scene to Example2.obj.         
        Console.WriteLine("Save example2 scene to Example2.obj.");
        SaveScene(sg, sgScene, "Example2.obj");
        
        // Check log for any warnings or errors.         
        Console.WriteLine("Check log for any warnings or errors.");
        CheckLog(sg);
    }

    static void RunExample3(Simplygon.ISimplygon sg)
    {
        // Same as RunExample1, but now all corner-data is stored as vertex-data, in a packet format. 
        // Since the 2 vertices where the quads meet don't share same UV, they will be 2 separate vertices, 
        // so 4 vertices / quad as opposed to 6 / quad in RunExample1, and only 6 for whole mesh in 
        // RunExample2. 
        const int vertexCount = 8;
        const int triangleCount = 4;
        const int cornerCount = triangleCount * 3;
        
        // 4 triangles x 3 indices ( or 3 corners ). 
        int[] cornerIds = new int[]
        {
            0, 1, 2, 
            0, 2, 3, 
            4, 5, 6, 
            4, 6, 7
        };
        
        // 8 vertices with values for the x, y and z coordinates. 
        float[] vertexCoordinates = new float[]
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
        float[] textureCoordinates = new float[]
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
        Simplygon.spPackedGeometryData sgPackedGeometryData = sg.CreatePackedGeometryData();
        
        // Set vertex- and triangle-counts for the Geometry. 
        // NOTE: The number of vertices and triangles has to be set before vertex- and triangle-data is 
        // loaded into the GeometryData. 
        sgPackedGeometryData.SetVertexCount(vertexCount);
        sgPackedGeometryData.SetTriangleCount(triangleCount);
        
        // Array with vertex-coordinates. Will contain 3 real-values for each vertex in the geometry. 
        Simplygon.spRealArray sgCoords = sgPackedGeometryData.GetCoords();
        
        // Array with triangle-data. Will contain 3 ids for each corner of each triangle, so the triangles 
        // know what vertices to use. 
        Simplygon.spRidArray sgVertexIds = sgPackedGeometryData.GetVertexIds();
        
        // Must add texture channel before adding data to it. 
        sgPackedGeometryData.AddTexCoords(0);
        Simplygon.spRealArray sgTexcoords = sgPackedGeometryData.GetTexCoords(0);
        
        // Add vertex-coordinates array to the Geometry. 
        sgCoords.SetData(vertexCoordinates, vertexCount * 3);
        
        // Add triangles to the Geometry. Each triangle-corner contains the id for the vertex that corner 
        // uses. 
        sgVertexIds.SetData(cornerIds, cornerCount);
        
        // Add texture-coordinates array to the Geometry. 
        sgTexcoords.SetData(textureCoordinates, cornerCount * 2);
        
        // Create a scene and a SceneMesh node with the geometry. 
        Simplygon.spScene sgScene = sg.CreateScene();
        Simplygon.spSceneMesh sgSceneMesh = sg.CreateSceneMesh();
        sgSceneMesh.SetName("Mesh3");
        Simplygon.spGeometryData sgGeometryData = sgPackedGeometryData.NewUnpackedCopy();
        sgSceneMesh.SetGeometry(sgGeometryData);
        sgScene.GetRootNode().AddChild(sgSceneMesh);

        
        // Save example3 scene to Example3.obj.         
        Console.WriteLine("Save example3 scene to Example3.obj.");
        SaveScene(sg, sgScene, "Example3.obj");
        
        // Check log for any warnings or errors.         
        Console.WriteLine("Check log for any warnings or errors.");
        CheckLog(sg);
    }

    static int Main(string[] args)
    {
        using var sg = Simplygon.Loader.InitSimplygon(out var errorCode, out var errorMessage);
        if (errorCode != Simplygon.EErrorCodes.NoError)
        {
            Console.WriteLine( $"Failed to initialize Simplygon: ErrorCode({(int)errorCode}) {errorMessage}" );
            return (int)errorCode;
        }
        RunExample1(sg);
        RunExample2(sg);
        RunExample3(sg);

        return 0;
    }

}
