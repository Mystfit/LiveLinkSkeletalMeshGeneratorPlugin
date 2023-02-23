// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SkeletonGeneratorStyle.h"

class FSkeletonGeneratorCommands : public TCommands<FSkeletonGeneratorCommands>
{
public:

	FSkeletonGeneratorCommands()
		: TCommands<FSkeletonGeneratorCommands>(TEXT("SkeletonGenerator"), NSLOCTEXT("Contexts", "SkeletonGenerator", "Livelink Skeleton Generator Plugin"), NAME_None, FSkeletonGeneratorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};