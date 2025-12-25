// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"
#include "CustomFastArrayTypes.generated.h"

struct FGameplayTag;
class UInv_ItemComponent;
class UItemData;
/**
 *  Read FastArraySerializer to quickly know to do it (no point in remembering it)
 *  I add "Entry" just to make it clear that it is not "an actor" nor "UObject - ItemData", it is just element of the fast array
 */
USTRUCT()
struct FInventoryItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

//our underlying data:
	UPROPERTY()
	TObjectPtr<UItemData> ItemData = nullptr;

/*Bonus:
 B is friend of A 
	<=> (pointer or literal A is the same)
	- if B has B::A a, then you can do B::a.[PrivateMember]
	- if B has B::function(A a), then you can do {a.[PrivateMember]} 
	
B is friend of A
	<=> Inside B, all A::private members are visible
 */
	//the DOC example didn't require this, so this is redundant
	friend struct FInventoryItemFastArray;

	//this is our external need, not directly relevant to FastArray at all: (this mean if )
	friend class UInv_InventoryComponent;	//NOT U_ItemComponent which is totally irrelevant here, we make it component of BP_Potion to FindComponentByClass and apply highlight and unhighlight it nothing more
};

USTRUCT()
struct FInventoryItemFastArray : public FFastArraySerializer
{
	GENERATED_BODY()
	FInventoryItemFastArray() : OwningActorComponent(nullptr){};
	FInventoryItemFastArray(UActorComponent* InOwningComponent) : OwningActorComponent(InOwningComponent){};
	FInventoryItemFastArray(UInv_InventoryComponent* InOwningInventoryComponent);
	
	TArray<UItemData*> GetAllItemData();

//conventional must:
	UPROPERTY()
	TArray<FInventoryItemEntry> ItemEntries;
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryItemEntry, FInventoryItemFastArray>( ItemEntries, DeltaParms, *this );
	}

//built-in optional: (go and copy the exact signature)
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize); //stephen don't have this
	
//should always: (params and return type are up to you)
	//we decide to pass in FItemEntry::UItemData instead of FItemEntry directly, because we will construct one or find and remove the one from it and it is convenient to use overtime:
	UItemData* AddItemEntry(UItemData* ItemDataToAdd); //funny why do I need the return of what I just pass in, because it may help me know if it is success or help?
	UItemData* AddItemEntry(UInv_ItemComponent* ItemComponent); //this time it is BP_Potion::ItemComponent  (not PC::InventoryComponent - huge one) - it sounds like BP_Potion+ will all have ItemComponent and its ItemData - of course :D :D
	
	void RemoveItemEntry(UItemData* ItemDataToRemove);

//extra helpers:
	UItemData* FindItemDataByItemTag(const FGameplayTag& ItemTag);
	
//bonus:
	/*because we're gonna create ::UPROPERTY(Replicated) FInventoryItemFastArray, where UPROPERTY() member of a struct will be replicated by default, hence if we don't want it to be replicated (which we don't here in this struct) we need to explicitly mark it "NotReplicated"
	-note that replicate T* (that is NOT a ActorComponent/Actor[/UObject] won't make sense, but here it its TObjectPtr<UActorComponent> so it does make sense lol, so we must avoid it here (either because it makes sense and we don't want it or because it doesn't make sense)
	-anyway it is just "pointer", it will be replicated when "pointer change address to new one or to nullptr"*/
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwningActorComponent;

	UPROPERTY(NotReplicated)
	TObjectPtr<UInv_InventoryComponent> OwningInventoryComponent;

	friend class UInv_InventoryComponent;
};

template<>
struct TStructOpsTypeTraits< FInventoryItemFastArray > : public TStructOpsTypeTraitsBase2< FInventoryItemFastArray >
{
	enum 
	{
		WithNetDeltaSerializer = true,
   };
};

