// Fill out your copyright notice in the Description page of Project Settings.
#include "ActorComponent/Inv_InventoryComponent.h"

#include "ActorComponent/Inv_ItemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Items/ItemData.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Inventory/UW_Inv_Inventory_Spacial.h"

//SetIsReplicatedByDefault(true) is the version you call in UActorComponent::constructor the other version is called only when it is called externally say in HostingActor or when you need to turn it off (never feel this need):
UInv_InventoryComponent::UInv_InventoryComponent()
	: ItemFastArray(this)
{
	PrimaryComponentTick.bCanEverTick = false;

//replication:
	SetIsReplicatedByDefault(true);

	//new: when you want to [DYNAMICALLY] add UObject to be replicated as sub object of this ActorComponent (not just itself):
	//you don't need to do this for AActor (just neeed to call AddReplicatedSubObject(InObject))
	bReplicateUsingRegisteredSubObjectList = true;
}

void UInv_InventoryComponent::AddReplicatedSubObjectHelper(UObject* InObject)
{
	if (IsUsingRegisteredSubObjectList() && //just to make sure "bReplicateUsingRegisteredSubObjectList = true"
		IsReadyForReplication() &&          //optional, to make sure "bIsReadyForReplication = true" and ReadyForReplication has been called
		IsValid(InObject)                   //nothing to talk about
	)
	{	
		AddReplicatedSubObject(InObject);
	}
}

void UInv_InventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//ThisClass work here? I recall some macro won't work with "ThisClass" lol? whatever
	DOREPLIFETIME(ThisClass, ItemFastArray);
	
}

void UInv_InventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

// Called when the game starts
void UInv_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupInventory();
}

/*universal rules:
 -the lower tire (say component of an actor) don't need to worry about the case its higher tire exists or not. Because if higher tire doesn't exist so the lower tire functions never get called
-if this comp is to be used in PC only, that exist in DC and server only, then this comp is constructed in server and CD only, hence ::BeginPlay->::SetupInventory() could only be called in server and CD only MAXIMUM.
-consequently Cast<APlayerController>(GetOwner()) should be always valid*/
void UInv_InventoryComponent::SetupInventory()
{
	OwningPlayerController = Cast<APlayerController>(GetOwner());
	//this mean you don't want other actors to use this comp and you expect PC is valid at this point
	check(IsValid(OwningPlayerController.Get()));

	//we don't want to construct the HUD in server:
	if (OwningPlayerController->IsLocalController() == false) return;

	//we want the PC is owner of this widget (and other widgets), to be exact we only have 4/5 choices: PC, GetWorld(), GetGameInstance(), UWidget
	WBP_Inventory_Spacial = CreateWidget<UUW_Inv_Inventory_Spacial>(OwningPlayerController.Get(), InventoryClass);

	WBP_Inventory_Spacial->AddToViewport();
	CloseInventory();
}

void UInv_InventoryComponent::OnRep_ItemFastArray()
{
}

void UInv_InventoryComponent::ToggleInventory()
{
	if (IsInventoryOpen) CloseInventory();
	else OpenInventory();
}

void UInv_InventoryComponent::OpenInventory()
{
	if (OwningPlayerController.IsValid() == false || IsValid(WBP_Inventory_Spacial) == false) return;

	IsInventoryOpen = true;
//visibility:
	WBP_Inventory_Spacial->SetVisibility(ESlateVisibility::Visible);
	
//input mode:
	FInputModeGameAndUI InputMode;
	OwningPlayerController->SetInputMode(InputMode);
	OwningPlayerController->SetShowMouseCursor(true);
}

void UInv_InventoryComponent::CloseInventory()
{
	if (OwningPlayerController.IsValid() == false || IsValid(WBP_Inventory_Spacial) == false) return;

	IsInventoryOpen = false;
	
//visibility:
	WBP_Inventory_Spacial->SetVisibility(ESlateVisibility::Collapsed);
	
//input mode:
	FInputModeGameOnly InputMode;
	OwningPlayerController->SetInputMode(InputMode);
	OwningPlayerController->SetShowMouseCursor(false);
}

