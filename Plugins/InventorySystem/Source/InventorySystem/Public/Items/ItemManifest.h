// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ItemFragment/ItemFragment.h"
#include "StructUtils/InstancedStruct.h"
#include "Types/InventoryTypes.h"
#include "UObject/Object.h"
#include "ItemManifest.generated.h"

enum class EItemCategory : uint8;
class UItemData;

/**
* call it FItemDetails/FItemsProperties if you want
* just struct containing all details of an item (hence it is member of UItemData)
, calling it Manifest is misleading
; it is just so optional that we give it the ability to newly create an ItemData instance on the HEAP and then assign itself to that newly created ItemData
; also this UItemData need to be replicated as subobject of PC::InventoryComponent (the conventional steps you will see), hence we add extra "OwningObject/Outer" param
 */
USTRUCT()
struct FItemManifest 
{
	GENERATED_BODY()

	/*By "manifest an item data" here i mean "newly create an item data, and then assign this ItemManifest/ItemProperties/ItemDetails to the CreatedItemData
	  Outer should be passed as PC/PC::InventoryComponent for the sake of the requirement of an Outer for replication:*/
	UItemData* ManifestItemData(UObject* Outer) const;

	// meta = (Categories= "Filter.Tag") only applied for FGameplayTag and F___Container, if it is used on other types it will be ignored
	//stephen didn't have this, I follow Vince good practice to filter out some!
	UPROPERTY(EditAnywhere, Category="Inventory", meta = (Categories = "Item"))
	FGameplayTag ItemTag; //this will know exactly which specific BP_Item_A_B_X
	
	//again: "EditAnywhere" TIRE0 make make sub-members of FStruct tire1,2,3 automatically "EditAnywhere" too, hence you need to do it in all TIREs (not the first time I face this)
	UPROPERTY(EditAnywhere, Category="Inventory")
	EItemCategory ItemCategory = EItemCategory::None; //this only help to know which category it is in

	UPROPERTY(EditAnywhere, Category="Inventory", meta=(ExcludeBaseStruct))
	TArray<TInstancedStruct<FItemFragment>> ItemFragments;  //TArray<FInstancedStruct> BaseStruct=/Script/... also okay

	/*you can directly use this (by accessing FItemManifest first from ItemComponent or UItemData) or Use the overload accepting "UItemData" and "FragmentTag" - either is fine!
	 - you can put requires std::derived_from<T, FItemFragment> there as well if you, but it is REDUNDANT
	 - universal rule: put this "requires" anywhere in the chain, one place is enough!
	 */
	template <class T>
	requires std::derived_from<T, FItemFragment> //this is new to me, but old in C++
	const T* GetItemFragmentByTag(const FGameplayTag& FragmentTag) const;

	//this return the first-found one without checking for tag. We don't create FREE function wrap around this one in ItemData.h for this one: (it is optional even for the function above)
	template <class T>
	requires std::derived_from<T, FItemFragment> //this is new to me, but old in C++
	const T* GetItemFragmentByType() const;

	template <class T>
	requires std::derived_from<T, FItemFragment>
	T* GetMutableItemFragmentByType();

public: //these are also optional helper (create it here or in ItemData both okay)
	FIntPoint GetGridDimensions() const;
	int32 GetMaxStackSize() const;
	bool IsStackable() const;
};

//if it is a template it must be defined in the same .h file remember?
template <class T>
requires std::derived_from<T, FItemFragment>
const T* FItemManifest::GetItemFragmentByTag(const FGameplayTag& FragmentTag) const
{
	for (const TInstancedStruct<FItemFragment>& ItemFragmentWrapper : ItemFragments)
	{
		/*IMPORTANT:
		 (1) This will "crash" if <T> doesn't match the underlying object of the ItemFragmentWrapper. We can't use it here, because there is either one of ItemFragments match the T type.
		-Hence in case we want to return  "T" we must use ItemFragmentWrapper.GetPtr<T>() first before return ItemFragmentWrapper.Get<T>()
		-However here we want to return T*  (which is here), hence just directly return ItemFragmentWrapper.GetPtr<T>() -- it's up to you, but return a pointer so that we can check if it exist.
			T ItemFragment = ItemFragmentWrapper.Get<T>();
	    (2) This will only return "nullptr" if <T> doesn't match the underlying object of the ItemFragmentWrapper, this is exactly what we need
		T* ItemFragmentPointer = ItemFragmentWrapper.GetPtr<T>()
		*/
		/*PUZZLE: why we must additionally add the "FragmentTag" to find the ItemFragment_X in the array?
		- well only in case the array only accept elements of different child types it will work, and this is absolutely risky if you do need 2 fragments of the same type for whatever reason
		- not to mention fragment children can possibly inherit from each other as well (we will avoid it, but who know) making ItemFragmentWrapper.GetPtr<T>() go "true" for more than one ItemFragmentWrappers
		- hence a tag is for such complication is surely needed
		*/
		if (const T* ItemFragmentPointer = ItemFragmentWrapper.GetPtr<T>())
		{
			//this "ItemFragmentWrapper.GetPtr<T>()" return a "const T", for the hosting template to be function() const {} as well - including the wrapper one using it in the chain (FREE function don't need to add const at "function() const {}" - but still need "const ReturnType"), to be exact it is not allowed nor needed) - more like a virus
			//you can simply "hack it if you want" but anyway I don't have the need to modify my ItemFragments anyway, I use it for "references"
			if (ItemFragmentPointer->FragmentTag == FragmentTag)
			{
				return ItemFragmentPointer;
			}
		}
	}
	
	return nullptr;
}

template <class T> requires std::derived_from<T, FItemFragment>
const T* FItemManifest::GetItemFragmentByType() const
{
		for (const TInstancedStruct<FItemFragment>& ItemFragmentWrapper : ItemFragments)
	{
		if (const T* ItemFragmentPointer = ItemFragmentWrapper.GetPtr<T>())
		{
			return ItemFragmentPointer;
		}
	}
	
	return nullptr;
}

//remove all const
template <class T> requires std::derived_from<T, FItemFragment>
T* FItemManifest::GetMutableItemFragmentByType() 
{
	//the reason why you can't use T&" is because you use function() const {...} lol - just remove it
	for (TInstancedStruct<FItemFragment>& ItemFragmentWrapper : ItemFragments)
	{
		if (T* ItemFragmentPointer = ItemFragmentWrapper.GetMutablePtr<T>()) //replace .GetPtr<T>()
		{
			return ItemFragmentPointer;
		}
	}
	
	return nullptr;
}