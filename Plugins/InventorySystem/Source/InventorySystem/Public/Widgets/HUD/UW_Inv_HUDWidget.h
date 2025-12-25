// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_Inv_HUDWidget.generated.h"

class UUW_Inv_InfoMessage;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void ShowPickupMessage(const FText& PickupMessage);
	UFUNCTION(BlueprintImplementableEvent)
	void HidePickupMessage();
	
	UFUNCTION()
	void NoRoomCallback();

protected:
	virtual void NativeOnInitialized() override;
	
	
	//WBP_PickupMessage directly create from BP, hence we dont have BindWidget here. But UUW_InfoMessage does, hence:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUW_Inv_InfoMessage> WBP_InfoMessage;
	
};
