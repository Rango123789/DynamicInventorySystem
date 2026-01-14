// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/CompositePattern/UW_Inv_Leaf.h"

void UUW_Inv_Leaf::ApplyFunction(TFunction<void(UUW_Inv_CompositeBase*)> FunctionToExecute)
{
	FunctionToExecute(this);
}
