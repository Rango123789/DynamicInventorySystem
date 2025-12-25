// Fill out your copyright notice in the Description page of Project Settings.
#include "Widgets/Inventory/UW_Inv_Inventory_Spacial.h"

#include "ActorComponent/Inv_ItemComponent.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Widgets/Inventory/Components/UW_Inv_InventoryGrid.h"

void UUW_Inv_Inventory_Spacial::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	InventoryGridArray.Empty();
	InventoryGridArray.Add(InventoryGrid_Equippable);
	InventoryGridArray.Add(InventoryGrid_Consumable);
	InventoryGridArray.Add(InventoryGrid_Craftable);
	
	Button_Equippable->OnClicked.AddDynamic(this, &ThisClass::ShowEquippableTab);
	Button_Consumable->OnClicked.AddDynamic(this, &ThisClass::ShowConsumableTab);
	Button_Craftable->OnClicked.AddDynamic(this, &ThisClass::ShowCraftableTab);

	ShowEquippableTab();
}

void UUW_Inv_Inventory_Spacial::ShowEquippableTab()
{
	DisableButton(Button_Equippable);
	WidgetSwitcher->SetActiveWidget(InventoryGrid_Equippable);
}

void UUW_Inv_Inventory_Spacial::ShowConsumableTab()
{
	DisableButton(Button_Consumable);
	WidgetSwitcher->SetActiveWidget(InventoryGrid_Consumable);
}

void UUW_Inv_Inventory_Spacial::ShowCraftableTab()
{
	DisableButton(Button_Craftable);
	WidgetSwitcher->SetActiveWidget(InventoryGrid_Craftable);
}

void UUW_Inv_Inventory_Spacial::DisableButton(UButton* InButton) const
{
	Button_Equippable->SetIsEnabled(true);
	Button_Consumable->SetIsEnabled(true);
	Button_Craftable->SetIsEnabled(true);
	
	InButton->SetIsEnabled(false);
}

//to be precise "GetCurrentAvailability"
FInventoryAvailabilityInfo UUW_Inv_Inventory_Spacial::GetAvailabilityInfoForItem(
	UInv_ItemComponent* ItemComponent)
{
	if (IsValid(ItemComponent) == false) return FInventoryAvailabilityInfo();
	
	for (auto& InventoryGrid : InventoryGridArray)
	{
		if (InventoryGrid->ItemCategory == ItemComponent->SourceItemManifest.ItemCategory)
		{
			return InventoryGrid->GetAvailabilityInfoForItem(ItemComponent);
		}
	}

	return FInventoryAvailabilityInfo();
}