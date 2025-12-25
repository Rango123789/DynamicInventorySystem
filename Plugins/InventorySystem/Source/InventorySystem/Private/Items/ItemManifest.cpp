// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/ItemManifest.h"

#include "InventoryTags/InventoryTags.h"
#include "Items/ItemData.h"

UItemData* FItemManifest::ManifestItemData(UObject* Outer) const
{
	UItemData* CreatedItemData = NewObject<UItemData>(Outer);
	
	CreatedItemData->SetItemManifestWrapperWithItemManifest(*this);
	
	return CreatedItemData;
}

FIntPoint FItemManifest::GetGridDimensions() const
{
	const FItemFragment_Grid* ItemFragment_Grid = GetItemFragmentByTag<FItemFragment_Grid>(ItemFragmentTags::Fragment_Grid);
	return ItemFragment_Grid ? ItemFragment_Grid->GridDimensions : FIntPoint(1, 1);
}

int32 FItemManifest::GetMaxStackSize() const
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
