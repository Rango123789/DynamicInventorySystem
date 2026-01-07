// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/ItemPopup/UW_Inv_ItemPopup.h"

#include "InventorySystem.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"


void UUW_Inv_ItemPopup::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_SplitStacks->OnClicked.AddDynamic(this, &ThisClass::OnSplitButtonClicked);
	Slider_SplitStacks->OnValueChanged.AddDynamic(this, &ThisClass::OnSplitSliderValueChanged);
	Button_Drop->OnClicked.AddDynamic(this, &ThisClass::OnDropButtonClicked);
	Button_Consume->OnClicked.AddDynamic(this, &ThisClass::OnConsumeButtonClicked);
}

void UUW_Inv_ItemPopup::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	RemoveFromParent();
}

void UUW_Inv_ItemPopup::CollapseSplit() const
{
	Button_SplitStacks->SetVisibility(ESlateVisibility::Collapsed);
	Slider_SplitStacks->SetVisibility(ESlateVisibility::Collapsed);
	TextBlock_SplitAmount->SetVisibility(ESlateVisibility::Collapsed);
}

void UUW_Inv_ItemPopup::CollapseConsume() const
{
	Button_Consume->SetVisibility(ESlateVisibility::Collapsed);
}

void UUW_Inv_ItemPopup::OnSplitButtonClicked()
{
	float FloatValue = Slider_SplitStacks->GetValue();
	int32 IntValue = FMath::Floor(FloatValue); //Floor because we will set Min = 1, Max = passed-in Max (rather then RoundToInt)
	
	if (OnSplit.ExecuteIfBound(OwningIndex, IntValue))
	{
		RemoveFromParent();
	}
}

void UUW_Inv_ItemPopup::OnDropButtonClicked()
{
	if (OnDrop.ExecuteIfBound(OwningIndex))
	{
		RemoveFromParent();	
	}
}

void UUW_Inv_ItemPopup::OnConsumeButtonClicked()
{
	if (OnConsume.ExecuteIfBound(OwningIndex))
	{
		RemoveFromParent();	
	}
}

//We don't remove from parent when change the value via slider, we work more on this later
void UUW_Inv_ItemPopup::OnSplitSliderValueChanged(float Value) 
{		
	TextBlock_SplitAmount->SetText(FText::FromString(FString::FromInt(Value)));
}

void UUW_Inv_ItemPopup::SetSliderValueAndParams(float InValue, float InMax) const
{
	Slider_SplitStacks->SetValue(InValue);
	Slider_SplitStacks->SetMinValue(1.0f);
	Slider_SplitStacks->SetMaxValue(InMax);
	
	TextBlock_SplitAmount->SetText(FText::FromString(FString::FromInt(InValue)));
}



FVector2D UUW_Inv_ItemPopup::GetSizeBox() const
{
	DebugHelpers::Print("DesiredSize: " + GetDesiredSize().ToString());
	DebugHelpers::Print("Width, Height Overrides: " + FVector2D(SizeBox->GetWidthOverride(), SizeBox->GetHeightOverride()).ToString());
	return { SizeBox->GetWidthOverride(), SizeBox->GetHeightOverride()};
}