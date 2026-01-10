// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/ItemManifest.h"

#include "ActorComponent/Inv_ItemComponent.h"
#include "InventoryTags/InventoryTags.h"
#include "Items/ItemData.h"

UItemData* FItemManifest::ManifestItemData(UObject* Outer) const
{
	UItemData* CreatedItemData = NewObject<UItemData>(Outer);
	
	CreatedItemData->SetItemManifestWrapperWithItemManifest(*this);
	
	return CreatedItemData;
}

/*you must pass in an WorldObjectContext for it to get the world successfully, WorldObjectContext of Actor is always ULevel/UWorld
 *NOT only this function help to spawn an actor back but also set this manifest as AActor::ItemComponent::SourceItemManifest
 *so the idea is that you modify this manifest to your needbefore calling this function!
*/
void FItemManifest::SpawnDroppedItem(UObject* WorldObjectContext, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	UWorld* World = WorldObjectContext->GetWorld();
	AActor* SpawnActor = World->SpawnActor<AActor>(ItemClassToSpawn, SpawnLocation, SpawnRotation);
	if (IsValid(SpawnActor) == false) return;
	
	UActorComponent* ActorComponent = SpawnActor->GetComponentByClass(UInv_ItemComponent::StaticClass());
	UInv_ItemComponent* ItemComponent = Cast<UInv_ItemComponent>(ActorComponent);
	if (IsValid(ItemComponent) == false) return;

	//this is just a copy assignment, so no worry:
	ItemComponent->SourceItemManifest = *this;
}

FIntPoint FItemManifest::GetGridDimensions() const
{
	const FItemFragment_Grid* ItemFragment_Grid = GetItemFragmentByTag<FItemFragment_Grid>(ItemFragmentTags::Fragment_Grid);
	return ItemFragment_Grid ? ItemFragment_Grid->GridDimensions : FIntPoint(1, 1);
}

int32 FItemManifest::GetMaxStackCount() const
{
	const FItemFragment_Stackable* ItemFragment_Stackable = GetItemFragmentByType<FItemFragment_Stackable>();
	return ItemFragment_Stackable ? ItemFragment_Stackable->MaxStackSize : 1;
}

bool FItemManifest::IsStackable() const
{
	const FItemFragment_Stackable* ItemFragment_Stackable =
	GetItemFragmentByType<FItemFragment_Stackable>();
	return ItemFragment_Stackable != nullptr;
}
