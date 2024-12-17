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
        string outputScenePath = string.Join("", new string[] { "output\\", "GeometryDataCasting", "_", path });
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

    static void RunGeometryDataDasting(Simplygon.ISimplygon sg)
    {
        // Load scene to process. 
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
        
        // Create the remeshing processor. 
        using Simplygon.spRemeshingProcessor sgRemeshingProcessor = sg.CreateRemeshingProcessor();
        sgRemeshingProcessor.SetScene( sgScene );
        using Simplygon.spRemeshingSettings sgRemeshingSettings = sgRemeshingProcessor.GetRemeshingSettings();
        using Simplygon.spMappingImageSettings sgMappingImageSettings = sgRemeshingProcessor.GetMappingImageSettings();
        
        // Set on-screen size target for remeshing. 
        sgRemeshingSettings.SetOnScreenSize( 300 );
        
        // Generates a mapping image which is used after the remeshing to cast new materials to the new 
        // remeshed object. 
        sgMappingImageSettings.SetGenerateMappingImage( true );
        sgMappingImageSettings.SetGenerateTexCoords( true );
        sgMappingImageSettings.SetApplyNewMaterialIds( true );
        sgMappingImageSettings.SetGenerateTangents( true );
        sgMappingImageSettings.SetUseFullRetexturing( true );
        using Simplygon.spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0);
        
        // Setting the size of the output material for the mapping image. This will be the output size of the 
        // textures when we do material casting in a later stage. 
        sgOutputMaterialSettings.SetTextureWidth( 2048 );
        sgOutputMaterialSettings.SetTextureHeight( 2048 );
        
        // Start the remeshing process.         
        Console.WriteLine("Start the remeshing process.");
        sgRemeshingProcessor.RunProcessing();
        
        // Setup and run the geometry data caster casting Coords to a texture.         
        Console.WriteLine("Setup and run the geometry data caster casting Coords to a texture.");
        string geometrydata_coordsTextureFilePath;
        using Simplygon.spGeometryDataCaster sgGeometryData_CoordsCaster = sg.CreateGeometryDataCaster();
        sgGeometryData_CoordsCaster.SetMappingImage( sgRemeshingProcessor.GetMappingImage() );
        sgGeometryData_CoordsCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgGeometryData_CoordsCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgGeometryData_CoordsCaster.SetOutputFilePath( "GeometryData_CoordsTexture" );

        using Simplygon.spGeometryDataCasterSettings sgGeometryData_CoordsCasterSettings = sgGeometryData_CoordsCaster.GetGeometryDataCasterSettings();
        sgGeometryData_CoordsCasterSettings.SetMaterialChannel( "GeometryData_Coords" );
        sgGeometryData_CoordsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgGeometryData_CoordsCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat.R16G16B16 );
        sgGeometryData_CoordsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.NoFill );
        sgGeometryData_CoordsCasterSettings.SetGeometryDataFieldType( Simplygon.EGeometryDataFieldType.Coords );
        sgGeometryData_CoordsCasterSettings.SetGeometryDataFieldIndex( 0 );

        sgGeometryData_CoordsCaster.RunProcessing();
        geometrydata_coordsTextureFilePath = sgGeometryData_CoordsCaster.GetOutputFilePath();
        
        // Setup and run the geometry data caster casting Normals to a texture.         
        Console.WriteLine("Setup and run the geometry data caster casting Normals to a texture.");
        string geometrydata_normalsTextureFilePath;
        using Simplygon.spGeometryDataCaster sgGeometryData_NormalsCaster = sg.CreateGeometryDataCaster();
        sgGeometryData_NormalsCaster.SetMappingImage( sgRemeshingProcessor.GetMappingImage() );
        sgGeometryData_NormalsCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgGeometryData_NormalsCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgGeometryData_NormalsCaster.SetOutputFilePath( "GeometryData_NormalsTexture" );

        using Simplygon.spGeometryDataCasterSettings sgGeometryData_NormalsCasterSettings = sgGeometryData_NormalsCaster.GetGeometryDataCasterSettings();
        sgGeometryData_NormalsCasterSettings.SetMaterialChannel( "GeometryData_Normals" );
        sgGeometryData_NormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgGeometryData_NormalsCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat.R16G16B16 );
        sgGeometryData_NormalsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.NoFill );
        sgGeometryData_NormalsCasterSettings.SetGeometryDataFieldType( Simplygon.EGeometryDataFieldType.Normals );
        sgGeometryData_NormalsCasterSettings.SetGeometryDataFieldIndex( 0 );

        sgGeometryData_NormalsCaster.RunProcessing();
        geometrydata_normalsTextureFilePath = sgGeometryData_NormalsCaster.GetOutputFilePath();
        
        // Setup and run the geometry data caster casting MaterialIds to a texture.         
        Console.WriteLine("Setup and run the geometry data caster casting MaterialIds to a texture.");
        string geometrydata_materialidsTextureFilePath;
        using Simplygon.spGeometryDataCaster sgGeometryData_MaterialIdsCaster = sg.CreateGeometryDataCaster();
        sgGeometryData_MaterialIdsCaster.SetMappingImage( sgRemeshingProcessor.GetMappingImage() );
        sgGeometryData_MaterialIdsCaster.SetSourceMaterials( sgScene.GetMaterialTable() );
        sgGeometryData_MaterialIdsCaster.SetSourceTextures( sgScene.GetTextureTable() );
        sgGeometryData_MaterialIdsCaster.SetOutputFilePath( "GeometryData_MaterialIdsTexture" );

        using Simplygon.spGeometryDataCasterSettings sgGeometryData_MaterialIdsCasterSettings = sgGeometryData_MaterialIdsCaster.GetGeometryDataCasterSettings();
        sgGeometryData_MaterialIdsCasterSettings.SetMaterialChannel( "GeometryData_MaterialIds" );
        sgGeometryData_MaterialIdsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );
        sgGeometryData_MaterialIdsCasterSettings.SetOutputPixelFormat( Simplygon.EPixelFormat.R8 );
        sgGeometryData_MaterialIdsCasterSettings.SetFillMode( Simplygon.EAtlasFillMode.NoFill );
        sgGeometryData_MaterialIdsCasterSettings.SetGeometryDataFieldType( Simplygon.EGeometryDataFieldType.MaterialIds );
        sgGeometryData_MaterialIdsCasterSettings.SetGeometryDataFieldIndex( 0 );

        sgGeometryData_MaterialIdsCaster.RunProcessing();
        geometrydata_materialidsTextureFilePath = sgGeometryData_MaterialIdsCaster.GetOutputFilePath();
        
        // Update scene with new casted textures. 
        using Simplygon.spMaterialTable sgMaterialTable = sg.CreateMaterialTable();
        using Simplygon.spTextureTable sgTextureTable = sg.CreateTextureTable();
        using Simplygon.spMaterial sgMaterial = sg.CreateMaterial();
        sgMaterial.SetName("OutputMaterial");
        using Simplygon.spTexture sgGeometryData_CoordsTexture = sg.CreateTexture();
        sgGeometryData_CoordsTexture.SetName( "GeometryData_Coords" );
        sgGeometryData_CoordsTexture.SetFilePath( geometrydata_coordsTextureFilePath );
        sgTextureTable.AddTexture( sgGeometryData_CoordsTexture );

        using Simplygon.spShadingTextureNode sgGeometryData_CoordsTextureShadingNode = sg.CreateShadingTextureNode();
        sgGeometryData_CoordsTextureShadingNode.SetTexCoordLevel( 0 );
        sgGeometryData_CoordsTextureShadingNode.SetTextureName( "GeometryData_Coords" );

        sgMaterial.AddMaterialChannel( "GeometryData_Coords" );
        sgMaterial.SetShadingNetwork( "GeometryData_Coords", sgGeometryData_CoordsTextureShadingNode );
        using Simplygon.spTexture sgGeometryData_NormalsTexture = sg.CreateTexture();
        sgGeometryData_NormalsTexture.SetName( "GeometryData_Normals" );
        sgGeometryData_NormalsTexture.SetFilePath( geometrydata_normalsTextureFilePath );
        sgTextureTable.AddTexture( sgGeometryData_NormalsTexture );

        using Simplygon.spShadingTextureNode sgGeometryData_NormalsTextureShadingNode = sg.CreateShadingTextureNode();
        sgGeometryData_NormalsTextureShadingNode.SetTexCoordLevel( 0 );
        sgGeometryData_NormalsTextureShadingNode.SetTextureName( "GeometryData_Normals" );

        sgMaterial.AddMaterialChannel( "GeometryData_Normals" );
        sgMaterial.SetShadingNetwork( "GeometryData_Normals", sgGeometryData_NormalsTextureShadingNode );
        using Simplygon.spTexture sgGeometryData_MaterialIdsTexture = sg.CreateTexture();
        sgGeometryData_MaterialIdsTexture.SetName( "GeometryData_MaterialIds" );
        sgGeometryData_MaterialIdsTexture.SetFilePath( geometrydata_materialidsTextureFilePath );
        sgTextureTable.AddTexture( sgGeometryData_MaterialIdsTexture );

        using Simplygon.spShadingTextureNode sgGeometryData_MaterialIdsTextureShadingNode = sg.CreateShadingTextureNode();
        sgGeometryData_MaterialIdsTextureShadingNode.SetTexCoordLevel( 0 );
        sgGeometryData_MaterialIdsTextureShadingNode.SetTextureName( "GeometryData_MaterialIds" );

        sgMaterial.AddMaterialChannel( "GeometryData_MaterialIds" );
        sgMaterial.SetShadingNetwork( "GeometryData_MaterialIds", sgGeometryData_MaterialIdsTextureShadingNode );

        sgMaterialTable.AddMaterial( sgMaterial );

        sgScene.GetTextureTable().Clear();
        sgScene.GetMaterialTable().Clear();
        sgScene.GetTextureTable().Copy(sgTextureTable);
        sgScene.GetMaterialTable().Copy(sgMaterialTable);
        
        // Save processed scene.         
        Console.WriteLine("Save processed scene.");
        SaveScene(sg, sgScene, "Output.fbx");
        
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
        RunGeometryDataDasting(sg);

        return 0;
    }

}
