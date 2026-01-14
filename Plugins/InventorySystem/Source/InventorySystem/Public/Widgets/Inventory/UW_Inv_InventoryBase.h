// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/InventoryTypes.h"
#include "UW_Inv_InventoryBase.generated.h"

class UInv_ItemComponent;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_InventoryBase : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual FInventoryAvailabilityInfo GetAvailabilityInfoForItem(UInv_ItemComponent* ItemComponent)
	{
		return FInventoryAvailabilityInfo();
	}

	virtual void OnItemHovered(UItemData* HoveredItemData){}
	virtual void OnItemUnhovered(){}
	virtual bool HasHoverItemInAction(){ return false; }
protected:
	
};
