// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT License. 

using System;
using System.IO;
using System.Threading.Tasks;

public class CustomErrorHandler : Simplygon.ErrorHandler
{
    public override void HandleError(Simplygon.spObject obj, string interfaceName, string methodName, int errorType , string errorText)
    {
        Console.WriteLine($"Error ({interfaceName}:{methodName}): {errorText}");
    }
}

public class Program
{
    static private CustomErrorHandler customErrorHandler;

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
        // Set the custom error handler to the Simplygon interface. 
        sg.SetErrorHandler(customErrorHandler);
        
        // Create the reduction pipeline. 
        using Simplygon.spReductionPipeline sgReductionPipeline = sg.CreateReductionPipeline();
        
        // To be able to use the T-Junction remover, the welder has to be enabled so this will trigger an 
        // error. 
        using Simplygon.spRepairSettings sgRepairSettings = sgReductionPipeline.GetRepairSettings();
        sgRepairSettings.SetUseWelding( false );
        sgRepairSettings.SetUseTJunctionRemover( true );
        
        // Start the reduction pipeline and the faulty settings will cause an error.         
        Console.WriteLine("Start the reduction pipeline and the faulty settings will cause an error.");
        sgReductionPipeline.RunSceneFromFile("../../../Assets/SimplygonMan/SimplygonMan.obj", "output.fbx", Simplygon.EPipelineRunMode.RunInNewProcess);
        
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
        customErrorHandler = new CustomErrorHandler();
        RunReduction(sg);

        return 0;
    }
}
