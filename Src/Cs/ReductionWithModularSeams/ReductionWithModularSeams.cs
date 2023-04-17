// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

using System;
using System.IO;
using System.Threading.Tasks;

public class Program
{
    static Simplygon.spScene LoadScene(Simplygon.ISimplygon sg, string path)
    {
        // Create scene importer 
        using Simplygon.spSceneImporter sgSceneImporter = sg.CreateSceneImporter();
        sgSceneImporter.SetImportFilePath(path);
        
        // Run scene importer. 
        var importResult = sgSceneImporter.Run();
        if (Simplygon.Simplygon.Failed(importResult))
        {
            throw new System.Exception("Failed to load scene.");
        }
        Simplygon.spScene sgScene = sgSceneImporter.GetScene();
        return sgScene;
    }

    static void SaveScene(Simplygon.ISimplygon sg, Simplygon.spScene sgScene, string path)
    {
        // Create scene exporter. 
        using Simplygon.spSceneExporter sgSceneExporter = sg.CreateSceneExporter();
        string outputScenePath = string.Join("", new string[] { "output\\", "ReductionWithModularSeams", "_", path });
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
                Console.WriteLine("Errors:");
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
            Console.WriteLine("No errors.");
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
                Console.WriteLine("Warnings:");
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
            Console.WriteLine("No warnings.");
        }
        
