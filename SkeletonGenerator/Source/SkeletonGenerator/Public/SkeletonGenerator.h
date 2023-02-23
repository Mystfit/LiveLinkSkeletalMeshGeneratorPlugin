// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FSkeletonGeneratorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void PluginButtonClicked();
	void CreatePanel(const FString& BlueprintAssetPath, const FString& PanelLabel);
	void AddMenuEntry(FMenuBuilder& MenuBuilder);
	TSharedPtr<class FUICommandList> PluginCommands;
};
