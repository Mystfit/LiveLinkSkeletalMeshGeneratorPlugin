// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkeletonGeneratorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FSkeletonGeneratorStyle::StyleInstance = nullptr;

void FSkeletonGeneratorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSkeletonGeneratorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSkeletonGeneratorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("StableDiffusionToolsStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FSkeletonGeneratorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SkeletonGeneratorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("SkeletonGenerator")->GetBaseDir() / TEXT("Resources"));
	Style->Set("SkeletonGenerator.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("ai-brain"), Icon20x20));

	return Style;
}

void FSkeletonGeneratorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSkeletonGeneratorStyle::Get()
{
	return *StyleInstance;
}
