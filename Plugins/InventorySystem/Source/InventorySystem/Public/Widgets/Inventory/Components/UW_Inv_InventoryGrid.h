// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActorComponent/Inv_ItemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Types/InventoryTypes.h"
#include "UW_Inv_InventoryGrid.generated.h"

class UUW_Inv_SlottedItem;
class UInv_InventoryComponent;
class UCanvasPanel;
class UUW_Inv_InventoryGridSlot;

/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	EItemCategory ItemCategory;

	//only this overload needed to be access GLOBALLY
	FInventoryAvailabilityInfo GetAvailabilityInfoForItem(UInv_ItemComponent* ItemComponent);
	//these 2 can be private
	FInventoryAvailabilityInfo GetAvailabilityInfoForItem(UItemData* InItemData);
	//this one is re-used by the 2 above
	FInventoryAvailabilityInfo GetAvailabilityInfoForItem(const FItemManifest& InItemManifest);

protected:
	UFUNCTION()
	void OnStacksAddedCallback(const FInventoryAvailabilityInfo& AvailabilityInfo);
	virtual void NativeOnInitialized() override;
	bool DoesItemMatchGridCategory(UItemData* ItemData);
	void ConstructGridSlots();

	void AddItemWidgetsToIndices(const FInventoryAvailabilityInfo& AvailabilityInfo, UItemData* ItemData);
	
	UFUNCTION()
	void OnItemAddedCallback(UItemData* ItemData);
	void AddItemWidgetToIndexFromSlotInfo(const FInventorySlotInfo& SlotInfo,
	                                         const FInventoryAvailabilityInfo& AvailabilityInfo, UItemData* ItemData);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 columns = 8;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 rows = 5;

	//From WBP_Grid::Canvas , currently we only have GridSlotSize, i.e GridSlot will be next to each other (no padding between, hence the padding if needed is handled by the local image itself, it is for cosmetic anyway)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	float GridSlotSize = 50.f;

	//we spawn full of them to cover the background
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUW_Inv_InventoryGridSlot> GridSlot_Class;	

	//we spawn as many items in our inventory (accumulating over time as we pick items up in game) = need an array to keep track of them
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUW_Inv_SlottedItem> SlottedItem_Class;
	
	UPROPERTY(Transient)
	TArray<UUW_Inv_InventoryGridSlot*> GridSlots;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel_GridSlots;	

	/*why dont' create TArray<WBP_SlottedItem> but TMap<int32, WBP_SlottedItem>?
	- The key "int32" HERE will be in fact the SlotInfo::SlotArrayIndex / WBP_Item::GridIndex (just shadow from SlotInfo) / WBP_Inventory::InventoryGrid::WBP_GridSlot::SlotArrayIndex (cosmetic background, not involve in what we go next)
	- Hence IF we create TArray<WBP_SlottedItem>, this index here won't match WBP_SlottedItem::GridIndex+ at all.
	- also note that on UItemData instance can have many of WBP_Item with same ::Values (for stackable items) as they come from the same Availability::SlotInfos
	Stephen decide TObjectPtr, instead of TWeakObjectPtr, simply because this is where it is first DYNAMICALLY created*/
	UPROPERTY()
	TMap<int32, TObjectPtr<UUW_Inv_SlottedItem>> SlottedItemMap;

	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
private:
//so far these sub helpers are used in this class itself:
	bool IsOutOfBounds(UUW_Inv_InventoryGridSlot* GridSlot, const FIntPoint& GridDimensions);
	bool HasRoomForGridSizeAtThisSlot(UUW_Inv_InventoryGridSlot* GridSlot, TSet<int32>& ClaimedSlotIndices, TSet<int32>& PotentialClaimedIndices, const FItemManifest& ItemManifest, int32 MaxStackSize);
	bool IsThisSubGridSlotQualified(UUW_Inv_InventoryGridSlot* GridSlot, UUW_Inv_InventoryGridSlot* SubGridSlot, TSet<int32>& ClaimedSlotIndices, TSet<int32>& PotentialClaimedIndices, const FItemManifest& ItemManifest, int32 MaxStackSize);
	
	bool IsIndexClaimed(const TSet<int32>& ClaimedIndices, int32 IndexToCheck);
	bool IsGridSlotPreoccupied(UUW_Inv_InventoryGridSlot* GridSlot);
	bool IsUpperLeftSlotOfThisSlot(UUW_Inv_InventoryGridSlot* ThisGridSlot, UUW_Inv_InventoryGridSlot* PotentialUpperLeftGridSlotToCheck);
	int32 GetStackCountFromAnySlotInGridSize(UUW_Inv_InventoryGridSlot* SubGridSlot);
	int32 CalculateAmountToFillForGridSlot(UUW_Inv_InventoryGridSlot*& GridSlot, bool bStackable, int32 MaxStackSize, int32 TotalRoomLeftToFill);
};
