using UnrealBuildTool;

public class BMIGraphicsRuntime : ModuleRules
{
    public BMIGraphicsRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects"
        });
    }
}
