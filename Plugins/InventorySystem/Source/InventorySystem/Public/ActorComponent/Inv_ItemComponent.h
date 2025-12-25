// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/ItemManifest.h"
#include "Inv_ItemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORYSYSTEM_API UInv_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInv_ItemComponent();

	UPROPERTY(EditAnywhere, Category="Inventory")
	FText PickupMessage = FText::FromString("E - Pick up");

	//this is the SourceItemManifest to be EditAnywhere in BP_Item::ItemComponent and to be assigned to PC::InventoryComp::ItemFastArray::ItemEntries::ItemEntry_i::ItemData::ItemManifest[Wrapper] (to be actually go into PC::InventoryComp::WBP_Inventory)
	UPROPERTY(EditAnywhere, Category="Inventory")
	FItemManifest SourceItemManifest;

protected:

public:
	/*UPDATE:
- there is no need of BP_ItemComponent::OnPickedUp NOR BP_Item::OnPickedUp - because I won't even reach clients lol
- you can in fact use BP_Item::OnDestroyed trigger on all device if Destroy() is called in the server (even if it has no C++ parent nor C++ interface)
 */
	void PickedUp();
	UFUNCTION(BlueprintImplementableEvent)
	void OnPickedUp();
};
