// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Components/UW_Inv_InventoryGridSlot.h"

#include "Components/Image.h"

void UUW_Inv_InventoryGridSlot::SetSlotStateAndBrush(ESlotState InSlotState) const
{
	switch (InSlotState)
	{
	case ESlotState::UOccupied:
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
