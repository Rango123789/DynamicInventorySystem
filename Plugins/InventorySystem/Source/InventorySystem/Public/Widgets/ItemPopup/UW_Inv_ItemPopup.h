// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_Inv_ItemPopup.generated.h"

//No point in name FOnItemPopupDrop, because we only use this delegate here and Object::On___ is enough to clarify, it is similar to UButton::OnClicked or UCommonButton::OnClicked!
//non-multicast mean only one callback is bound at a time at most
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDropDelegate, int32 , OwningIndex);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnConsumeDelegate, int32, OwningIndex);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnSplitDelegate, int32, OwningIndex, int32, SplitAmount);

class USizeBox;
class UTextBlock;
class USlider;
class UButton;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_ItemPopup : public UUserWidget
{
	GENERATED_BODY()
public:
	FVector2D GetSizeBox() const;
	
	FOnDropDelegate OnDropDelegate;
	FOnConsumeDelegate OnConsumeDelegate;
	FOnSplitDelegate OnSplitDelegate;

	int32 OwningIndex = INDEX_NONE;
protected:
	virtual void NativeOnInitialized() override;
	//you can decide to add "X" button" to close it if you want, but this is the quickest way to close it! hell yeah I see it a lot in games but don't know how to do it!
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	
	UFUNCTION()
	void OnSplitButtonClickedCallback();
	UFUNCTION()
	void OnDropButtonClickedCallback();
	UFUNCTION()
	void OnConsumeButtonClickedCallback();
	UFUNCTION()
	void OnSplitSliderValueChanged(float Value);

	
	//for stackable item only
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SplitStacks; 
		UPROPERTY(meta = (BindWidget))
		TObjectPtr<USlider> Slider_SplitStacks;
		UPROPERTY(meta = (BindWidget))
		TObjectPtr<UTextBlock> TextBlock_SplitAmount;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Drop;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Consume;

	//you will see why we need this:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox;

public:
	//no need for "Drop", it always exists:
	void CollapseSplit() const;
	void CollapseConsume() const;

	/*the value it accepts will be still "float" as it is (it is underlying type you can't help it), only the TextBlock will round it for showing purpose:
	Do not confuse:
	- MaxStackCount  = fixed
	- MaxSplitAmount = WBP_SlottedItem::StackCount - 1 = its subject to change contextually  
	*/
	void SetSliderValueAndParams(float InSplitAmount, float InMaxSplitAmount) const;
};
