// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/CompositePattern/UW_Inv_Composite.h"

#include "Blueprint/WidgetTree.h"

void UUW_Inv_Composite::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//ChildWidget mean any widget within this WBP_Composite[/tree]:
	WidgetTree->ForEachWidget(
		[this](UWidget* ChildWidget)
		{
			//IsValid(Item) is redundant I think
			if (UUW_Inv_CompositeBase* Item = Cast<UUW_Inv_CompositeBase>(ChildWidget); IsValid(Item)) 
			{
				Children.Add(Item);
			}
		}
	);
}


void UUW_Inv_Composite::Collapse()
{
	for (UUW_Inv_CompositeBase* Child : Children)
	{
		Child->SetVisibility(ESlateVisibility::Collapsed);	
	}
}

void UUW_Inv_Composite::ApplyFunction(TFunction<void(UUW_Inv_CompositeBase*)> FunctionToExecute)
{
	for (UUW_Inv_CompositeBase* Child : Children)
	{
		//funny this is NOT the call from parent part, this is the call from "an instance", hence it need to be PUBLIC :D :D
		Child->ApplyFunction(FunctionToExecute);
	}
}
