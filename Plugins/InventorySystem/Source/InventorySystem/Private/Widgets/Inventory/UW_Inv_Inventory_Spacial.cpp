// Fill out your copyright notice in the Description page of Project Settings.
#include "Widgets/Inventory/UW_Inv_Inventory_Spacial.h"
#include "ActorComponent/Inv_ItemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Utils/Inv_BPFunctionLibrary.h"
#include "Widgets/Inventory/Components/UW_Inv_InventoryGrid.h"
#include "Widgets/Inventory/Components/HoverItem/UW_Inv_HoverItem.h"
#include "Widgets/ItemDescription/UW_Inv_ItemDescription.h"

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

UUW_Inv_ItemDescription* UUW_Inv_Inventory_Spacial::GetItemDescriptionWidget()
{
	if (IsValid(WBP_ItemDescription) == false)
	{
		WBP_ItemDescription = CreateWidget<UUW_Inv_ItemDescription>(GetOwningPlayer(), ItemDescription_Class );

		UCanvasPanelSlot* CanvasSlot = CanvasPanel->AddChildToCanvas(WBP_ItemDescription);

	//you can set its Size here if you want (because it should remain constant):
		CanvasSlot->SetAutoSize(true); //this also work!
		CanvasSlot->SetSize(WBP_ItemDescription->GetSize()); //hence this could be redundant!
		
	//but you need to set its Position in Tick, because it is following the mouse lol.
	//Here I just give it the starting off position. anyway this is not clamped so it causes artifact lol. so let's comment it out
		//CanvasSlot->SetPosition(UWidgetLayoutLibrary::GetMousePositionOnViewport(this));
	}

	return WBP_ItemDescription;
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

void UUW_Inv_Inventory_Spacial::OnItemHovered(UItemData* HoveredItemData)
{
	UUW_Inv_ItemDescription* ItemDescription = GetItemDescriptionWidget();
	ItemDescription->SetVisibility(ESlateVisibility::Collapsed);

	//HERE WE GO: (now the "ItemData" param come in handy)
	HoveredItemData->GetItemManifest().AssimilateWidgetFragmentsToCompositeWidget(ItemDescription); 

	//better clear it in case Players try to spam hover before the timer reach lol
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

	/*member TimerHandle - the later override the first as HostingFunction repeat before timer not reach = callback trigger ONCE when...(if the same member TimerHanfle responsible for different callbacks kicks in at the same time - only the last one will override and execute)
	 local TimerHandle - each HostingFunction call will have a separate TimerCallback trigger = callback trigger MANY TIMES*/
	GetWorld()->GetTimerManager().SetTimer(
	TimerHandle,
	[this]()
		{
			//you better don't copy by reference for TimerCallback, in case you try to use ItemDescription above.
			GetItemDescriptionWidget()->SetVisibility(ESlateVisibility::HitTestInvisible);	
		},
		0.5f, false );
}

void UUW_Inv_Inventory_Spacial::OnItemUnhovered()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	if (IsValid(WBP_ItemDescription))
	{
		WBP_ItemDescription->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UUW_Inv_Inventory_Spacial::HasHoverItemInAction()
{
	for (auto InventoryGrid : InventoryGridArray)
	{
		if (IsValid(InventoryGrid->WBP_HoverItem)) return true;
	}
	return false;
}

void UUW_Inv_Inventory_Spacial::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//set Position of WBP_ItemDescription following the mouse if it is in action:
	if (IsValid(WBP_ItemDescription))
	{
		
		UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(WBP_ItemDescription);
		if (IsValid(CanvasSlot) == false) return;
		
		//you must set it size before "WBP_ItemDescription->GetCachedGeometry().GetLocalSize()" (which I did in GetItemDescription), but anyway
			//CanvasSlot->SetSize(WBP_ItemDescription->GetSize()); 
		
		FVector2D CanvasSize = CanvasPanel->GetCachedGeometry().GetLocalSize();         //OR use the static helper
		FVector2D WidgetSize    = WBP_ItemDescription->GetSize(); //OR WBP_ItemDescription->GetCachedGeometry().GetLocalSize()
		FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
		FVector2D ClampedMousePosition = UInv_BPFunctionLibrary::GetClampedMousePosition(MousePosition, CanvasSize, WidgetSize);

		CanvasSlot->SetPosition(ClampedMousePosition);
	}
}
