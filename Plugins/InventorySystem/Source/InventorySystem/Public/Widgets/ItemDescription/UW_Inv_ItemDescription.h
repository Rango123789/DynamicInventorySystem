// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/CompositePattern/UW_Inv_Composite.h"
#include "UW_Inv_ItemDescription.generated.h"

class USizeBox;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_ItemDescription : public UUW_Inv_Composite //UPDATE parent
{
	GENERATED_BODY()
public:
	//simply return SizeBox->GetDesiredSize() , not necessarily LocalSize/AllottedSize, surely not rendering/absolute size. Optional anyway
	FVector2D GetSize() const;
protected:
	//this will be the root of this WBP_X
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox;
	
};
