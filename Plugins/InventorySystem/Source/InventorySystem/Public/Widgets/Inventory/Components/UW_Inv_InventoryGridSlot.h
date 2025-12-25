// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_Inv_InventoryGridSlot.generated.h"

class UItemData;

UENUM(BlueprintType)
enum class ESlotState : uint8
{
	UOccupied,
	Occupied,
	Selected,
	GrayedOut
};

class UImage;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_InventoryGridSlot : public UUserWidget
{
	GENERATED_BODY()
public:
	//I use this single function to set the brush for Image_GridSlot::Brush
	void SetSlotStateAndBrush(ESlotState InSlotState) const;

//all of these are to be assigned, hence need UPROPERTY(__) so far:
	//this one will go hand in hand with its brush
	ESlotState SlotState;

	/*This one is optional, you can also rely on SlotState*/
	bool bAvailable = true;
	
	//its index is assigned from WBP_InventoryGrid::GridSlots on creation), so don't set it again in AddItemWidgetsToIndices
	int32 GridSlotIndex;

	/*i assume:
	 - it is the index of the starting occupied WBP_Slot in case it is non-stackable item
	 - it is the index of the starting occupied WBP_Slot in case it is stackable item as well = meaning if the stackable ItemData already exist in the inventory, then assign AvailabilityInfo::SlotInfo::SlotArrayIndex in WBP_Grid::AddItemWidgetsToIndices()~>ForEachInGridDimensions could be incorrect if we understand it in this way. but let's see anyway
	 */
	int32 UpperLeftIndex{INDEX_NONE};
	
	/* I think WBP_GridSlot::StackCount is just "a part of Availability::TotalRoomToFill", the total of all occupied WBP_GridSlots 's StackCount will equal too Availability::TotalRoomToFill, but let's see anyway*/
	int32 StackCount = 0;
	
	/*each occupied WBP_GridSlot should be aware of the ItemData occupying it
	 *this also helps to know if an item is already here, but ::SlotState also help to know it as well lol
	 *a specific ItemData can have many WBP_SlottedItem instances span cross many slots (as the StackCount keep increase) or a single WBP_SlottedItem that spawn on a GridSize of many slots (non-stackable item) */
	UPROPERTY()
	TWeakObjectPtr<UItemData> OwningItemData;
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_GridSlot;

	//4 EditAnywhere brushes for 4 states:
	UPROPERTY(EditAnywhere, Category="Inventory")
	FSlateBrush UnoccupiedBrush;
	UPROPERTY(EditAnywhere, Category="Inventory")
	FSlateBrush OccupiedBrush;
	UPROPERTY(EditAnywhere, Category="Inventory")
	FSlateBrush SelectedBrush;
	UPROPERTY(EditAnywhere, Category="Inventory")
	FSlateBrush GrayedOutBrush;
};
