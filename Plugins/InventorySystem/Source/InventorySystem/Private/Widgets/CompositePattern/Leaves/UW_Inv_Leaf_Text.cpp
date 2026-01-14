// Fill out your copyright notice in the Description page of Project Settings.
#include "Widgets/CompositePattern/Leaves/UW_Inv_Leaf_Text.h"
#include "Components/TextBlock.h"
#include "InventoryTags/InventoryTags.h"

void UUW_Inv_Leaf_Text::NativePreConstruct()
{
	Super::NativePreConstruct();

	FragmentTag = ItemFragmentTags::Fragment_Widget_Text;

/*OPTION1:
	//very smart technique when you want to modify a part of something and keep the rest as it is:
	FSlateFontInfo SlateFontInfo = TextBlock->GetFont();
	SlateFontInfo.Size = FontSize;
	
	TextBlock->SetFont(SlateFontInfo);
*/
	
//OPTION2:
	TextBlock->SetFont(FontInfo);
}
