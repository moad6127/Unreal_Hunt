using UnrealBuildTool;

public class HuntAnimTools : ModuleRules
{
	public HuntAnimTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "Json", "UnrealEd", "BlueprintGraph", "Kismet", "AnimGraph", "ToolsetRegistry"
		});
	}
}
