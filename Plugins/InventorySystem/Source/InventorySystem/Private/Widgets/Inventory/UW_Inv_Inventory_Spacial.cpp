// Fill out your copyright notice in the Description page of Project Settings.
#include "Widgets/Inventory/UW_Inv_Inventory_Spacial.h"
#include "ActorComponent/Inv_ItemComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
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

	for (auto& Grid : InventoryGridArray)
	{
		Grid->OuterCanvas = CanvasPanel;
	}
	
	ShowEquippableTab();
}

//when HoverItem is valid, we drop it no matter key we press (optionally check ActiveGrid->bIsInVanvas, but in fact no need because GridSlots cover all part of it)
FReply UUW_Inv_Inventory_Spacial::NativeOnMouseButtonDown(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	ActiveInventoryGrid->DropHoverItem();
	return FReply::Handled();
}

//you can create SetActiveGrid(ButtonToDisable, GridToBeActive) , so that you don't repeat yourself
void UUW_Inv_Inventory_Spacial::ShowEquippableTab()
{
	DisableButton(Button_Equippable);
	WidgetSwitcher->SetActiveWidget(InventoryGrid_Equippable);
	InventoryGrid_Equippable->ShowVisibleCursorWidget();
	ActiveInventoryGrid = InventoryGrid_Equippable;
}

void UUW_Inv_Inventory_Spacial::ShowConsumableTab()
{
	SetActiveInventoryGrid(InventoryGrid_Consumable, Button_Consumable);
}

void UUW_Inv_Inventory_Spacial::ShowCraftableTab()
{
	// SetActiveInventoryGrid(InventoryGrid_Craftable, Button_Craftable);
}

void UUW_Inv_Inventory_Spacial::SetActiveInventoryGrid(UUW_Inv_InventoryGrid* GridToBeActive, UButton* ButtonToDisable)
{
	//you invoke this function from a different WBP_Grid, but because PC is only one, PC::SetMouseCursorWidget() will be overriden by the new one from the new active grid anyway, so this is totally REDUNDANT so far (unless you want to do other things along in it which we didn't)
	//the "if" is because the very first time it isn't valid lol, so better off remove this whole line if you want
	if (ActiveInventoryGrid.IsValid()) ActiveInventoryGrid->ShowHiddenCursorWidget();

	//now it is new
	DisableButton(ButtonToDisable);
	WidgetSwitcher->SetActiveWidget(GridToBeActive);
	GridToBeActive->ShowVisibleCursorWidget();
	ActiveInventoryGrid = GridToBeActive;
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