//this is the only overload at this level we need PC trace for BP_Item::ItemComponent, hence that's what we directly at first place
void UInv_InventoryComponent::TryAddItemToPlayerInventory(UInv_ItemComponent* InItemComponent)
{
	/*WBP_Inventory_Spacial::Grids will be naturally responsible for telling whether WBP_ItemData already exist on its own, we'll get to that, don't worry
	 *So the whole chapter 5 is only for this 
	 */
	FInventoryAvailabilityInfo AvailabilityInfo = WBP_Inventory_Spacial->GetAvailabilityInfoForItem(InItemComponent);

	//basically we see if it already exist in InventoryComponent::ItemFastArray
	UItemData* FoundExistingItemData = ItemFastArray.FindItemDataByItemTag(InItemComponent->SourceItemManifest.ItemTag);
	AvailabilityInfo.ItemData = FoundExistingItemData; //no matter it is valid or nullptr. Important: TWeakObjectPtr<T> = T*, need include T.h for conversion

	if ( AvailabilityInfo.TotalRoomToFill == 0)
	{
		NoRoomDelegate.Broadcast();	
		return;
	}

	/*if the ItemData already exist in PlayerInventory and it is stackable, then we simply just "stack" it (not sure about the case we have to create a duplicate widget when one slot is full? well it is a different visual matter I guess - if it is stackable we keep a total count and create a single instance of ItemData for it no matter how many slots it can span over)
	- why you need these set of params in this case is based on what you need, you'll see more clear later:
	- you may be wondering why don't we pass along AvailabilityInfo? well because it can be easily get back anywhere (server or DC) as long as we got ItemComponent/ItemData, or to be clear as long as we got ItemManifest to compare again WBP_Grid::GridSlots
	- Hence it will be retrieved independently again when we need it to create/update WBP_SlottedItem for VISUAL part*/
	if (AvailabilityInfo.ItemData.IsValid() && AvailabilityInfo.bStackable)
	{
		//broadcast it here mean only CD get it, what's all we need in this case (because WBP_SlottedItem only need to create in CD):
		//UPDATE: we need we broadcast AvailabilityInfo (that is also just assigned AvailabilityInfo.ItemData), not ItemData (because GetAvailabilityInfoForItem(ItemData/ItemComponent) won't have AvailabilityInfo.ItemData set - this is the only exception - because whether it exist or not must be found from FastArray itself)
		OnStacksAdded.Broadcast(AvailabilityInfo);
		
		// Add stacks to an item that already exists in the inventory. We only want to update the stack count,
		// not create a new item of this type.
		ServerRPC_AddStacksToExistingStackableItem(InItemComponent, AvailabilityInfo.TotalRoomToFill, AvailabilityInfo.Remainder);
	}
	/*if it is not already appear in PlayerInventory (either stackable or not) or it is NOT stackable (either it already appear in PlayerInventory or not) we need to create a new item instance of ItemData (if it is not-stackable ItemData and already appear in PlayerInventory then we still need to create "a separate instance of the same ItemData type" (hence hold a separate data set, unlike a specific type of stackable ItemData will only share one instance and increase the "total count" only, where how many slots it spans is just a sparate VISUAL matter) )
	- the (!A || !B) is not needed because it naturally "mutually exclusive" with "A && B", (!A || !B) && C make it possible that it could be that none of "if or else if" is executed at all*/
	else if ( (!AvailabilityInfo.ItemData.IsValid() || !AvailabilityInfo.bStackable) && AvailabilityInfo.TotalRoomToFill > 0)
	{
		/* This item type doesn't exist in the inventory. Create a new one and update all pertinent slots.
		- I don't agree with this practice, just pass in  AvailabilityInfo.TotalRoomToFill (stackable or not) and bStackable (because what if you pick a collection of items in a box that have 2 non-stackable items of the same type right? well it doesn't look like it support in this course, anyway we can always avoid this by only allow maximum on non-stackable item of the same type PER pickup/collection pickup! yeah!)
		; I DON'T DO THIS:
		   ServerRPC_AddNewItem(InItemComponent, AvailabilityInfo.bStackable ? AvailabilityInfo.TotalRoomToFill : 0);
		-well you may want to accept it because the param name is "ItemStackCount" specifically, not "TotalRoomToFill", but I will add bStackable anyway
		- ItemStackCount = AvailabilityInfo.TotalRoomToFill = 1 (if non-stackable) or AnyAmount (if stackable) */
		ServerRPC_AddNewItem(InItemComponent, AvailabilityInfo.bStackable, AvailabilityInfo.TotalRoomToFill);
	}
}

