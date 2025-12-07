// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_Inv_HUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void ShowPickupMessage(FText PickupMessage);
	UFUNCTION(BlueprintImplementableEvent)
	void HidePickupMessage();
};
