// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/CompositePattern/Leaves/UW_Inv_Leaf_LabeledNumber.h"

#include "Components/TextBlock.h"
#include "InventoryTags/InventoryTags.h"

void UUW_Inv_Leaf_LabeledNumber::SetLabelText(const FText& InText, bool bCollapsed) const
{
	if (bCollapsed)
	{
		TextBlock_Label->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	TextBlock_Label->SetText(InText);
}

void UUW_Inv_Leaf_LabeledNumber::SetValueText(const FText& InText, bool bCollapsed) const
{
	if (bCollapsed)
	{
		TextBlock_Value->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	TextBlock_Value->SetText(InText);
}

void UUW_Inv_Leaf_LabeledNumber::NativePreConstruct()
{
	Super::NativePreConstruct();

	FragmentTag = ItemFragmentTags::Fragment_Widget_LabeledNumber;
	TextBlock_Label->SetFont(FontInfo_Label);
	TextBlock_Value->SetFont(FontInfo_Value);
}
