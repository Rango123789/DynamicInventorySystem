// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_Inv_InfoMessage.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_InfoMessage : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	void SetMessageAndSetTimer(const FText& NewMessage);

protected:
	//totally optional, stephen make it to play WBPAnim::Opacity[0->1] and [1->0]. Ether of the case we want it to be "Not-hit testible" so that it won't affect our game (you can simply make WBP_HUDWidget "Not-hit testable: self and children" and the job done for all WBP_Subs lol)
	//I kind of line the name convention: XHide, XShow when it plays WBPAnim to Hide and Show
	UFUNCTION(BlueprintImplementableEvent)
	void MessageShow();
	UFUNCTION(BlueprintImplementableEvent)
	void MessageHide();

	bool bIsShown = false;

private:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_Message;
	
	//read "FTimerHandleUE5" to know the difference between make a local and member timer handle when we re-use the handle before time Timer reaches: 
	FTimerHandle TimerHandle;
	UPROPERTY(EditAnywhere)
	float ShowTime = 3.f;
};
