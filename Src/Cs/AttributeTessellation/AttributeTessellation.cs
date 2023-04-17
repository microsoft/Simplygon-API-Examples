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
        string outputScenePath = string.Join("", new string[] { "output\\", "AttributeTessellation", "_", path });
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

    static void RunRemeshingWithTessellatedAttributes(Simplygon.ISimplygon sg)
    {
        // Load scene to process.         
        Console.WriteLine("Load scene to process.");
        Simplygon.spScene sgScene = LoadScene(sg, "../../../Assets/SimplygonMan/SimplygonMan.obj");
        
        // Create the remeshing pipeline. 
        using Simplygon.spRemeshingPipeline sgRemeshingPipeline = sg.CreateRemeshingPipeline();
        
        // Fetch all the needed settings objects for the processing, including the attribute 
        // tessellation settings, which we will use to set up the attribute tessellation on the processed 
        // mesh. 
        using Simplygon.spRemeshingSettings sgRemeshingSettings = sgRemeshingPipeline.GetRemeshingSettings();
        using Simplygon.spAttributeTessellationSettings sgAttributeTessellationSettings = sgRemeshingPipeline.GetAttributeTessellationSettings();
        using Simplygon.spMappingImageSettings sgMappingImageSettings = sgRemeshingPipeline.GetMappingImageSettings();
        
        // Set on-screen size target for remeshing. 
        sgRemeshingSettings.SetOnScreenSize( 500 );
        sgRemeshingSettings.SetGeometricalAccuracy( 2.0f );
        
        // Get the attribute tessellation settings. The displacement data will be cast into a 
        // tessellated displacement attribute. In this example we use relative area as the density 
        // setting, which means that triangles are tessellated based on the size of the triangle, so that 
        // the tessellated attributes roughly take up the same area. The value is normalized and scale 
        // independent, so the total area of all the subvalues will add up to the normalized value 1. We set 
        // the maximum area per value to 1/1000000, which means that there will be at least 1000000 values 
        // total in the scene, unless we cap the total number of values with MaxTotalValuesCount or 
        // MaxTessellationLevel. 
        sgAttributeTessellationSettings.SetEnableAttributeTessellation( true );
        sgAttributeTessellationSettings.SetAttributeTessellationDensityMode( Simplygon.EAttributeTessellationDensityMode.RelativeArea );
        sgAttributeTessellationSettings.SetMaxAreaOfTessellatedValue( 0.000001f );
        sgAttributeTessellationSettings.SetOnlyAllowOneLevelOfDifference( true );
        sgAttributeTessellationSettings.SetMinTessellationLevel( 0 );
        sgAttributeTessellationSettings.SetMaxTessellationLevel( 5 );
        sgAttributeTessellationSettings.SetMaxTotalValuesCount( 1000000 );
        
        // Set up the process to generate a mapping image which will be used after the reduction to cast new 
        // materials to the new reduced object, and also to cast the displacement data from the original 
        // object into the tessellated attributes of the processed mesh. 
        sgMappingImageSettings.SetGenerateMappingImage( true );
        sgMappingImageSettings.SetGenerateTexCoords( true );
        sgMappingImageSettings.SetApplyNewMaterialIds( true );
        sgMappingImageSettings.SetGenerateTangents( true );
        sgMappingImageSettings.SetUseFullRetexturing( true );
        sgMappingImageSettings.SetTexCoordGeneratorType( Simplygon.ETexcoordGeneratorType.ChartAggregator );
        using Simplygon.spMappingImageOutputMaterialSettings sgOutputMaterialSettings = sgMappingImageSettings.GetOutputMaterialSettings(0);
        
        // Set the size of the mapping image in the output material. This will be the output size of the 
        // textures when we do the material casting in the pipeline. 
        sgOutputMaterialSettings.SetTextureWidth( 2048 );
        sgOutputMaterialSettings.SetTextureHeight( 2048 );
        sgOutputMaterialSettings.SetMultisamplingLevel( 2 );
        
        // Add a diffuse texture caster to the pipeline. This will cast the diffuse color (aka base 
        // color/albedo) in the original scene into a texture map in the output scene. 
        using Simplygon.spColorCaster sgDiffuseCaster = sg.CreateColorCaster();

        using Simplygon.spColorCasterSettings sgDiffuseCasterSettings = sgDiffuseCaster.GetColorCasterSettings();
        sgDiffuseCasterSettings.SetMaterialChannel( "Diffuse" );
        sgDiffuseCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );

        sgRemeshingPipeline.AddMaterialCaster( sgDiffuseCaster, 0 );
        
        // Add a normals texture caster to the pipeline. This will cast the normals in the original scene 
        // into a normal map in the output scene. 
        using Simplygon.spNormalCaster sgNormalsCaster = sg.CreateNormalCaster();

        using Simplygon.spNormalCasterSettings sgNormalsCasterSettings = sgNormalsCaster.GetNormalCasterSettings();
        sgNormalsCasterSettings.SetMaterialChannel( "Normals" );
        sgNormalsCasterSettings.SetGenerateTangentSpaceNormals( true );
        sgNormalsCasterSettings.SetOutputImageFileFormat( Simplygon.EImageOutputFormat.PNG );

        sgRemeshingPipeline.AddMaterialCaster( sgNormalsCaster, 0 );
        
        // Add a displacement caster to the pipeline. This will cast the displacement values, but instead 
        // of casting to a texture, it will cast into the tessellated attributes. 
        using Simplygon.spDisplacementCaster sgDisplacementCaster = sg.CreateDisplacementCaster();

        sgDisplacementCaster.SetScene( sgScene );
        using Simplygon.spDisplacementCasterSettings sgDisplacementCasterSettings = sgDisplacementCaster.GetDisplacementCasterSettings();
        sgDisplacementCasterSettings.SetMaterialChannel( "Displacement" );
        sgDisplacementCasterSettings.SetDilation( 10 );
        sgDisplacementCasterSettings.SetOutputToTessellatedAttributes( true );
        sgDisplacementCasterSettings.GetAttributeTessellationSamplingSettings().SetSourceMaterialId( 0 );
        sgDisplacementCasterSettings.GetAttributeTessellationSamplingSettings().SetAttributeFormat( Simplygon.EAttributeFormat.U16 );
        sgDisplacementCasterSettings.GetAttributeTessellationSamplingSettings().SetSupersamplingCount( 16 );
        sgDisplacementCasterSettings.GetAttributeTessellationSamplingSettings().SetBlendOperation( Simplygon.EBlendOperation.Mean );

        sgRemeshingPipeline.AddMaterialCaster( sgDisplacementCaster, 0 );
        
        // Start the remeshing pipeline.         
        Console.WriteLine("Start the remeshing pipeline.");
        sgRemeshingPipeline.RunScene(sgScene, Simplygon.EPipelineRunMode.RunInThisProcess);
        
        // Save processed scene.         
        Console.WriteLine("Save processed scene.");
        SaveScene(sg, sgScene, "RemeshedOutput.gltf");
        
        // Create an attribute tessellation tool object. 
        using Simplygon.spAttributeTessellation sgAttributeTessellation = sg.CreateAttributeTessellation();
        
        // Generate a tessellated copy of the scene.         
        Console.WriteLine("Generate a tessellated copy of the scene.");
        using Simplygon.spScene sgTessellatedScene = sgAttributeTessellation.NewTessellatedScene(sgScene);
        
        // Save the tessellated copy of the scene.         
        Console.WriteLine("Save the tessellated copy of the scene.");
        SaveScene(sg, sgTessellatedScene, "RemeshedTessellatedOutput.obj");
        
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
        RunRemeshingWithTessellatedAttributes(sg);

        return 0;
    }

}
