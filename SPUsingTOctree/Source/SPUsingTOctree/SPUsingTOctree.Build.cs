// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;
public class SPUsingTOctree : ModuleRules
{
	public SPUsingTOctree(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		MinFilesUsingPrecompiledHeaderOverride = 1;
		bUseUnity = false;
		
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
			PublicDefinitions.Add("AS_DEBUG=1");
		else
			PublicDefinitions.Add("AS_DEBUG=0");
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
