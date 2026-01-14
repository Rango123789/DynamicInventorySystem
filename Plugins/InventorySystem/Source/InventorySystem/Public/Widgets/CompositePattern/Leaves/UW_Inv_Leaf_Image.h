// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/CompositePattern/UW_Inv_Leaf.h"
#include "UW_Inv_Leaf_Image.generated.h"

class USizeBox;
class UImage;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_Leaf_Image : public UUW_Inv_Leaf
{
	GENERATED_BODY()

public:
	UUW_Inv_Leaf_Image();
	
	void SetImageIcon(UTexture2D* InIcon) const;
	void SetImageIcon(const FSlateBrush& InBrush) const; //no need

	//both of them will be set to FItemFrament_Image::ImageSize, meaning either of them is redundant (but anyway lol)
	void SetImageSize(const FVector2D& InSize) const;   //will be size of WBP_Leaf_Image if we don't set SizeBox's Size
	void SetSizeBoxSize(const FVector2D& InSize) const; //it is root so effectively set size of WBP_Leaf_Image
	
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	//we want ItemFragment can control the size of this WBP_Leaf_Image within WBP_ItemDescription. So this time is no longer optional for our intention (we can hardcode of Height and Widget Override in WBP_X, but it will be overriden by the value from ItemManifest anyway)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox;
};
