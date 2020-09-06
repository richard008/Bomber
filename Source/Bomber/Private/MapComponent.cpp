﻿// Copyright 2020 Yevhenii Selivanov.

#include "MapComponent.h"
//---
#include "Bomber.h"
#include "GeneratedMap.h"
#include "LevelActorDataAsset.h"
#include "SingletonLibrary.h"
//---
#include "Components/BoxComponent.h"

// Sets default values for this component's properties
UMapComponent::UMapComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Initialize the Box Collision component
	BoxCollision = CreateDefaultSubobject<UBoxComponent>("BoxCollisionComponent");
}

// Updates a owner's state. Should be called in the owner's OnConstruction event.
void UMapComponent::OnComponentConstruct(UMeshComponent* MeshComponent, FLevelActorMeshRow ComparedMeshRowTypes)
{
	AGeneratedMap* LevelMap = USingletonLibrary::GetLevelMap();
	AActor* Owner = GetOwner();
	if (!IS_VALID(Owner)  	// The owner is not valid
        || !LevelMap)		   	// The Level Map is not valid
	{
		return;
	}

	// Find mesh
	if(ComparedMeshRowTypes.LevelType == ELT::None)
	{
		ComparedMeshRowTypes.LevelType = LevelMap->GetLevelType();
	}
	ActorDataAssetInternal->GetMeshRowByTypes(ComparedMeshRowTypes);

	// Set mesh
	if (auto SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
	{
		SkeletalMeshComponent->SetSkeletalMesh(Cast<USkeletalMesh>(ComparedMeshRowTypes.Mesh));
	}
	else if (auto StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
	{
		StaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(ComparedMeshRowTypes.Mesh));
	}


	// Find new Location at dragging and update-delegate
	USingletonLibrary::PrintToLog(Owner, "OnMapComponentConstruction", "-> \t FCell()");
	LevelMap->SetNearestCell(this);

	// Owner updating
	USingletonLibrary::PrintToLog(Owner, "OnMapComponentConstruction", "-> \t AddToGrid");
	USingletonLibrary::GetLevelMap()->AddToGrid(Cell, this);

#if WITH_EDITOR	 // [IsEditorNotPieWorld]
	if (USingletonLibrary::IsEditorNotPieWorld())
	{
		// Remove all text renders of the Owner
		USingletonLibrary::PrintToLog(Owner, "[IsEditorNotPieWorld]OnMapComponentConstruction", "-> \t ClearOwnerTextRenders");
		USingletonLibrary::ClearOwnerTextRenders(Owner);

		// Update AI renders after adding obj to map
		USingletonLibrary::PrintToLog(Owner, "[IsEditorNotPieWorld]OnMapComponentConstruction", "-> \t BroadcastAiUpdating");
		USingletonLibrary::GOnAIUpdatedDelegate.Broadcast();
	}
#endif	//WITH_EDITOR [IsEditorNotPieWorld]
}

//  Called when a component is registered (not loaded
void UMapComponent::OnRegister()
{
	Super::OnRegister();
	AActor* Owner = GetOwner();
	if (!IS_VALID(Owner))  // owner is not valid
	{
		return;
	}
	USingletonLibrary::PrintToLog(Owner, "OnRegister", "");

	// Disable the tick
	Owner->SetActorTickEnabled(false);

	// Set the movable mobility for in-game attaching
	if (Owner->GetRootComponent() != nullptr)
	{
		Owner->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	}

	// Finding the actor data asset
	ActorDataAssetInternal = USingletonLibrary::GetDataAssetByActorClass(Owner->GetClass());
	ensureMsgf(ActorDataAssetInternal, TEXT("ASSERT: 'the Actor Data Asset' was not found"));

	// Initialize the Box Collision Component
	if (ActorDataAssetInternal
	    && ensureMsgf(BoxCollision, TEXT("ASSERT: 'BoxCollisionInternal' is not valid")))
	{
		BoxCollision->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		BoxCollision->SetBoxExtent(ActorDataAssetInternal->GetCollisionExtent());
		BoxCollision->SetCollisionResponseToAllChannels(ActorDataAssetInternal->GetCollisionResponse());
#if WITH_EDITOR
		BoxCollision->SetHiddenInGame(!bShouldShowRenders);
#endif
	}

#if WITH_EDITOR	 // [IsEditorNotPieWorld]
	if (USingletonLibrary::IsEditorNotPieWorld())
	{
		// Should not call OnConstruction on drag events
		Owner->bRunConstructionScriptOnDrag = false;
	}
#endif	//WITH_EDITOR [IsEditorNotPieWorld]
}

// Called when a component is destroyed for removing the owner from the Level Map.
void UMapComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	AActor* const ComponentOwner = GetOwner();
	if (IS_TRANSIENT(ComponentOwner) == false		   // Is not transient owner
		&& IsValid(USingletonLibrary::GetLevelMap()))  // is valid and is not transient the level map
	{
		USingletonLibrary::PrintToLog(ComponentOwner, "OnComponentDestroyed", "-> \t DestroyActorsFromMap");

		// During a game: destroyed bombs, pickup-ed items
		USingletonLibrary::GetLevelMap()->RemoveMapComponent(this);

		//disable collision for safety
		ComponentOwner->SetActorEnableCollision(false);

		// @TODO Delete spawned collision component
		//

#if WITH_EDITOR	 // [IsEditorNotPieWorld]
		if (USingletonLibrary::IsEditorNotPieWorld())
		{
			// Editor delegates
			USingletonLibrary::GOnAIUpdatedDelegate.Broadcast();
		}
#endif	//WITH_EDITOR [IsEditorNotPieWorld]
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}
