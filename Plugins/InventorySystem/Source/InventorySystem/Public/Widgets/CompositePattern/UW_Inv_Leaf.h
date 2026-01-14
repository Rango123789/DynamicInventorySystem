// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UW_Inv_CompositeBase.h"
#include "UW_Inv_Leaf.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_Leaf : public UUW_Inv_CompositeBase
{
	GENERATED_BODY()
public:
	virtual void ApplyFunction(TFunction<void(UUW_Inv_CompositeBase*)> FunctionToExecute) override;
protected:
};
