// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RuntimeInspector : ModuleRules
{

    public RuntimeInspector(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"UMG",
				"Slate",
				"SlateCore",
				"Json",
				"JsonUtilities",
                "InputCore",
				"DeveloperSettings"
            }
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "ApplicationCore",
                "ImageCore",
                "Networking",
                "Projects",
                "RenderCore",
                "Sockets"
            }
		);

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "UMGEditor"
                }
            );
        }


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            PublicDefinitions.Add("RUNTIME_INSPECTOR_ENABLED=0");
        }
        else
        {
            PublicDefinitions.Add("RUNTIME_INSPECTOR_ENABLED=1");
        }
    }
}
