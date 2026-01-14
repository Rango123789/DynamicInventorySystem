// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/ItemManifest.h"

#include "ActorComponent/Inv_ItemComponent.h"
#include "InventoryTags/InventoryTags.h"
#include "Items/ItemData.h"
#include "Widgets/CompositePattern/UW_Inv_CompositeBase.h"

UItemData* FItemManifest::ManifestItemData(UObject* Outer)
{
	UItemData* CreatedItemData = NewObject<UItemData>(Outer);

	/*OPTION1: this must be done before the copy is done (for the worst case): = I think this is the better/correct option
	*/
		for (TInstancedStruct<FItemFragment>& ItemFragment : ItemFragments) //do not use "auto" it still give you "const" lol
		{
			//if you don't add <T> it give you the BaseStruct
			ItemFragment.GetMutable().InitializeFragment();
		}
	
	//this make a copy (according to GPT && verified by Stephen as well!)
	CreatedItemData->SetItemManifestWrapperWithItemManifest(*this);

	/*OPTION2: you can indeed loop through CreatedItemData->ItemManifest-> ItemFragments instead. But risky because what if there is a remainder lol
		for (TInstancedStruct<FItemFragment>& ItemFragment : CreatedItemData->GetItemManifestMutable().ItemFragments) //do not use "auto" it still give you "const" lol. Also "&" is a must to delete the right data lol
		{
			//if you don't add <T> it give you the BaseStruct
			ItemFragment.GetMutable().InitializeFragment();
		}
	*/

	/*ClearFragments() HERE will have effect on BP_Item::SourceItemManifest, not ItemData::ItemManifest = copy of___
	 *Warning: according to my calculation, you don't want to do this, even if it is on BP_Item::SourceItemManifest, because what if there is a remainder? in the case there is no remainder you don't need to do this anyway because BP_Item::SourceItemManifest will destroy with BP_Item anyway!
		for (TInstancedStruct<FItemFragment> ItemFragment : ItemFragments)
		{
			ItemFragment.Reset(); //reset to empty
		}
		ItemFragments.Empty();
	-->Conclusion: this effect is done on BP_Item::SourceItemManifest not ItemData::ItemManifest (just get a copy) - but this is a mistake anyway
	*/

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

void FItemManifest::AssimilateWidgetFragmentsToCompositeWidget(UUW_Inv_CompositeBase* CompositeBase) const
{
	TArray<const FItemFragment_Widget*> WidgetFragments = GetAllFragmentsByType<FItemFragment_Widget>();

	/* n_ WidgetFragments * m_Leaves
	- (m_Leaves = all widgets within WBP_ItemDesc of type/inherit from UUW_CompositeBase)
	- (n = depending on how you configure BP_Item::SourceItemManifest -->ItemData::ItemManifest */
	for (const FItemFragment_Widget*  WidgetFragment : WidgetFragments)
	{
		//CompositeBase here to initiate the function, where each Leaf is the one need its tag to check again the current ItemFragment_Widget::FragmentTag, check all the leaves for this turn until it found the associate WBP_Leaf_X to expand or more
		CompositeBase->ApplyFunction([&](UUW_Inv_CompositeBase* Leaf)
		{
			WidgetFragment->Assimilate(Leaf);		
		});
	}
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
