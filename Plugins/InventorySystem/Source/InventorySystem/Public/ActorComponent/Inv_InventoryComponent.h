// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/CustomFastArrayTypes.h"
#include "Inv_InventoryComponent.generated.h"

struct FInventoryAvailabilityInfo;
class UInv_ItemComponent;
class UItemData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, UItemData*, ItemData);

//UPDATE: we need we broadcast AvailabilityInfo (that is also just assigned AvailabilityInfo.ItemData), not ItemData (because GetAvailabilityInfoForItem(ItemData/ItemComponent) won't have AvailabilityInfo.ItemData set - this is the only exception - because whether it exist or not must be found from FastArray itself)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStacksChange, const FInventoryAvailabilityInfo&, AvailabilityInfo); //or UItemData also okay?

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNoRoomDelegate);

class UUW_Inv_Inventory_Spacial;

//In this course, This come is only meant for PC:: , don't use it in other classes
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORYSYSTEM_API UInv_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInv_InventoryComponent();
	void AddReplicatedSubObjectHelper(UObject* InObject /*ToBeReplicatedAsSubObjectOfThisActorComponent*/);

	//I always want OnRep for fast array, note that the limitation to this is that you can't have "OldArray", hence if you need it one you need to manually create it (note that it is not for reason of overhead when having that param in general, because it doesn't cost extra replication at all, it handles locally)
	UPROPERTY(ReplicatedUsing = OnRep_ItemFastArray, EditAnywhere, Category = "Inventory")
	FInventoryItemFastArray ItemFastArray;

	UFUNCTION()
	void OnRep_ItemFastArray();
	
	UFUNCTION(BlueprintCallable)
	void ToggleInventory();
	UFUNCTION(BlueprintCallable)
	void CloseInventory();
	UFUNCTION(BlueprintCallable)
	void OpenInventory();

	bool IsInventoryOpen = false;

	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnItemAdded;
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnItemRemoved;

	UPROPERTY(BlueprintAssignable)
	FOnStacksChange OnStacksAdded;
	
	UPROPERTY(BlueprintAssignable)
	FNoRoomDelegate	NoRoomDelegate;

	void TryAddItemToPlayerInventory(UInv_ItemComponent* InItemComponent);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	//this one is for fun. happen before both UActorComponent/AActor(Owner)::BeginPlay(). But you can also use constructors already, so this functions rarely being used!
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

//you always got "HitActor::ItemComponent", so should be always a param in all cases:
	//we know in this case it is stackable, hence no pram of it:
	UFUNCTION(Server, Reliable)
	void ServerRPC_AddStacksToExistingStackableItem(UInv_ItemComponent* ItemComponent, int32 TotalRoomToFill, int32 Remainder); //renamed from "ItemStackCount"
	
	/*stackable or not still need to add to PlayerInventory when not exist there yet:
		//OPTION1: also pass in bStackable, in case it is false, ItemStackCount will be modified to "0" at server-side
			//UFUNCTION(Server, Reliable)
			//void ServerRPC_AddNewItem(UInv_ItemComponent* ItemComponent, int32 ItemStackCount, bool bStackable); 
		//OPTION2: pass in "ItemStackCount = 0" when AvailabilityInfo.bStackable=false, because it does make sense in case it is a non-stackable item. so that you don't need to send "bool bStackable" which save "Net bandwidth". Universal rule: always do all the side calculating at the DC device, so that you can send as less param as possible*/
	UFUNCTION(Server, Reliable)
	void ServerRPC_AddNewItem(UInv_ItemComponent* ItemComponent, bool bStackable, int32 TotalRoomToFill); //renamed from "ItemStackCount"
	
	//to be selected as WBP_Inventory_Spacial, don't know why STEPHEN must choose a parent pointer here? i decide to pick the bigger class, I don't fancy casting, I change it back if needed:
	UPROPERTY(EditAnywhere, meta = (Category = "Inventory"))
	TSubclassOf<UUW_Inv_Inventory_Spacial> InventoryClass;

	//if it is the first one construct something, let's use UPROPERTY() to hold and manage it
	UPROPERTY()
	TObjectPtr<UUW_Inv_Inventory_Spacial> WBP_Inventory_Spacial;

	//we follow good practice: if it is not the one first construct something, let's use "TWeakObjectPtr" from now on
	TWeakObjectPtr<APlayerController> OwningPlayerController;
private:
	void SetupInventory();
	
public:
	
};

/* Quick recap:
+the RPC_AddNewItem is complete (either for ExistingStackableItem or NewInstanceOfNonStackableItem)
; because it affect ItemFastArray and trigger sub chains (PostReplicatedAdd -> OnItemAdded..) on clients ONLY (and we also consider the case it is standardlone / DedicatedServer by broadcast OnItemAdded along right in the RPC already

+but the chain RPC_AddStacksToItem is NOT for "cosmetic part" (at least, it also in fact technical part because WBP_Grid::GridSlots::Values does effect the GetAvailabilityInfo for next pickup surely)
; because it doesn't affect ItemFastArray
; hence broadcast OnStacksAdded[ToExistingStackableItem] in EITHER:
(1) the RPC_AddStacksToItem = broadcast action only trigger where call it, not replicated  
= we don't need it trigger in the server in this sub chain because WBP_X exist in CD device only
(2) outside of RPC_AddStacksToItem in the outer TryAddItemToPlayerInventory (in is in the CD)
= we need this one!


+OnStackAdded can broadcast either:
(1) ItemData (so that you can GetAvailability from this ItemData - with the hope that it is consistent since you just GetAvailability in the outer TryAddItemToPlayerInventory = in fact it is consistent )
= so its param is the same as OnItemAdded delegate
(2) directly AvailabilityInfo (from GetAvailability just call from  TryAddItemToPlayerInventory)
= you may prefer this, it leaves no chance of inconsistency of Availability (retrieved at 2 different times)
= this is final option

+WBP_Grid::OnStacksAddedCallback 
{
  for(SlotInfo : AvaiInfo.SlotInfos)
  {
    if(SlotInfo.IsItemAtIndex) //the first existing one is to be filled up
    {
      //important for next GetAvailabilityInfo for next pickup
        //update WBP_GridSlot::StackCount += SlotInfo::AmountToFill 
      //visual part
        //update WBP_SlottedItem::StackCount += SlotInfo::AmountToFill 
    }
    else //next ones is not yet exist on HUD yet (but ItemData already exist - read @@IMPORTANT below to see why), hence WBP_SlottedItem need to be created 
    {
      //In fact all of these steps can be re-used, all the code inside the for loop inside AddItemWidgetsToIndices can be called here. so now you can factorize that function and call here if you want (you can keep those code where it was and only use the sub function here if you want)
        //create WBP_SlottedItem
        //set WBP_GridSlot::StackCount = SlotInfo::AmountToFill  
        //set WBP_SlottedItem::StackCount  = SlotInfo::AmountToFill  	    
    }
  }
}

@@IMPORTANT: 
- for stackable item need only one ItemData for all whole group of WBP_SlottedItem[s] of the same type no matter how many times you pick
- for non-stackable item it will be separate ItemData in FastArray for separate instances even if it is of the very same type)

@@NOTE: you can rename AddItemWidgetsToIndices to "AddItemWidgetToIndices" so that it sound more clear
, that this sub chain is only/main for cosmetic "widget" part (that in fact does affect technical part as well)

 */