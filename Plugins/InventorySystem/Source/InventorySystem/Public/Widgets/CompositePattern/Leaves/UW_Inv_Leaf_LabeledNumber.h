// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/CompositePattern/UW_Inv_Leaf.h"
#include "UW_Inv_Leaf_LabeledNumber.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_Leaf_LabeledNumber : public UUW_Inv_Leaf
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_Label;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_Value;

	//i think this is kind of lame, you can either direct access or create a separate function to collapse it lol. 
	void SetLabelText(const FText& InText ,bool bCollapsed = false) const;
	void SetValueText(const FText& InText ,bool bCollapsed = false) const;
protected:
	virtual void NativePreConstruct() override;

	//stephen use  "float FontSize" which give it less control from WBP_Host/WBP_ItemDesc (we're not talking about FragmentSide at all, it won't give any Values about font currently, so no concern)
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateFontInfo FontInfo_Label; 

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateFontInfo FontInfo_Value; 
};
