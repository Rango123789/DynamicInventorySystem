// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

/**
 * 
 */

class UItemData;
//I think I will keep [Inv_] pattern, because who know in other project somebody use "EItemType" name for other purpose lol
//also for "inventory" I choose the terms "Category" instead of "type" for items!
UENUM(BlueprintType)
enum class EItemCategory : uint8	
{
	None,
	Equippable,
	Consumable,
	Craftable
};


USTRUCT(BlueprintType)
struct FInventorySlotInfo
{
	GENERATED_BODY()

	/*
	FInventorySlotInfo(){}
	FInventorySlotInfo(const int32& InSlotIndex, const int32& InAmountToFill, bool InIsItemAtIndex) :
		SlotArrayIndex(InSlotIndex), AmountToFill(InAmountToFill), IsItemAtIndex(InIsItemAtIndex) {}
	*/
	
	//we need to know the index of the slot to work (it is identical to  ::WBP_Inventory::InventoryGrid::WBP_GridSlot::SlotIndex? - yes, but it may not corresponding to the key in TMap<int32, WBP_Item> )
	//it is corresponding to "index in 1DArray" (if you need to calculate WBP_Item's DrawPos, you need to convert ArrayIndex to PositionIndex = we have helper function in BPFuncLibrary!)
	int32 SlotArrayIndex = INDEX_NONE;

	//if it is for Slot for stackable items (i.e ::WBP_Grid_Consumable+), we need to know how many stacks left this Slot can allow you can fill on:
	int32 AmountToFill = 0;
	
	//whether the widget of a stackable item is already there, hence no need to create new widget when you can still stack it
	bool IsItemAtIndex = false;
	
};

//we will check BP_Item(Potion...)::ItemComponent against PC::InventoryComp::WBP_Inventory_Spacial{...} to decide its values - so basically it is specific to a specific BP_Item::ItemComponent (not represent for all possible Items in world at once)
USTRUCT(BlueprintType)
struct FInventoryAvailabilityInfo
{
	GENERATED_BODY()

	FInventoryAvailabilityInfo(){} //no need 
	
	int32 TotalRoomToFill = 0;
	int32 Remainder = 0;
	bool bStackable = false;

	TArray<FInventorySlotInfo> SlotInfos;	

	//this is currently empty and ONLY appear in F___FastArray::Items::ItemData so far, the  F___FastArray will be declared in PC::InventoryComponent (but not done yet lol)
	//apparently this is for convenient, not mean "UItemData" already in WBP_Grid or not (it is the SlotInfo::bItemAtIndex that tells it instead I guess), but anyway let's see
	TWeakObjectPtr<UItemData> ItemData;
};

//can name it ESlotQuadrant
UENUM(BlueprintType)
enum class ETileQuadrant : uint8
{
	TopLeft,      //NW
	TopRight,    //NE
	BottomLeft,  //SW
	BottomRight  //SE
};

//can name it FSlotParameters
USTRUCT(BlueprintType)
struct FTileParameters
{
	GENERATED_BODY()

	//the current SlotIndex you hover on while wearing WBP_HoverItem
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 HoveredIndex{INDEX_NONE};

	//its Position{XSlot * YSlot} to the top-left Canvas
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FIntPoint HoveredNormalizedPosition; //rename from "Coordinates", DO NOT name it GridDimensions lol

	//which part of HoveredGridSlot the mouse is on
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	ETileQuadrant TileQuadrant;
};

/*when you define a FREE "bool operator==" (outside of the type)
, they typically make it "inline"
, because it typically is "short function" or else?*/
inline bool operator==(const FTileParameters& A, const FTileParameters& B)
{
	return A.HoveredIndex == B.HoveredIndex &&
		   A.HoveredNormalizedPosition == B.HoveredNormalizedPosition &&
		   A.TileQuadrant == B.TileQuadrant;
};

USTRUCT(BlueprintType)
struct FSpaceQueryResult
{
	GENERATED_BODY()

	//true if the space queried has no items in it
	bool bHasSpace{false};

	//valid if there is a single item in the space that we can swap with (if this is true then bHasSpace wil be naturally false)
	TWeakObjectPtr<UItemData> PreoccupiedItemData{nullptr};

	//the Top Left Index of the PreoccupiedItemData if there is one (not when 0 nor 2+)
	int32 TopLeftSlotIndexOfPreoccupiedItem = INDEX_NONE;
};






