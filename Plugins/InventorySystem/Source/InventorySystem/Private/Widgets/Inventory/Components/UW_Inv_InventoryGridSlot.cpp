// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Components/UW_Inv_InventoryGridSlot.h"

#include "Components/Image.h"

void UUW_Inv_InventoryGridSlot::SetSlotStateAndBrush(ESlotState InSlotState) const
{
	switch (InSlotState)
	{
	case ESlotState::Unoccupied:
		Image_GridSlot->SetBrush(UnoccupiedBrush);
		break;
	case ESlotState::Occupied:
		Image_GridSlot->SetBrush(OccupiedBrush);
		break;
	case ESlotState::Selected:
		Image_GridSlot->SetBrush(SelectedBrush);
		break;
	case ESlotState::GrayedOut:
		Image_GridSlot->SetBrush(GrayedOutBrush);
		break;
	}
}

void UUW_Inv_InventoryGridSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent); //this help to call the BP version (not used in this course)

	OnGridSlotHovered.Broadcast(GridSlotIndex, InMouseEvent);
}

void UUW_Inv_InventoryGridSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent); //this help to call the BP version

	OnGridSlotUnhovered.Broadcast(GridSlotIndex, InMouseEvent);
}

FReply UUW_Inv_InventoryGridSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	OnGridSlotClicked.Broadcast(GridSlotIndex, InMouseEvent);

	//universal rule: a function return FReply will require you to return either ::Handled or Unhandled, not Super::
	return FReply::Handled();
}
