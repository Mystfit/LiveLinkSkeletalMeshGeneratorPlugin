// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkeletonGeneratorBPLibrary.h"
#include "UObject/SavePackage.h"
#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "ImportUtils/SkeletalMeshImportUtils.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "Rendering/SkeletalMeshModel.h"
#include "RenderCommandFence.h"
#include "IMeshBuilderModule.h"
#include "Engine/SkeletalMeshSocket.h"
#include "SkinWeightsBindingTool.h"
#include "SkeletonGenerator.h"
#include "LevelEditor.h"
#include "InteractiveToolManager.h"
#include "EditorModeManager.h"
#include "ToolTargetManager.h"
#include "EdModeInteractiveToolsContext.h"

USkeletonGeneratorBPLibrary::USkeletonGeneratorBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

USkeletalMesh* USkeletonGeneratorBPLibrary::CreateSkeletalMeshAssetFromBones(const FString& AssetPath, const FName& AssetName, const TArray<FName>& Bones, const TArray<int>& BoneParents, const TArray<FTransform>& BoneParentSpaceTransforms)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("UInterchangeSkeletalMeshFactory::CreateEmptyAsset")
#if !WITH_EDITOR || !WITH_EDITORONLY_DATA

		UE_LOG(LogInterchangeImport, Error, TEXT("Cannot import skeletalMesh asset in runtime, this is an editor only feature."));
	return nullptr;
#else

	// Set up package to hold our skeletal mesh and skeleton
	FString FullPackagePath = FPaths::Combine(AssetPath, AssetName.ToString());
	UPackage* Package = CreatePackage(*FullPackagePath);
	if (!IsValid(Package)) {
		return nullptr;
	}

	Package->FullyLoad();
	if (!Package) {
		UE_LOG(LogTemp, Warning, TEXT("Could not create SkeletalMesh package at path %s"), *FullPackagePath);
		return nullptr;
	}

	// Create an asset if it doesn't exist
	USkeletalMesh* SkeletalMesh = nullptr;
	UObject* ExistingAsset = StaticFindObject(nullptr, Package, *AssetName.ToString());
	if(ExistingAsset && ExistingAsset->GetClass()->IsChildOf(USkeletalMesh::StaticClass())){
		SkeletalMesh = Cast<USkeletalMesh>(ExistingAsset);
		SkeletalMesh->ReleaseResources();
		SkeletalMesh->ReleaseResourcesFence.Wait();
		SkeletalMesh->PreEditChange(nullptr);
	}
	else {
		SkeletalMesh = NewObject<USkeletalMesh>(Package, AssetName, RF_Public | RF_Standalone);
	}
	
    // Create skeleton object
	USkeleton* Skeleton = NewObject<USkeleton>(Package, FName("S_" + AssetName.ToString()), RF_Public | RF_Standalone);
	FReferenceSkeleton RefSkeleton;
	FSkeletalMeshImportData SkelMeshImportData;
	int32 SkeletalDepth;
	if (!SkeletalMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not create SkeletalMesh asset %s"), *AssetName.ToString());
		return nullptr;
	}

	FSkeletalMeshModel* ImportedResource = SkeletalMesh->GetImportedModel();
	//check(ImportedResource->LODModels.Num() == 0);
	ImportedResource->LODModels.Empty();
	ImportedResource->LODModels.Add(new FSkeletalMeshLODModel());
	const int32 ImportLODModelIndex = 0;
	FSkeletalMeshLODModel& LODModel = ImportedResource->LODModels[ImportLODModelIndex];
	
	// Build bone list for reference skeleton
	for (size_t bone_idx = 0; bone_idx < Bones.Num(); ++bone_idx) {
		FTransform ParentSpaceTransform;
		if (BoneParents[bone_idx] > -1) {
			ParentSpaceTransform = BoneParentSpaceTransforms[bone_idx];
		}
		//auto ParentSpaceTransform = BoneParentSpaceTransforms[bone_idx];
		SkeletalMeshImportData::FJointPos Transform;
		Transform.Transform = FTransform3f(FRotator3f(ParentSpaceTransform.GetRotation().Rotator()), FVector3f(ParentSpaceTransform.GetLocation()), FVector3f(ParentSpaceTransform.GetScale3D()));
		SkeletalMeshImportData::FBone bone{
			Bones[bone_idx].ToString(),
			0,
			-1,
			BoneParents[bone_idx],
			Transform
		};
		SkelMeshImportData.RefBonesBinary.Add(bone);
	}

	// Build and assign the reference skeleton
	SkeletalMeshImportUtils::ProcessImportMeshSkeleton(Skeleton, RefSkeleton, SkeletalDepth, SkelMeshImportData);
	SkeletalMesh->SetSkeleton(Skeleton);
	SkeletalMesh->SetRefSkeleton(RefSkeleton);
	SkeletalMesh->CalculateRequiredBones(LODModel, RefSkeleton, /*BonesToRemove=*/ nullptr);
	SkeletalMesh->CalculateInvRefMatrices();

	// Begin LOD setup
	SkeletalMesh->SetLODImportedDataVersions(ImportLODModelIndex, ESkeletalMeshGeoImportVersions::LatestVersion, ESkeletalMeshSkinningImportVersions::LatestVersion);
	SkeletalMesh->ResetLODInfo();
	FSkeletalMeshLODInfo& NewLODInfo = SkeletalMesh->AddLODInfo();

	//The imported LOD is always 0 here, the LOD custom import will import the LOD alone(in a temporary skeletalmesh) and add it to the base skeletal mesh later
	check(SkeletalMesh->GetLODInfo(ImportLODModelIndex) != nullptr);
	
	//Set the build options
	SkeletalMesh->GetLODInfo(ImportLODModelIndex)->BuildSettings = FSkeletalMeshBuildSettings();
	
	//New MeshDescription build process
	IMeshBuilderModule& MeshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
	
	//We must build the LODModel so we can restore properly the mesh, but we do not have to regenerate LODs
	const bool bRegenDepLODs = false;
	FSkeletalMeshBuildParameters SkeletalMeshBuildParameters(SkeletalMesh, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), ImportLODModelIndex, bRegenDepLODs);
	bool bBuildSuccess = MeshBuilderModule.BuildSkeletalMesh(SkeletalMeshBuildParameters);
	check(bBuildSuccess);

	// Create initial bounding box based on expanded version of reference pose for meshes without physics assets
	FBox3f BoundingBox(ForceInitToZero);
	for (size_t idx = 0; idx < SkeletalMesh->GetRefSkeleton().GetNum(); ++idx) {
		UE_LOG(LogTemp, Log, TEXT("Bone position %s"), *FVector3f(SkeletalMesh->GetRefSkeleton().GetRefBonePose()[idx].GetLocation()).ToString());
		BoundingBox += FVector3f(SkeletalMesh->GetRefSkeleton().GetRefBonePose()[idx].GetLocation());
	}
	FBox3f Temp = BoundingBox;
	FVector3f MidMesh = 0.5f * (Temp.Min + Temp.Max);
	BoundingBox.Min = Temp.Min + 1.0f * (Temp.Min - MidMesh);
	BoundingBox.Max = Temp.Max + 1.0f * (Temp.Max - MidMesh);
	BoundingBox.Min[2] = Temp.Min[2] + 0.1f * (Temp.Min[2] - MidMesh[2]);
	SkeletalMesh->SetImportedBounds(FBoxSphereBounds(FBox3d(BoundingBox)));

	SkeletalMesh->PostEditChange();
	Skeleton->PostEditChange();

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs PackageArgs;
	PackageArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	PackageArgs.bForceByteSwapping = true;
	bool bSaved = UPackage::SavePackage(Package, SkeletalMesh, *PackageFileName, PackageArgs);
	bSaved = UPackage::SavePackage(Package, Skeleton, *PackageFileName, PackageArgs);

	return SkeletalMesh;
