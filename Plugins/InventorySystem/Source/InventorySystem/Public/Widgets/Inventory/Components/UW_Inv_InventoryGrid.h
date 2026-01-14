// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActorComponent/Inv_ItemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Types/InventoryTypes.h"
#include "UW_Inv_InventoryGrid.generated.h"

class UUW_Inv_ItemPopup;
enum class ESlotState : uint8;
class UUW_Inv_HoverItem;
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
	
	//need I or should I use UPROPERTY() on TWeakObjectPtr? Stephen don't use it, meaning it is not needed (it knows how to track to know when it is valid or not).Also adding UPROPERTY() didn't cause any compile error, "Object" suggest it is for "UObject" (unlike TWeakPtr for generic type) and also suggest adding  UPROPERTY() is fine (again it is not needed even if it is allowed or not)
	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	//this canvas belong to WBP_Inventory_Spacial containing this WBP_InventoryGrid
	UPROPERTY()
	TWeakObjectPtr<UCanvasPanel> OuterCanvas;

	//only this overload needed to be access GLOBALLY
	FInventoryAvailabilityInfo GetAvailabilityInfoForItem(UInv_ItemComponent* ItemComponent);
	//these 2 can be private
	FInventoryAvailabilityInfo GetAvailabilityInfoForItem(UItemData* InItemData);
	//this one is re-used by the 2 above
	FInventoryAvailabilityInfo GetAvailabilityInfoForItem(const FItemManifest& InItemManifest);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//this return a position of InWidget (its top-left) relative to Viewport origin (Viewport is parent) in Local Space (before DPI, hence TileSize or Geometry::GetLocalSize() is appropriate to be used):
	FVector2D GetWidgetPositionInViewport(UWidget* InWidget);

	FIntPoint CalculateDropNormalizedPosition(const FIntPoint& GridDimensions);

	FSpaceQueryResult GetSpaceQueryResult(int32 InDropIndex, FIntPoint& GridDimensions);
	void UpdateSpaceQueryResult();
	bool HasRoomForGridSizeAtThisSlot(const FIntPoint& GridDimensions, UUW_Inv_InventoryGridSlot* StartGridSlot);
	void UpdateTileParameters(const FVector2D& CanvasPositionInViewPort, const FVector2D& MousePositionInViewPort);
	FIntPoint CalculateNormalizedPositionOfHoveredGridSlot(const FVector2D& CanvasPositionInViewPort, const FVector2D& MousePositionInViewPort);
	ETileQuadrant CalculateTileQuadrant(const FVector2D& CanvasPositionInViewPort, const FVector2D& MousePositionInViewPort);

		
	void ShowVisibleCursorWidget();
	void ShowHiddenCursorWidget(); //sounds weird, but yes, Hidden widget will have some opacity (not completely hide)

	void DropHoverItem();
protected:
	UFUNCTION()
	void OnStacksAddedCallback(const FInventoryAvailabilityInfo& AvailabilityInfo);
	bool CreateHoverItemAndRemoveClickedSlottedItem(int32 ClickedGridIndex, UUW_Inv_InventoryGridSlot* ClickedGridSlot,
	                                                UUW_Inv_SlottedItem* ClickedSlottedItem,
	                                                UItemData* ClickedItemData);
	void CreateHoverItem(UUW_Inv_InventoryGridSlot* ClickedGridSlot,
	                     UItemData* ClickedItemData, int32 StackOverride = -1);
	void RemoveClickedSlottedItem(int32 ClickedGridIndex, UUW_Inv_InventoryGridSlot* ClickedGridSlot,
	                              UUW_Inv_SlottedItem* ClickedSlottedItem, FIntPoint GridDimensions);
	virtual void NativeOnInitialized() override;
	bool DoesItemMatchGridCategory(UItemData* ItemData);
	void ConstructGridSlots();

	void AddItemWidgetsToIndices(const FInventoryAvailabilityInfo& AvailabilityInfo, UItemData* ItemData);
	
	void AddItemWidgetToIndexFromSlotInfo(const FInventorySlotInfo& SlotInfo,
										  const FInventoryAvailabilityInfo& AvailabilityInfo, UItemData* ItemData);
	
	UFUNCTION()
	void OnItemAddedCallback(UItemData* ItemData);
	UFUNCTION()
	void OnSlottedItemClicked(int32 ClickedUpperLeftIndex, const FPointerEvent& MouseEvent);
		
	void CreateItemPopupWidget(const int32& OwningIndex);
	
	UFUNCTION()
	void OnGridSlotHovered(const int32& AffectedGridSlot, const FPointerEvent& PointerEvent);
	UFUNCTION()
	void OnGridSlotUnhovered(const int32& AffectedGridSlot, const FPointerEvent& PointerEvent);
	UFUNCTION()
	void OnGridSlotClicked(const int32& AffectedGridSlot, const FPointerEvent& PointerEvent);
	bool IsTheSameStackableItemAsHoverItem(UItemData* ClickedItemData);

	UFUNCTION()
	void OnSplitButtonClicked(int32 OwningIndex, int32 SplitAmount);
	UFUNCTION()
	void OnConsumeButtonClicked(int32 OwningIndex);
	UFUNCTION()
	void OnDropButtonClicked(int32 OwningIndex);
	void ClearHoverItem();


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
	UPROPERTY(Transient)
	TArray<UUW_Inv_InventoryGridSlot*> GridSlots;
	
	//we spawn as many items in our inventory (accumulating over time as we pick items up in game) = need an array to keep track of them
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUW_Inv_SlottedItem> SlottedItem_Class;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel_GridSlots;

	//we will spawn when LClick on WBP_SlottedItem and will be destroyed/invalidated when LClick is released
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUW_Inv_HoverItem> HoverItem_Class;
public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UUW_Inv_HoverItem> WBP_HoverItem;
protected:
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUW_Inv_ItemPopup> ItemPopup_Class;

	UPROPERTY()
	TObjectPtr<UUW_Inv_ItemPopup> WBP_ItemPopup = nullptr;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FVector2D ItemPopupOffset = {-10.f,-10.f};
	
	/*why dont' create TArray<WBP_SlottedItem> but TMap<int32, WBP_SlottedItem>?
	- The key "int32" HERE will be in fact the SlotInfo::SlotArrayIndex / WBP_Item::GridIndex (just shadow from SlotInfo) / WBP_Inventory::InventoryGrid::WBP_GridSlot::SlotArrayIndex (cosmetic background, not involve in what we go next)
	- Hence IF we create TArray<WBP_SlottedItem>, this index here won't match WBP_SlottedItem::GridIndex+ at all.
	- also note that on UItemData instance can have many of WBP_Item with same ::Values (for stackable items) as they come from the same Availability::SlotInfos
	Stephen decide TObjectPtr, instead of TWeakObjectPtr, simply because this is where it is first DYNAMICALLY created*/
	UPROPERTY()
	TMap<int32, TObjectPtr<UUW_Inv_SlottedItem>> SlottedItemMap;

