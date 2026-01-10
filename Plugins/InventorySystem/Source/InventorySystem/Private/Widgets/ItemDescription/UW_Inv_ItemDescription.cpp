// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ItemDescription/UW_Inv_ItemDescription.h"

#include "Components/SizeBox.h"

FVector2D UUW_Inv_ItemDescription::GetSize() const
{
	return SizeBox->GetDesiredSize();
}
