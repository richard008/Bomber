// Fill out your copyright notice in the Description page of Project Settings.

#include "MapComponent.h"

#include "Bomber.h"
#include "GameFramework/Character.h"
#include "GeneratedMap.h"

// Sets default values for this component's properties
UMapComponent::UMapComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

void UMapComponent::UpdateSelfOnMap()
{
	if (ISVALID(GetOwner()) == false							// owner is not valid
		|| ISVALID(USingletonLibrary::GetLevelMap()) == false)  // levelMap is not valid
	{
		return;
	}

	cell = FCell(GetOwner());  // Find new location at dragging and update-delegate
	USingletonLibrary::GetLevelMap()->AddActorOnMapByObj(cell, GetOwner());
	UE_LOG_STR("UpdateSelfOnMap: %s UPDATED", *GetOwner()->GetName());
}

void UMapComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	if (ISVALID(GetOwner()) == false							// owner is not valid
		|| ISVALID(USingletonLibrary::GetLevelMap()) == false)  // levelMap is not valid
	{
		return;
	}

	// Shouldt call OnConstruction on drag events
	GetOwner()->bRunConstructionScriptOnDrag = false;

	// Push owner to regenerated TMap
	USingletonLibrary::GetLevelMap()->onActorsUpdatedDelegate.AddDynamic(this, &UMapComponent::UpdateSelfOnMap);

	UE_LOG_STR("OnComponentCreated: %s", *GetOwner()->GetName());
}

void UMapComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (ISVALID(USingletonLibrary::GetLevelMap()) == true)
	{
		const ACharacter* character = Cast<ACharacter>(GetOwner());
		if (character != nullptr &&
			USingletonLibrary::GetLevelMap()->charactersOnMap_.Contains(character))
		{
			USingletonLibrary::GetLevelMap()->charactersOnMap_.Remove(character);
			UE_LOG_STR("OnComponentDestroyed: %s removed from TSet", *GetOwner()->GetName());
		}
		UE_LOG_STR("OnComponentDestroyed: %s", *GetOwner()->GetName());
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}
