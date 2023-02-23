// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkeletonGeneratorCommands.h"

#define LOCTEXT_NAMESPACE "FSkeletonGeneratorModule"

void FSkeletonGeneratorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "Livelink Skeleton Generator", "Bring up the Live link skeleton generator panel", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
