// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/CompositePattern/UW_Inv_Leaf.h"
#include "UW_Inv_Leaf_Text.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_Leaf_Text : public UUW_Inv_Leaf
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock;
protected:
	virtual void NativePreConstruct() override;

//anyway these are just for fun, because it will be override by ItemFragment_Widget_X::Values anyway = HENCE make your option from ItemFragment_Widget_X, not here -- - just for preview purpose nothing more
	//OPTION1
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 FontSize{12};          

	//OPTION2: so that you can edit everything of font (not just size) - I like this better because it gives more control from ItemManifest/WBP_Host side (and so you can have many WBP_Leaf_Text instances with different Font in WBP_ItemDescription)
	//but you may not able to see "the preview lol" = use the trick "copy properties" from WBP_X:::Font and paste it back to anywhere else lol
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateFontInfo FontInfo; 
};
