// Fill out your copyright notice in the Description page of Project Settings.


#include "Types/CustomFastArrayTypes.h"
#include "ActorComponent/Inv_InventoryComponent.h"
#include "ActorComponent/Inv_ItemComponent.h"
#include "Items/ItemData.h"
#include "GameplayTagContainer.h"

FInventoryItemFastArray::FInventoryItemFastArray(UInv_InventoryComponent* InOwningInventoryComponent)
: OwningActorComponent(InOwningInventoryComponent), OwningInventoryComponent(InOwningInventoryComponent)
{

};

TArray<UItemData*> FInventoryItemFastArray::GetAllItemData()
{
	TArray<UItemData*> AllItemData;
	AllItemData.Reserve(ItemEntries.Num()); //good practice for better performance

	for (auto& ItemEntry : ItemEntries)
	{
		if(IsValid(ItemEntry.ItemData))
		{
			AllItemData.Add(ItemEntry.ItemData);
		}
	}

	return AllItemData;
}

//why don't we just store InventoryComponent directly for better perforamnce lol
void FInventoryItemFastArray::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	if (IsValid(OwningInventoryComponent) == false) return;

//broadcast OwningComponent::OnItemRemoved() per each RemovedItemEntry
	for (int32 RemovedIndex : RemovedIndices)
	{
		OwningInventoryComponent->OnItemRemoved.Broadcast(ItemEntries[RemovedIndex].ItemData);
	}
}

void FInventoryItemFastArray::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (IsValid(OwningInventoryComponent) == false) return;

	//broadcast OwningComponent::OnItemRemoved() per each RemovedItemEntry
	for (int32 AddedIndex : AddedIndices)
	{
		OwningInventoryComponent->OnItemAdded.Broadcast(ItemEntries[AddedIndex].ItemData);
	}
}

void FInventoryItemFastArray::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
}