        // Error out if Simplygon has errors. 
        if (hasErrors)
        {
            throw new System.Exception("Processing failed with an error");
        }
    }

    static Simplygon.spGeometryDataCollection ExtractGeometriesInScene(Simplygon.ISimplygon sg, Simplygon.spScene sgModularAssetsScene)
    {
        // Extract all geometries in the scene into individual geometries 
        Simplygon.spGeometryDataCollection sgGeometryDataCollection = sg.CreateGeometryDataCollection();
        int id = sgModularAssetsScene.SelectNodes("ISceneMesh");
        var set = sgModularAssetsScene.GetSelectionSetTable().GetSelectionSet(id);

        var geometryCount = set.GetItemCount();
        for (uint geomIndex = 0; geomIndex < geometryCount; ++geomIndex)
        {
            var guid = set.GetItem(geomIndex);
            Simplygon.spSceneNode sgSceneNode = sgModularAssetsScene.GetNodeByGUID(guid);
            Simplygon.spSceneMesh sgSceneMesh = Simplygon.spSceneMesh.SafeCast(sgSceneNode);
            Simplygon.spGeometryData geom = sgSceneMesh.GetGeometry();
            sgGeometryDataCollection.AddGeometryData(geom);
        }
        return sgGeometryDataCollection;
    }

    static void DebugModularSeams(Simplygon.ISimplygon sg, bool outputDebugInfo, Simplygon.spModularSeams sgModularSeams)
    {
        if (outputDebugInfo)
        {
            // Optional but helpful to be able to see what the analyzer found. 
            // Each unique modular seam can be extracted as a geometry. If the analyzer ran with 
            // IsTranslationIndependent=false then the seam geometry should be exactly located at the same 
            // place as the modular seams in the original scene. 
            // Each modular seam also has a string array with all the names of the geometries that have that 
            // specific modular seam. 
            var seamCount = sgModularSeams.GetModularSeamCount();
            for (uint seamIndex = 0; seamIndex < seamCount; ++seamIndex)
            {
                Simplygon.spGeometryData debugGeom = sgModularSeams.NewDebugModularSeamGeometry((int)seamIndex);
                Simplygon.spStringArray geometryNames = sgModularSeams.NewModularSeamGeometryStringArray((int)seamIndex);
                

                Simplygon.spScene debugScene = sg.CreateScene();
                debugScene.GetRootNode().CreateChildMesh(debugGeom);

                string fileName = string.Join("", new string[] { "output\\", "ReductionWithModularSeams_seam_", seamIndex.ToString(), ".obj" });
                

                using Simplygon.spSceneExporter sgSceneExporter = sg.CreateSceneExporter();
                sgSceneExporter.SetExportFilePath( fileName );
                sgSceneExporter.SetScene( debugScene );
                sgSceneExporter.Run();
                

                var vertexCount = debugGeom.GetVertexCount();
                var geometryNamesCount = geometryNames.GetItemCount();
                string outputText = string.Join("", new string[] { "Seam ", seamIndex.ToString(), " consists of ", vertexCount.ToString(), " vertices and is shared among ", geometryNamesCount.ToString(), " geometries:" });
                Console.WriteLine(outputText);
                for (uint geomIndex = 0; geomIndex < geometryNamesCount; ++geomIndex)
                {
                    string geometryName = geometryNames.GetItem((int)geomIndex);
                    string geometryNameOutput = string.Join("", new string[] { " geom ", geomIndex.ToString(), ": ", geometryName });
                    Console.WriteLine(geometryNameOutput);
                }
            }
        }
    }

    static void ModifyReductionSettings(Simplygon.spReductionSettings sgReductionSettings, float triangleRatio, float maxDeviation)
    {
        sgReductionSettings.SetKeepSymmetry( true );
        sgReductionSettings.SetUseAutomaticSymmetryDetection( true );
        sgReductionSettings.SetUseHighQualityNormalCalculation( true );
        sgReductionSettings.SetReductionHeuristics( Simplygon.EReductionHeuristics.Consistent );
        
        // The importances can be changed here to allow the features to be weighed differently both during 
        // regular reduction and during the analyzing of modular seam 
        sgReductionSettings.SetEdgeSetImportance( 1.0f );
        sgReductionSettings.SetGeometryImportance( 1.0f );
        sgReductionSettings.SetGroupImportance( 1.0f );
        sgReductionSettings.SetMaterialImportance( 1.0f );
        sgReductionSettings.SetShadingImportance( 1.0f );
        sgReductionSettings.SetSkinningImportance( 1.0f );
        sgReductionSettings.SetTextureImportance( 1.0f );
        sgReductionSettings.SetVertexColorImportance( 1.0f );
        
        // The reduction targets below are only used for the regular reduction, not the modular seam 
        // analyzer 
        sgReductionSettings.SetReductionTargetTriangleRatio( triangleRatio );
        sgReductionSettings.SetReductionTargetMaxDeviation( maxDeviation );
        sgReductionSettings.SetReductionTargets(Simplygon.EStopCondition.All, true, false, true, false);
    }

    static void GenerateModularSeams(Simplygon.ISimplygon sg, Simplygon.spScene sgModularAssetsScene)
    {
        Simplygon.spGeometryDataCollection sgGeometryDataCollection = ExtractGeometriesInScene(sg, sgModularAssetsScene);
        
        // Figure out a small value in relation to the scene that will be the tolerance for the modular seams 
        // if a coordinate is moved a distance smaller than the tolerance, then it is regarded as the same 
        // coordinate so two vertices are the at the same place if the distance between them is smaller than 
        // radius * smallValue 
        sgModularAssetsScene.CalculateExtents();
        float smallValue = 0.0001f;
        float sceneRadius = sgModularAssetsScene.GetRadius();
        float tolerance = sceneRadius * smallValue;
        using Simplygon.spReductionSettings sgReductionSettings = sg.CreateReductionSettings();
        
        // The triangleRatio and maxDeviation are not important here and will not be used, only the 
        // relative importances and settings 
        ModifyReductionSettings(sgReductionSettings, 0.0f, 0.0f);
        
        // Create the modular seam analyzer. 
        using Simplygon.spModularSeamAnalyzer sgModularSeamAnalyzer = sg.CreateModularSeamAnalyzer();
        sgModularSeamAnalyzer.SetTolerance( tolerance );
        sgModularSeamAnalyzer.SetIsTranslationIndependent( false );
        var modularGeometryCount = sgGeometryDataCollection.GetItemCount();
        
        // Add the geometries to the analyzer 
        for (uint modularGeometryId = 0; modularGeometryId < modularGeometryCount; ++modularGeometryId)
        {
            var modularGeometryObject = sgGeometryDataCollection.GetItemAsObject(modularGeometryId);
            Simplygon.spGeometryData modularGeometry = Simplygon.spGeometryData.SafeCast(modularGeometryObject);
            sgModularSeamAnalyzer.AddGeometry(modularGeometry);
        }
        
        // The analyzer needs to know the different reduction settings importances and such because it 
        // runs the reduction as far as possible for all the seams and stores the order and max deviations 
        // for future reductions of assets with the same seams 
        sgModularSeamAnalyzer.Analyze(sgReductionSettings);
        
        // Fetch the modular seams. These can be stored to file and used later 
        using Simplygon.spModularSeams sgModularSeams = sgModularSeamAnalyzer.GetModularSeams();
        string modularSeamsPath = string.Join("", new string[] { "output\\", "ModularAssets.modseam" });
        sgModularSeams.SaveToFile(modularSeamsPath);
    }

    static Simplygon.spModularSeams LoadModularSeams(Simplygon.ISimplygon sg)
    {
        // Load pre-generated modular seams 
        Simplygon.spModularSeams sgModularSeams = sg.CreateModularSeams();
        string modularSeamsPath = string.Join("", new string[] { "output\\", "ModularAssets.modseam" });
        sgModularSeams.LoadFromFile(modularSeamsPath);
        return sgModularSeams;
    }

    static void RunReduction(Simplygon.ISimplygon sg, Simplygon.spScene sgModularAssetsScene, Simplygon.spModularSeams sgModularSeams, float triangleRatio, float maxDeviation, float modularSeamReductionRatio, float modularSeamMaxDeviation)
    {
        Simplygon.spGeometryDataCollection sgGeometryDataCollection = ExtractGeometriesInScene(sg, sgModularAssetsScene);
        var modularGeometryCount = sgGeometryDataCollection.GetItemCount();
        
        // Add the geometries to the analyzer 
        for (uint modularGeometryId = 0; modularGeometryId < modularGeometryCount; ++modularGeometryId)
        {
            var modularGeometryObject = sgGeometryDataCollection.GetItemAsObject(modularGeometryId);
            Simplygon.spGeometryData modularGeometry = Simplygon.spGeometryData.SafeCast(modularGeometryObject);
            
            // Run reduction on each geometry individually, 
            // feed the modular seams into the reducer with the ModularSeamSettings 
            // so the modular seams are reduced identically and are untouched by the rest of the 
            // geometry reduction 
            Simplygon.spScene sgSingleAssetScene = sgModularAssetsScene.NewCopy();
            
            // Remove all the geometries but keep any textures, materials etc. 
            sgSingleAssetScene.RemoveSceneNodes();
            
            // Add just a copy of the current geometry to the scene 
            Simplygon.spGeometryData modularGeometryCopy = modularGeometry.NewCopy(true);
            Simplygon.spSceneNode sgRootNode = sgSingleAssetScene.GetRootNode();
            Simplygon.spSceneMesh sgSceneMesh = sgRootNode.CreateChildMesh(modularGeometryCopy);
            

            using Simplygon.spReductionProcessor sgReductionProcessor = sg.CreateReductionProcessor();
            sgReductionProcessor.SetScene( sgSingleAssetScene );
            using Simplygon.spReductionSettings sgReductionSettings = sgReductionProcessor.GetReductionSettings();
            using Simplygon.spModularSeamSettings sgModularSeamSettings = sgReductionProcessor.GetModularSeamSettings();
            
            // Set the same reduction (importance) settings as the modular seam analyzer for consistent 
            // quality 
            ModifyReductionSettings(sgReductionSettings, triangleRatio, maxDeviation);
            sgModularSeamSettings.SetReductionRatio( modularSeamReductionRatio );
            sgModularSeamSettings.SetMaxDeviation( modularSeamMaxDeviation );
            sgModularSeamSettings.SetStopCondition( Simplygon.EStopCondition.All );
            sgModularSeamSettings.SetModularSeams( sgModularSeams );
            

            sgReductionProcessor.RunProcessing();
            string geomName = modularGeometry.GetName();
            string outputName = string.Join("", new string[] { geomName, ".obj" });
            SaveScene(sg, sgSingleAssetScene, outputName);
        }
    }

    static void RunReductionWithModularSeams(Simplygon.ISimplygon sg)
    {
        // Set reduction targets. Stop condition is set to 'All' 
        float triangleRatio = 0.5f;
        float maxDeviation = 0.0f;
        float modularSeamReductionRatio = 0.75f;
        float modularSeamMaxDeviation = 0.0f;
        
        // Load a scene that has a few modular assets in it as different scene meshes. 
        Simplygon.spScene sgModularAssetsScene = LoadScene(sg, "../../../Assets/ModularAssets/ModularAssets.obj");
        bool generateNewSeams = true;
        if (generateNewSeams)
        {
            GenerateModularSeams(sg, sgModularAssetsScene);
        }
        Simplygon.spModularSeams sgModularSeams = LoadModularSeams(sg);
        DebugModularSeams(sg, true, sgModularSeams);
        
        // Run the reduction. The seams are reduced identically and the rest of the geometries are reduced 
        // like normal 
        RunReduction(sg, sgModularAssetsScene, sgModularSeams, triangleRatio, maxDeviation, modularSeamReductionRatio, modularSeamMaxDeviation);
        
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
        RunReductionWithModularSeams(sg);

        return 0;
    }

}
