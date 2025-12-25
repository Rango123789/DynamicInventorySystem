// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/HUD/UW_Inv_HUDWidget.h"

#include "InventorySystem.h"
#include "ActorComponent/Inv_InventoryComponent.h"
#include "Utils/Inv_BPFunctionLibrary.h"
#include "Widgets/HUD/Components/UW_Inv_InfoMessage.h"

void UUW_Inv_HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

//access PC and PC::InventoryComp::NoRoomDelegate

	UInv_InventoryComponent* InventoryComponent = UInv_BPFunctionLibrary::GetInventoryComponentFromPC(GetOwningPlayer());
	if (IsValid(InventoryComponent))
	{
		DebugHelpers::Print("Inventory Component valid");
		InventoryComponent->NoRoomDelegate.AddDynamic(this, &ThisClass::NoRoomCallback);
	}
		
}

void UUW_Inv_HUDWidget::NoRoomCallback()
{
	WBP_InfoMessage->SetMessageAndSetTimer(FText::FromString("Not Enough Room In Inventory"));
}

