// Fill out your copyright notice in the Description page of Project Settings.
#include "Items/ItemFragment/ItemFragment.h"

#include "Components/TextBlock.h"
#include "Widgets/CompositePattern/UW_Inv_CompositeBase.h"
#include "Widgets/CompositePattern/Leaves/UW_Inv_Leaf_Image.h"
#include "Widgets/CompositePattern/Leaves/UW_Inv_Leaf_LabeledNumber.h"
#include "Widgets/CompositePattern/Leaves/UW_Inv_Leaf_Text.h"

/******Consumable fragments**********/
void FItemFragment_Consumable_Health::OnConsume(APlayerController* PC)
{
	// Get a stats component from the PC or the PC->GetPawn()
	// or get the Ability System Component and apply a Gameplay Effect
	// or call an interface function for Healing()

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Health Potion consumed! Healing by: %f"), Health));
}

void FItemFragment_Consumable_Mana::OnConsume(APlayerController* PC)
{
	// Replenish mana however you wish

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Mana Potion consumed! Mana replenished by: %f"), Mana));
}

/******End Consumable fragments**********/

/******Widget Fragments******/
//at this level it is Leaf already:
void FItemFragment_Widget::Assimilate(UUW_Inv_CompositeBase* InCompositeBase) const
{
	/*correct - it is now at leaf level already
	*/
	if (InCompositeBase->FragmentTag == FragmentTag)
	{
		InCompositeBase->Expand();	
	}

	/*overkill:
	InCompositeBase->ApplyFunction([this](UUW_Inv_CompositeBase* CompositeBaseWidget)
	{
		if (CompositeBaseWidget->FragmentTag == FragmentTag)
		{
			CompositeBaseWidget->Expand();	
		}
	});
	 */
}

void FItemFragment_Image::Assimilate(UUW_Inv_CompositeBase* InCompositeBase) const
{
	//this will expand if matches tag: 
	Super::Assimilate(InCompositeBase);

	//still we need to check matches tag again:
	if (InCompositeBase->FragmentTag != FragmentTag) return;

	//this is where we cast to the associate WBP_Leaf_X and set its ::SubWidgets and Values using info from this widget fragment itself: (what's sooner than the moment WBP_Leaf_X know it is just simulated?)
	UUW_Inv_Leaf_Image* WBP_Leaf_Image = Cast<UUW_Inv_Leaf_Image>(InCompositeBase);
	if (IsValid(WBP_Leaf_Image) == false) return;
	
	WBP_Leaf_Image->SetImageIcon(Icon);
	WBP_Leaf_Image->SetImageSize(IconSize);
	WBP_Leaf_Image->SetSizeBoxSize(IconSize); //either of them is redundant, but anyway
}

void FItemFragment_Widget_Text::Assimilate(UUW_Inv_CompositeBase* InCompositeBase) const
{
//same things:
	//this will expand if matches tag: 
	FItemFragment_Widget::Assimilate(InCompositeBase);

	//still we need to check matches tag again:
	if (InCompositeBase->FragmentTag != FragmentTag) return;

	//this is where we cast to the associate WBP_Leaf_X and set its ::SubWidgets and Values using info from this widget fragment itself: (what's sooner than the moment WBP_Leaf_X know it is just simulated?)
	UUW_Inv_Leaf_Text* WBP_Leaf_Text = Cast<UUW_Inv_Leaf_Text>(InCompositeBase);
	if (IsValid(WBP_Leaf_Text) == false) return;

//similar but specifically different:
	WBP_Leaf_Text->TextBlock->SetText(Text);
	// WBP_Leaf_Text->TextBlock->SetFont(FontInfo);
}

void FItemFragment_Widget_LabeledNumber::Assimilate(UUW_Inv_CompositeBase* InCompositeBase) const
{
	//same things:
	//this will expand if matches tag: 
	FItemFragment_Widget::Assimilate(InCompositeBase);

	//still we need to check matches tag again:
	if (InCompositeBase->FragmentTag != FragmentTag) return;

	//this is where we cast to the associate WBP_Leaf_X and set its ::SubWidgets and Values using info from this widget fragment itself: (what's sooner than the moment WBP_Leaf_X know it is just simulated?)
	UUW_Inv_Leaf_LabeledNumber* WBP_Leaf_LabeledNumber = Cast<UUW_Inv_Leaf_LabeledNumber>(InCompositeBase);
	if (IsValid(WBP_Leaf_LabeledNumber) == false) return;

	//similar but specifically different:
	WBP_Leaf_LabeledNumber->SetLabelText(LabelText, bCollapseLabel);

	/*
	1. If you convert to FText, simply use this:
	FText::AsNumber(float/int32 InValue, FNumberFormattingOptions)

	FNumberFormattingOptions FormattingOptions;
		FormattingOptions.MinimumFractionalDigits = MinFractionDigits;
		FormattingOptions.MaximumFractionalDigits = MaxFractionDigits;
	FText ValueText = FText::AsNumber(Value, &FormattingOptions);

	2. If you convert float/int32 to FString (first):
	(1) Use FString::Printf( TEXT(" %.2f"), __) <=> 2 fractional digits
	(2) FText::AsNumber(float/int32 InValue, FNumberFormattingOptions).ToString()
	 */
	FNumberFormattingOptions FormattingOptions;
		FormattingOptions.MinimumFractionalDigits = MinFractionDigits;
		FormattingOptions.MaximumFractionalDigits = MaxFractionDigits;
	FText ValueText = FText::AsNumber(Value, &FormattingOptions);
	WBP_Leaf_LabeledNumber->SetValueText(ValueText, bCollapseValue);
}

void FItemFragment_Widget_LabeledNumber::InitializeFragment()
{
	//currently empty: 
	Super::InitializeFragment();

	//next time it won't be executed (provided that it is appropriately assigned back to the drop item):
	if (bRandomizeValue)
	{
		Value = FMath::RandRange(MinValue, MaxValue);
		bRandomizeValue = false;
	}
}

/******Widget Fragments******/
