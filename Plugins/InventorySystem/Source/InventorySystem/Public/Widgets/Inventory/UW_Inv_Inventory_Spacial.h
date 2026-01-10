// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UW_Inv_InventoryBase.h"
#include "UW_Inv_Inventory_Spacial.generated.h"

class UCanvasPanel;
class UButton;
class UUW_Inv_InventoryGrid;
class UWidgetSwitcher;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_Inventory_Spacial : public UUW_Inv_InventoryBase
{
	GENERATED_BODY()
public:
	virtual FInventoryAvailabilityInfo GetAvailabilityInfoForItem(UInv_ItemComponent* ItemComponent) override;
	
protected:
	
	virtual void NativeOnInitialized() override;
	//currently its subs WBP_SlottedItem, WBP_GridSlot, WBP_ItemPopup (added as child of either canvases that is part of this WBP_Host tree) also override this function and return Handled(), hence this parent version won't trigger in case you click on them:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	UFUNCTION()
	void ShowEquippableTab();
	UFUNCTION()
	void ShowConsumableTab();
	UFUNCTION()
	void ShowCraftableTab();
	UFUNCTION()
	void DisableButton(UButton* InButton) const;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher;
	
		UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
		TObjectPtr<UUW_Inv_InventoryGrid> InventoryGrid_Equippable;
	
		UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
		TObjectPtr<UUW_Inv_InventoryGrid> InventoryGrid_Consumable;
	
		UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
		TObjectPtr<UUW_Inv_InventoryGrid> InventoryGrid_Craftable;

	//just for the sake of looping through them instead of using switch:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUW_Inv_InventoryGrid>> InventoryGridArray;
	//pretty much optional:
	UPROPERTY(Transient)
	TWeakObjectPtr<UUW_Inv_InventoryGrid> ActiveInventoryGrid; //I don't want to increase reference count
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Equippable;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Consumable;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Craftable;

	//OPTIONAL:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;
	
private:
	void SetActiveInventoryGrid(UUW_Inv_InventoryGrid* GridToBeActive, UButton* ButtonToDisable);
};
