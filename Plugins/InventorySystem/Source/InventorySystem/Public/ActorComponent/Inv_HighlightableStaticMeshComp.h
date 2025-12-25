// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Interface/Inv_HighlightInterface.h"
#include "Inv_HighlightableStaticMeshComp.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UInv_HighlightableStaticMeshComp : public UStaticMeshComponent, public IInv_HighlightInterface
{
	GENERATED_BODY()
public:
	/*
	UMaterialInterface //used most of the time - because this is the parent of all so you can select any child types :D :D
	UMaterial : UMaterialInterface -->directly M_Material //never used so far
	UMaterialInstance : UMaterialInterface -->MI_Material //used sometimes
	UMaterialInstanceDynamic: UMaterialInterface          //used sometimes
	 */
	UPROPERTY(EditAnywhere)
	UMaterialInterface* MyOverlayMaterial; //it matches the built-in name, hence change it
	
	virtual void Highlight_Implementation() override;
	virtual void UnHighlight_Implementation() override;
};
