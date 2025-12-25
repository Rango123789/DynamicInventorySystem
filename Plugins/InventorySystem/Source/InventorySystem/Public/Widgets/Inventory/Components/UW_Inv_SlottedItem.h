// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_Inv_SlottedItem.generated.h"

class UTextBlock;
class UItemData;
class UImage;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_SlottedItem : public UUserWidget
{
	GENERATED_BODY()

public:	
	void SetImageIcon(UTexture2D* InIcon) const;
	void SetImageIcon(const FSlateBrush& InBrush) const;
	
	int32 GetGridIndex() const { return GridIndex; }
	void SetGridIndex(int32 InGridIndex) { GridIndex = InGridIndex; }
	FIntPoint GetGridDimensions() const { return GridDimensions; };
	FIntPoint SetGridDimensions(FIntPoint InGridDimensions){ return GridDimensions = InGridDimensions; };
	bool GetIsStackable() const {return bStackable; }
	void SetIsStackable(bool InIsStackable){bStackable = InIsStackable;}

	void UpdateStackCount(const int32& InStackCount) const;
	
//protected: i decide to remove these "protected" formality to see the relation better (though I still keep those defined getter/setter anyway)

//important info to keep track of each WBP_Item instance
	//just mean "the GridSlotIndex of the starting Slot" containing "the item "in the current Grid
	int32 GridIndex{INDEX_NONE};
	//to be assigned from ItemData:::Fragment_Grid::GridDimensions
	FIntPoint GridDimensions;
	//to be assigned from SlotAvailability=f(ItemData)  (surely ItemData::ItemManifest::ItemCategory at least relevant to this) 
	bool bStackable = false;

//each WBP_Item can have a weak pointer to its associate UItemData (this practice is exactly like "ListData" and "WBP_ListEntry" , where ListData shouldn't care about "wBP_ListEntry", but WBP_ListEntry better off hold a reference to its associate ListData)	
	TWeakObjectPtr<UItemData> OwningItemData; //i use the terms "Owning" like Vince does in FrontendUI

protected:
	//bind widgets
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_StackCount;
};
