// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UW_Inv_CompositeBase.h"
#include "UW_Inv_Composite.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_Composite : public UUW_Inv_CompositeBase
{
	GENERATED_BODY()
public:

	virtual void ApplyFunction(TFunction<void(UUW_Inv_CompositeBase*)> FunctionToExecute) override;
	virtual void Collapse() override;
protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUW_Inv_CompositeBase>> Children;
	
};
