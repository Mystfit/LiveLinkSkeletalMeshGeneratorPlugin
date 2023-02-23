// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkeletonGenerator.h"
#include "SkeletonGeneratorCommands.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "FSkeletonGeneratorModule"

void FSkeletonGeneratorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	// Register commands
	FSkeletonGeneratorStyle::Initialize();
	FSkeletonGeneratorStyle::ReloadTextures();
	FSkeletonGeneratorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FSkeletonGeneratorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FSkeletonGeneratorModule::PluginButtonClicked),
		FCanExecuteAction());
	

	TSharedPtr<FExtender> NewMenuExtender = MakeShareable(new FExtender);
	NewMenuExtender->AddMenuExtension("LevelEditor",
		EExtensionHook::After,
		PluginCommands,
		FMenuExtensionDelegate::CreateRaw(this, &FSkeletonGeneratorModule::AddMenuEntry));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(NewMenuExtender);
}

void FSkeletonGeneratorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

void FSkeletonGeneratorModule::PluginButtonClicked()
{
	FString DependencyBPPath = "/SkeletonGenerator/Widgets/BLU_SkelmeshFromLivelink.BLU_SkelmeshFromLivelink";
	CreatePanel(DependencyBPPath, "Live link Skeleton Generator");
}

void FSkeletonGeneratorModule::CreatePanel(const FString& BlueprintAssetPath, const FString& PanelLabel)
{
	UEditorUtilitySubsystem* EUSubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	UEditorUtilityWidgetBlueprint* EditorWidget = LoadObject<UEditorUtilityWidgetBlueprint>(NULL, *BlueprintAssetPath, NULL, LOAD_Verify, NULL);
	EUSubsystem->SpawnAndRegisterTab(EditorWidget);

	// Find created tab so we can rename it
	FName TabID;
	EUSubsystem->SpawnAndRegisterTabAndGetID(EditorWidget, TabID);
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
	if (auto Tab = LevelEditorTabManager->FindExistingLiveTab(TabID)) {
		Tab->SetLabel(FText::FromString(PanelLabel));
	}
}

void FSkeletonGeneratorModule::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("SkeletonGeneratorMenu", TAttribute<FText>(FText::FromString("Live Link Skeleton Generator")));
	MenuBuilder.AddMenuEntry(FSkeletonGeneratorCommands::Get().OpenPluginWindow);
	MenuBuilder.EndSection();
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSkeletonGeneratorModule, SkeletonGenerator)