//remove -->MarkArrayDirty() (because after remove it no longer exist to refer that the removed item), add/change -->markItemDirty
void FInventoryItemFastArray::RemoveItemEntry(UItemData* ItemDataToRemove)
{
	/*What will not work:
	 (1) this is not safe: remove item while iterating over the array (even if I try to return after first remove)
	for (auto& ItemEntry : ItemEntries)
	{
		if (ItemEntry.ItemData == ItemDataToRemove)
		{
			ItemEntries.Remove(ItemEntry);
			MarkArrayDirty();

			return; //if you want, but it may help to remove duplicate item if we don't return lol?
		}
	}
	
	(2) this doesn't work HERE neither. Simply because .Remove() look for "operator=" where FInventoryItemEntry has "operator=" for replication, we can't override it as we will break it
	FInventoryItemEntry* ItemEntryToRemove{};
	for (FInventoryItemEntry& ItemEntry : ItemEntries)
	{
		if (ItemEntry.ItemData == ItemDataToRemove)
		{
			ItemEntryToRemove = &ItemEntry;
			MarkArrayDirty();
		}
	}
	if (ItemEntryToRemove)	ItemEntries.Remove(*ItemEntryToRemove);
	*/
	/* what will work:
	(1) OPTION1:
	for(int32 i = 0 , i < ItemEntries.num , i++)
	{
	  if(Your condition) 
	  {
		ItemEntries.RemoveAtSwap(i); //this is the key, it doesn't change the array size, hence safe
		MarkArrayDirty();
		break;
	  }
	}

	(2) OPTION2: similar to "failed option", but this time we cache the "index" to remove (not the element)
	int32 IndexToRemove = INDEX_NONE;

	for(int32 i = 0 , i < ItemEntries.num , i++)
	{
	  if(Your condition) 
	  {
		IndexToRemove = i; 
		break;
	  }
	}

	if (IndexToRemove != INDEX_NONE)
	{
		ItemEntries.RemoveAt(IndexToRemove);
		MarkArrayDirty();
	}

	(3) OPTION3: use iterator so that you can even remove it when you loop through the array (I didn't know this lol) - this is the option stephen use in this course! yeah!
	= TArray’s iterator is designed to survive removal, while pointers / references are not.
	for (auto It = ItemEntries.CreateIterator(); It; ++It)
	{
		// Check if this is the entry we want to remove
		if (It->ItemData == ItemDataToRemove)
		{
			// Safely remove the element at the iterator's current index
			// The iterator updates itself so it stays valid
			It.RemoveCurrent();

			// Notify FastArraySerializer that the array structure changed
			MarkArrayDirty();

			// Stop after removing one item (omit return to remove duplicates)
			return;
		}
	}
	 */
	for (auto It = ItemEntries.CreateIterator(); It; ++It)
	{
		FInventoryItemEntry InventoryItemEntry = *It;

		if (InventoryItemEntry.ItemData == ItemDataToRemove)
		{
			It.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

UItemData* FInventoryItemFastArray::FindItemDataByItemTag(const FGameplayTag& ItemTag)
{
	//you can use TArray<T>.FindByPredicate([](const T& Element)->bool {...}  ) as well, but I don't feel the need lol
	//not sure it has algorithm that is better in performance, but I don't care for now lol
	for (FInventoryItemEntry& ItemEntry : ItemEntries)
	{
		if (IsValid(ItemEntry.ItemData) &&
			ItemEntry.ItemData->GetItemManifest().ItemTag == ItemTag)
		{
			return ItemEntry.ItemData;
		}
	}

	return nullptr;
}


/*again it is F___Array need to have OwningComponent, not F__Array::ItemEntries::ItemEntry (not even create it)
; funny F___SerializerItem has operator=, but it is about ReplicationID, so if you want to check if "UItemData*" is currently in any ItemEntries::ItemData or not you must manually loop through and check it, but anyway not sure we even need it:*/
UItemData* FInventoryItemFastArray::AddItemEntry(UItemData* ItemDataToAdd)
{
	//we don't have this step in DS course, because it is handled outside to make sure this is called in the server already, now we would like to handle it locally: (anyway all is up to you)
	check(OwningActorComponent) //make sure you assign it wherever you declare UPROPERTY(Replicated) F__FastArray
	check(OwningActorComponent->GetOwner()) //we will create UPROPERTY(Replicated) F__FastArray in PC::InventoryComponent (hence OwningActorComponent = PC::InventoryComponent too), so GetOwner() here will be PC if you care!
	if (OwningActorComponent->GetOwner()->HasAuthority() == false) return nullptr;
	
	/*OPTION1: the old risky practice (no need to use "NewObject" because it will make a copy when we add anyway?)
		//FInventoryItemEntry InventoryItemEntry1;
		//InventoryItemEntry1.ItemData = ItemDataToAdd;
		//ItemEntries.Add(InventoryItemEntry1);*/

	//OPTION2: better practice
	//Add_GetRef(InItem) will need you to pass in "ItemType{}", where AddDefaulted_GetRef() don't require it!
	FInventoryItemEntry& InventoryItemEntry = ItemEntries.AddDefaulted_GetRef(); //this step already add, do NOT call ItemEntries.Add again
	InventoryItemEntry.ItemData = ItemDataToAdd;

	/*OPTION2: call PostReplicatedAdd() here*/
	
	MarkItemDirty(InventoryItemEntry);
	
	return ItemDataToAdd;
}

//the reason for this overload is that we may not have an UItemData created in PlayerInventory yet
UItemData* FInventoryItemFastArray::AddItemEntry(UInv_ItemComponent* ItemComponent)
{
	check(IsValid(OwningInventoryComponent))
	if (OwningInventoryComponent->GetOwner()->HasAuthority() == false) return nullptr;
	
	AActor* OwningActor	= OwningInventoryComponent->GetOwner(); //it is expected to be PC
	if (!IsValid(OwningActor)) return nullptr; 

	FInventoryItemEntry& InventoryItemEntry = ItemEntries.AddDefaulted_GetRef(); //this step already add, do NOT call ItemEntries.Add again

//This is where we need a brand new ItemData and take care of Replication step:
	/*GPT: Outer (UObject::PrivateOuter) and Owner (AActor::Owner - AActor) are different, Outer is immutable and is assigned at creation of a UObject
	 * UObject::Outer help to define where it is in the package/memory
	-Used by:
	Garbage Collection
	Object naming / paths
	Serialization
	Replication identity (NetGUIDs)
	= Explain why if it is to be replicated, you better off give it an appropriate Outer at creation lol

	- when you do GetWorld()->SpawnActor<>() it doesn't let you to specify Outer, because Outer of a newly spawned actor is always UWorld::ULevel
	, what you can do is to set CreatedActor::Owner via SpawnParams parameter (that indeed affect Replication flow)
	
	 * AActor::Owner = A gameplay relationship
	-Used by:
	RPC routing (Server, Client)
	Network relevancy
	Damage instigation
	Possession logic
	= this is also important of course!
	 
	 * If a UObject is going to be replicated, you must create it with an Outer that is in the same ownership chain as the replicating Actor (typically the Actor or one of its Components).
	 */
	UItemData* CreatedItemData = ItemComponent->SourceItemManifest.ManifestItemData(OwningActor); //CreatedItemData::Outer = OwningActor/PC
	OwningInventoryComponent->AddReplicatedSubObject(CreatedItemData); //will this make OwningInventoryComponent become outer of CreatedItemData too? no! explain why you better off give it an appropriate Outer at creation, otherwise it won't work properly

//back to normal
	InventoryItemEntry.ItemData = CreatedItemData;
	
	MarkItemDirty(InventoryItemEntry);
	
	return CreatedItemData;
}