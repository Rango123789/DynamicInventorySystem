// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_Inv_HoverItem.generated.h"

class UItemData;
class UImage;
class UTextBlock;
/**
 * go and copy all members of sub widgets of WBP_SlottedItem lol, including those methods for sub widgets
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_HoverItem : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetImageIcon(UTexture2D* InIcon) const;
	void SetImageIcon(const FSlateBrush& InBrush) const;
	void UpdateStackCount(const int32& InStackCount);
	
	//just mean "the GridSlotIndex of the starting Slot" containing "the item "in the current Grid
	//you may want to rename it to "OldGridIndex" / PreviousGridIndex / BackupGridIndex", but I keep the name for the sake of copying code lol
	int32 GridIndex{INDEX_NONE};
	//to be assigned from ItemData:::Fragment_Grid::GridDimensions
	FIntPoint GridDimensions;
	//to be assigned from SlotAvailability=f(ItemData)  (surely ItemData::ItemManifest::ItemCategory at least relevant to this) 
	bool bStackable = false;
	//this is new (where WBP_SlottedItem didn't need it so far, since WBP_::Grid::GridSlots[WBP_SlottedItem::Index] will help to get it immediately). Perhaps because for when we want to merge StackCount with other WBP_SlottedItem of the same type.
	int32 StackCount = 0;

	TWeakObjectPtr<UItemData> OwningItemData; 
protected:
	//bind widgets
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_StackCount;
public:	

};
