// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "Items/ItemManifest.h"
#include "ItemData.generated.h"

//struct FItemManifest;
/**
 * I name it ItemData (similar to ListData_X,Y,Z), where stpep
 * it will represent an item, it is a UObject holding data, not AActor
 *
 * IMPORTANT realization:
 - UItemData only has the FItemManifest member (I just realize this lol)
 - so the only reason why we use UObject around FItemManifest is that we need "a decent UObject" to be added and replicated as Sub Object of PC::InventoryComponent! we have as many UObject as many number of Item in our PlayerInventory
 - so yeah!
 */
UCLASS(Blueprintable, BlueprintType)
class INVENTORYSYSTEM_API UItemData : public UObject
{
	GENERATED_BODY()
	
public:
	UItemData();
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//you want "reference", and you want "literal object", not pointer (common for struct)
	void SetItemManifestWrapperWithItemManifest(const FItemManifest& InItemManifest);
	//use this version for accessing not modifying
	const FItemManifest& GetItemManifest() const{ return ItemManifestWrapper.Get<FItemManifest>();};
	//use this version for modifying (directly access to the Wrapper also have the same outcome)
	FItemManifest& GetItemManifestMutable() { return ItemManifestWrapper.GetMutable<FItemManifest>();}

//these are just optional and for convenience should we need to access anything in ItemData::ItemManifest::Values directly from ItemData: (you can always create them directly from FItemManifest itself)
	bool IsStackable() const;
	const FGameplayTag& GetItemTag() const;
	const EItemCategory& GetItemCategory() const;
	bool IsItemOfType(const FGameplayTag& ItemTagToCheck) const;
protected:
	/*used FInstancedStruct<FItemManifiest+> instead FItemManifiest so that it can change/swap out any children of FItemManifiest (or any generict if you don't use meta to restruct its base struct class at first place)
	-because it is now in UObject, you want it replicated, you need "Replicated" (UPROPERTY() like struct is not enough)
	- meta = (BaseStruct = "/Script/[ModuleNameContainingTheStructType].[StructNameNotIncludingPrefixF]
	- it can be marked "Replicated" to be replicate like normal struct of course, and the underlying struct will be replicated.

	VERY IMPORTANT:
	not only you must add "StructUtils" module, but also include the specified "BaseStruct" right here, because it at least need to know the "BaseStruct" type at compile time - or at least when you specify it
	*/
	//PROPERTY(Replicated, EditAnywhere, Category = "Inventory") - if you don't limit it, it will show all possible FStruct in BP to select! "BaseStruct"is a bit like "TSubclassOf<UObject>", where FInstancedStruct  is a LITERAL struct object (not TSubClassOf<T>)
	UPROPERTY(Replicated, VisibleAnywhere, meta = (BaseStruct = "/Script/InventorySystem.ItemManifest")) 
	FInstancedStruct ItemManifestWrapper;

	//this also works: (keep keep it here for fun)
	UPROPERTY(Replicated, VisibleAnywhere) 
	TInstancedStruct<FItemManifest> ItemManifestWrapper_test;

public:
	//all WBP_SlottedItem[s] represent a stackable BP_Item of the same type (having the same ItemTag) will have a single owning ItemData (no matter it is picked at different times)
	//where for non-stackable item, this doesn't make sense nor needed(because 2 instances of the same non-stackable item type will be added as 2 separate entries/items to FastArray), consider it "0" or "1" doesn't make any difference
	UPROPERTY(Replicated)
	int32 TotalStackCount = 0;
};

//FREE function or also member is upto you. But by this way we can also accept an invalid ItemData - explain why stephen prefers this:
//"const" is not applied on function() const {}, but is still applied to "const ReturnType"!
template <typename T>
const T* GetItemFragmentByTag(UItemData* ItemData, const FGameplayTag& FragmentTag) 
{
	if (IsValid(ItemData) == false) return nullptr;

	FItemManifest ItemManifest = ItemData->GetItemManifest();
	
	return ItemManifest.GetItemFragmentByTag<T>(FragmentTag);
}