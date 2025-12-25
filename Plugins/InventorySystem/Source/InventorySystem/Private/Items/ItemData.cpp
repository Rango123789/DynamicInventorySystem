// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/ItemData.h"
#include "Net/UnrealNetwork.h"

UItemData::UItemData()
{
	//it is just UObject, just override IsSupportedForNetworking()  return true (no such a thing like bReplicates nor SetIsReplicated here)
}

//stephen forgot this:
bool UItemData::IsSupportedForNetworking() const
{
	return true;
}


void UItemData::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	//always don't this for any UObject/UActorComponent/Actor::Vars to be replicated:
	DOREPLIFETIME(ThisClass, ItemManifestWrapper);
	DOREPLIFETIME(ThisClass, TotalStackCount);
}

void UItemData::SetItemManifestWrapperWithItemManifest(const FItemManifest& InItemManifest)
{
	ItemManifestWrapper = FInstancedStruct::Make<FItemManifest>(InItemManifest);
}

bool UItemData::IsStackable() const
{
	const FItemFragment_Stackable* ItemFragment_Stackable =
		GetItemManifest().GetItemFragmentByType<FItemFragment_Stackable>();
	return ItemFragment_Stackable != nullptr;
}

const FGameplayTag& UItemData::GetItemTag() const
{
	return GetItemManifest().ItemTag;
}

const EItemCategory& UItemData::GetItemCategory() const
{
	return GetItemManifest().ItemCategory;
}

bool UItemData::IsItemOfType(const FGameplayTag& ItemTagToCheck) const
{
	return GetItemTag() == ItemTagToCheck;
}