#endif //else !WITH_EDITOR || !WITH_EDITORONLY_DATA
	
}

void USkeletonGeneratorBPLibrary::BindSkeletonSkinWeights(USkeletalMesh* SkeletalMesh)
{
	/*auto SkinBindingTool = NewObject<USkinWeightsBindingTool>(GetTransientPackage());
	auto Targets = TArray<USkeletalMesh*>{ SkeletalMesh };
	SkinBindingTool->SetTargets(Targets);*/
	FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
	TWeakPtr<ILevelEditor> LevelEditorPtr = LevelEditorModule->GetLevelEditorInstance();

	if (auto LevelEditor = LevelEditorPtr.Pin()) {
		// Set target for skin bind tool
		
		/*const FToolTargetTypeRequirements ToolTargetRequirements =
			FToolTargetTypeRequirements({
				UDynamicMeshCommitter::StaticClass(),
				UDynamicMeshProvider::StaticClass()
			});*/

		//auto Target = LevelEditor->GetEditorModeManager().GetInteractiveToolsContext()->TargetManager->BuildTarget(SkeletalMesh, FToolTargetTypeRequirements::)

		LevelEditor->GetEditorModeManager().GetInteractiveToolsContext()->ToolManager->SelectActiveToolType(EToolSide::Left, TEXT("BeginSkinWeightsPaintTool"));
		LevelEditor->GetEditorModeManager().GetInteractiveToolsContext()->ToolManager->ActivateTool(EToolSide::Left);
		auto ActiveTool = LevelEditorPtr.Pin()->GetEditorModeManager().GetInteractiveToolsContext()->ToolManager->GetActiveTool(EToolSide::Left);
		check(ActiveTool);

	}
	
}


UDynamicMesh* USkeletonGeneratorBPLibrary::SetNumWeightLayers(
	UDynamicMesh* TargetMesh,
	int32 NumLayers,
	bool bDeferChangeNotifications) 
{
	if (TargetMesh)
	{
		TargetMesh->EditMesh([&](FDynamicMesh3& EditMesh)
			{
				EditMesh.Attributes()->SetNumWeightLayers(NumLayers);
			}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, bDeferChangeNotifications);
	}
	return TargetMesh;
}
