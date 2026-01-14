// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/CompositePattern/Leaves/UW_Inv_Leaf_Image.h"

#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "InventoryTags/InventoryTags.h"

UUW_Inv_Leaf_Image::UUW_Inv_Leaf_Image()
{
	//I like to set it beforehand, but you can set it in WBP_Leaf_Image if you want
	FragmentTag = ItemFragmentTags::Fragment_Image;
}

void UUW_Inv_Leaf_Image::SetImageIcon(UTexture2D* InIcon) const
{
	Image_Icon->SetBrushFromTexture(InIcon);
}

void UUW_Inv_Leaf_Image::SetImageIcon(const FSlateBrush& InBrush) const
{
	Image_Icon->SetBrush(InBrush);
}

void UUW_Inv_Leaf_Image::SetImageSize(const FVector2D& InSize) const
{
	//this get "predicated warning": Image_Icon->SetBrushSize(InSize) and suggest we use the bellow:
	Image_Icon->SetDesiredSizeOverride(InSize);
}

void UUW_Inv_Leaf_Image::SetSizeBoxSize(const FVector2D& InSize) const
{
	//SizeBox didn't have local SetSize, it has SetWidthOverride, .... at a time:
	SizeBox->SetWidthOverride(InSize.X);
	SizeBox->SetHeightOverride(InSize.Y);
}

