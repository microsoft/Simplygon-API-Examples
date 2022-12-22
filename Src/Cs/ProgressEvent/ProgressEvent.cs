// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

using System;
using System.IO;
using System.Threading.Tasks;

public class CustomObserver : Simplygon.Observer
{
    public override bool OnProgress(Simplygon.spObject subject, float progressPercent)
    {
        Console.WriteLine($"Progress: {progressPercent}");
        // return false to abort the processing
        return true;
    }
}

public class Program
{
    static private CustomObserver customObserver;

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
    }

    static void RunReduction(Simplygon.ISimplygon sg)
    {
        // Create the reduction pipeline. 
        using Simplygon.spReductionPipeline sgReductionPipeline = sg.CreateReductionPipeline();
        
        // Add the custom observer to the reduction pipeline. 
        sgReductionPipeline.AddObserver(customObserver);
        string outputScenePath = string.Join("", new string[] { "output\\", "ProgressEvent", "_Output.fbx" });
        
        // Start the reduction pipeline.         
        Console.WriteLine("Start the reduction pipeline.");
        sgReductionPipeline.RunSceneFromFile("../../../Assets/SimplygonMan/SimplygonMan.obj", outputScenePath, Simplygon.EPipelineRunMode.RunInNewProcess);
        
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
        customObserver = new CustomObserver();
        RunReduction(sg);

        return 0;
    }

}