/*I better off rename ItemStackCount to "TotalRoomToFill" and understand that:
- it will be TotalItemStackCount in case it is stackable
- it will be "1" in case it is non-stackable and it has no use
EITHER way, in this case we don't need bStackable because we did check it is stackable before calling this RPC1:
*/
void UInv_InventoryComponent::ServerRPC_AddStacksToExistingStackableItem_Implementation(
	UInv_ItemComponent* ItemComponent, int32 TotalRoomToFill, int32 Remainder)
{
	//we can also pass it from the Outer function, but anyway perhaps for the sake of reduce NetBandwith lol
	UItemData* ExistingItemData = ItemFastArray.FindItemDataByItemTag(ItemComponent->SourceItemManifest.ItemTag);
	if (IsValid(ExistingItemData) == false) return; //it can't be, because we did it once outside to each here
	
	ExistingItemData->TotalStackCount += TotalRoomToFill;
	
	//TODO: tell the ItemComponent to destroy its owning actor (BP_Item) in the level if Remainder=0, otherwise update the BP_Item::ItemManifest::Fragment_Stackable::StackCount :
	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else
	{
		//the current version return "const T*" so we can't modify it lol, hence how we need the multable version:
		if (FItemFragment_Stackable* ItemFragment_Stackable =
				ItemComponent->SourceItemManifest.GetMutableItemFragmentByType<FItemFragment_Stackable>())
		{
			ItemFragment_Stackable->StackCount = Remainder; //both Stephen and I set it correctly already
		}
	}
}

//but this like I love to add "bStackable" to make it clear
void UInv_InventoryComponent::ServerRPC_AddNewItem_Implementation(UInv_ItemComponent* ItemComponent,
                                                                  bool bStackable, int32 TotalRoomToFill)
{
	UItemData* AddedItemData = ItemFastArray.AddItemEntry(ItemComponent);
	if (IsValid(AddedItemData) == false) return;
	
	/*I just set it like that no the case of non-stackable, or just directly "TotalRoomToFill" if you want (it must be "1" for non-stackable item since the calculation/convention we make in GetAvailabilityInfo chain, because it doesn't make sense anyway
	 *but an existing ItemData with ::TotalStackCount=0 is a better sign for me, so that we don't need to try to find out it is "stackable" or not! because if you choose "1", it can still be no different from a stackable item that actually have stackcount of 1 lol!)*/
	AddedItemData->TotalStackCount = bStackable ? TotalRoomToFill : 0; 

	/* OPTION1 is call the equivalent code here, OPTION2 is call "item-level Post/PreReplicatedX" right in FastArray::AddEntry, right after Items.Add and before MarkItemDirty() ; but currently we have 2 overload of AddEntry from there, so call it from there isn't very wise right lol
	 * again you can't not call the "Array-level Post/PreReplicatedX" because you need to pass in "AddedIndices" which really inconvenient to find and get it lol.
	 * OPTION3 is just directly do the equivalent code "InventoryComponent->OnItemAdded.Broadcast" right in AddX = the best option, that is suggested by GPT!
	-UActorComponent also has GetNetMode()
	-the only reason we do this check is that we want to exclude the "NM_DediatedServer" case (HasAuthority could be either 3 of them: standalone, DS, LS).
	-But this is optional anyway, the sub chain will deal with HUD and Widgets that doesn't exist in DS and will be naturally stop (hence do the check nullptr before proceed of course):*/
	if (GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer)
	{
		OnItemAdded.Broadcast(AddedItemData);
	}

	//TODO: tell the ItemComponent to destroy its owning actor (BP_Item) in the level (well there is a possibility that you need to update BP_Item::ItemManifest::Fragment_Stackable::StackCount as well if it is the first time you pick it up but you don't have room for all stackcount lol):
	ItemComponent->PickedUp();
}

