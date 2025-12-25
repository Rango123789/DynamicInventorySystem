// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UW_Inv_InventoryBase.h"
#include "UW_Inv_Inventory_Spacial.generated.h"

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

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Equippable;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Consumable;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Craftable;

	//just for the sake of looping through them instead of using switch:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUW_Inv_InventoryGrid>> InventoryGridArray;
private:
	
};
