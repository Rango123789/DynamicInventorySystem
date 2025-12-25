// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/Inventory/Components/UW_Inv_SlottedItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUW_Inv_SlottedItem::SetImageIcon(UTexture2D* InIcon) const
{
	Image_Icon->SetBrushFromTexture(InIcon);
}

void UUW_Inv_SlottedItem::SetImageIcon(const FSlateBrush& InBrush) const
{
	Image_Icon->SetBrush(InBrush);
}

void UUW_Inv_SlottedItem::UpdateStackCount(const int32& InStackCount) const
{
	if (InStackCount > 0)
	{
		TextBlock_StackCount->SetVisibility(ESlateVisibility::Visible);
		TextBlock_StackCount->SetText(FText::FromString(FString::FromInt(InStackCount)));
	}
	else //assume "0" as it doesn't make sense for non-stackable item
	{
		TextBlock_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}
