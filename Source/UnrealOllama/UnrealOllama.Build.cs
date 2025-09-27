// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class UnrealOllama : ModuleRules
{
	public UnrealOllama(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"HTTP",
				"Json",
				"JsonUtilities",
				"ImageWrapper",
				"RHI",
				"RenderCore",
			}
		);
	}
}