//OPTIONAL: mouse cursor widgets for different WBP_Inventory::WBP_Grid1,2,3
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUserWidget> Cursor_Visible_Class; //each WBP_Grid_i will select its own unique WBP_Visible if you want to
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUserWidget> Cursor_Hidden_Class; //all WBP_Grid1,2,3 will select to the same WBP_Hidden
	UPROPERTY()
	TObjectPtr<UUserWidget> WBP_Cursor_Visible;
	UPROPERTY()
	TObjectPtr<UUserWidget> WBP_Cursor_Hidden;

	//if you don't want to construct something at first place, but still want to cache it
	//, then use the GetterGenerator (that generate one and cache it if it is not valid at first time, next time simply return the cached pointer)
	UUserWidget* GetVisibleCursorWidget();
	UUserWidget* GetHiddenCursorWidget();
	
//will be updated per frame:
	FTileParameters TileParameters {};
	FTileParameters LastTileParameters {};
	int32 DropIndex { INDEX_NONE };
	FSpaceQueryResult SpaceQueryResult {};

	/*DO NOT confuse:
	MouseInCanvas = literally mean whether it is in canvas
	MouseExitedCanvas() is only return true if "bLastMouseInCanvas=true && bMouseInCanvas=false", meaning it will be true for a single frame when you move mouse in and out!
	 */
	bool bMouseInCanvas;       //Ved = already
	bool bLastMouseInCanvas;
	bool MouseExitedCanvas(const FVector2D& CanvasPositionInViewport, const FVector2D& MousePositionInViewPort);

	//we don't have "Current", we simply always know them (it is HighlightIndex = DropIndex if it is valid). Anyway you may or may not want to name it "LastDropIndex" (I don't care)
	int32 LastHighlightedIndex = INDEX_NONE; 
	FIntPoint LastHighlightedGridDimensions{-1,-1};
	
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

	//IMPORTANT: current we SpaceQuery.bHasRoom=false in case "WBP_HoverItem" overlap with "any preoccupied WBP_SlottedItem", meaning it skip highlight and unhighlight (in the else if), and the UnHighlightSlots will help to
	//UnHighlightSlots(LastValidDropIndex) is always called (because it is also called inside HighlightSlots before proceed)
		//[HighlightCurrentDropSlots]And[UnhighlightLastDropSlotOrResetFakeOne] --HighlightSlotsAndResetTheLastOnes
	void HighlightSlots(int32 StartIndex, const FIntPoint& InGridDimensions);
		//[UnhighlightLastDropSlot]Or[ResetFakeOne],  PreoccupiedGridSlots will be set to occupied regardless --UnhighlightSlotsAndResetTheLastOnes
	void UnHighlightSlots(int StartIndex, const FIntPoint& InGridDimensions);

	//do not confuse f(Unoccupied/occupied) with HighlightSlots and UnHighlightSlots above (do more, hence the name is misleading!)
	void SetSlotStateForAllSlotsInGridSize(const int32& StartIndex, const FIntPoint& InGridDimensions, const ESlotState& InSlotState);
};
