// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Inv_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InventorySystem.h"
#include "ActorComponent/Inv_InventoryComponent.h"
#include "ActorComponent/Inv_ItemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Interface/Inv_HighlightInterface.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/HUD/UW_Inv_HUDWidget.h"

AInv_PlayerController::AInv_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AInv_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	//only valid in DC, so you must check before use (we currently consider for Multiplayer game)
	UEnhancedInputLocalPlayerSubsystem* EILPSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	if (IsValid(EILPSubsystem))
	{
		EILPSubsystem->AddMappingContext(IMC_Default_PC, 0);
	}

	CreateHUDWidget();

	//as long as we add it from BP_ItemComponent (and then add BP_ItemComponent to this PC), we should expect it got a valid one:
	InventoryComponent = FindComponentByClass<UInv_InventoryComponent>();
}

void AInv_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TraceForItem();
}

//PC::BeginPlay called in both DC and server, it doesn't make sense if you do it in the server so yeah.
//HUD only exist in DC (hence ::BeginPlay could only trigger in DC, hence if we do it in HUD we don't need such if (IsLocalController()))
void AInv_PlayerController::CreateHUDWidget()
{
	if (IsLocalController())
	{
		WBP_HUDWidget = CreateWidget<UUW_Inv_HUDWidget>(this, HUDWidget_Class);
		if (IsValid(WBP_HUDWidget))	WBP_HUDWidget->AddToViewport(); //if check or not up to you.
	}
}

//because the hosting function itself may only trigger on the CD - GPT
void AInv_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return;

	//use "ETrigger::Started" will make it trigger ONCE per press, even if we left IA_X::Modifiers=none (default to "special down" trigger per frame if you hold the key) - I explain why in UI course already! Or you could do "::Triggered" && "IA_X::Modifiers=Pressed" to achieve the same goal:
	EnhancedInputComponent->BindAction(IA_PrimaryInteract, ETriggerEvent::Started, this, &ThisClass::Input_PrimaryInteract);
	EnhancedInputComponent->BindAction(IA_ToggleInventory, ETriggerEvent::Started, this, &ThisClass::Input_ToggleInventory);
}

void AInv_PlayerController::Input_PrimaryInteract()
{
	DebugHelpers::Print("Primary Interact");

//step1: we must at least check if we're currently looking at any item at all before proceed (otherwise it is kind of lame):
	if (ThisActor == nullptr) return; //pass this just mean it is an actor blocking "ItemTrace" TraceChannel

	UInv_ItemComponent* ItemComponent = ThisActor->FindComponentByClass<UInv_ItemComponent>();
	if (IsValid(ItemComponent) == false) return; //pass this mean it is definitely an item (an actor having ItemComp)

//step2: (the check ensure you don't press the key too soon when Inventory isn't assigned yet, who know!)
	if (InventoryComponent.IsValid())
	{
		//InventoryComponent->NoRoomDelegate.Broadcast();
		InventoryComponent->TryAddItemToPlayerInventory(ItemComponent);
	}
}

//review: the callback of the IA_X can have any kind of signature: empty, "F__Something" , whaterver -- this is what I learnt in GAS course
void AInv_PlayerController::Input_ToggleInventory()
{
	if (InventoryComponent.IsValid()) InventoryComponent->ToggleInventory();
}

void AInv_PlayerController::TraceForItem()
{
//step0:
	if (IsLocalController() == false) return;
	
//step1: Get location and direction in world of Center screen:

	int32 SizeX{};
	int32 SizeY{};

	/*behind the scene: Cast<ULocalPlayer>(Player)->ViewportClient (UGameViewportClient)->GetViewportSize(OutVector2D) = don't to this yourself neither in PC or not  -- this suggest you only have a happen ending in the CD (hence step0)
	- You can also do:
	    GEngine->GameViewport (UGameViewportClient)->GetViewportSize(OutVector2D)  = you will do this if you're not in PC
	- GPT: GEngine->GameViewport/Cast<ULocalPlayer>(Player)->ViewportClient ONLY exist on machines that have a viewport =  make sense! how can a DS device can have a "Viewport" lol!
	*/
	GetViewportSize(SizeX, SizeY); //return 0,0 if there is no HUD lol
	FVector CenterScreenLocation;
	FVector WorldDirection; // as GPT to know why it is FVector not FRotator
	/*behind the scene: in turn call UGameplayStatics::DeprojectScreenToWorld = you will do this if you're not in PC*/
	bool bSucceed = DeprojectScreenPositionToWorld(SizeX * 0.5f, SizeY * 0.5f, CenterScreenLocation, WorldDirection);
	if (bSucceed == false ) return;
	
	FVector Start = CenterScreenLocation;
	FVector End = CenterScreenLocation + WorldDirection * TraceLength;

//step2: do the trace
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End,TraceChannel);

//step3: [filter, whatever]
	LastActor = ThisActor; //do this before change ThisActor
	ThisActor = HitResult.GetActor();

	//this pattern is exact like we do in GAS (just show in different way). Note that the order of th 2nd or 3rd if blocks doesn't matter, because we exclude the "==" case, so that can't be both valid at the same time.
	if (ThisActor == LastActor) return;

	if (ThisActor.IsValid() && IsValid(WBP_HUDWidget))
	{
		if (UInv_ItemComponent* ItemComponent= ThisActor->FindComponentByClass<UInv_ItemComponent>())
		{
			WBP_HUDWidget->ShowPickupMessage(ItemComponent->PickupMessage);
		}

		/*Which one to use:
		AActor::FindComponentByInterface(U__Interface::StaticClass()) - overload1,  return UActorComponent*
		|| AActor::FindComponentByInterface<U__Interface>() - overload2, return I__ deprecated
		|| AActor::FindComponentByInterface<I__Interface>() - overload3, return I__recommended by DOC
		 will help to detect if that component ": IInterface"

		+if you need to call BPNativeEvent function in I__Iterface
		, you must use this I__Iterface::Exc_function(UObject)
		, hence in this case you want overload1 (if you use overload3 you need to cast back to specific U__Object which is stupid and break the reason why you use interface :D :D or just UObject so that you don't break it)

		+if you simple to call "virtual void" or "BPImplementableEvent"
		, you surely prefer overload3

		+never use overload2
		
		----HERE:
		you can't do this because it is BPNativeEvent
		if (IInv_HighlightInterface* IHighlightInterface = ThisActor->FindComponentByInterface<IInv_HighlightInterface>())
		{
			IHighlightInterface->Highlight();
		} */
		if (UActorComponent* ActorComponent = ThisActor->FindComponentByInterface(UInv_HighlightInterface::StaticClass()); IsValid(ActorComponent))
		{
			//call it without extra check because we just "equivalently check it above" via "find" right :D :D
			IInv_HighlightInterface::Execute_Highlight(ActorComponent);
		}
	}

	//this also works somehow (like in GAS), but if you do: "if (!ThisActor.IsValid()) WBP_HUDWidget->HidePickupMessage()" before the "==" if - it also works, the reason is simple. In GAS course we need to call Enemy1,2->highlight/unhighlight - where in this course currently we only use call PC::WBP_HUDWidget::Show/Hide (helper belong to PC::WBP_HUDWidget itself)
	if (LastActor.IsValid() && IsValid(WBP_HUDWidget))
	{
		WBP_HUDWidget->HidePickupMessage();

		if (UActorComponent* ActorComponent = LastActor->FindComponentByInterface(UInv_HighlightInterface::StaticClass()); IsValid(ActorComponent))
		{
			//call it without extra check because we just "equivalently check it above" via "find" right :D :D
			IInv_HighlightInterface::Execute_UnHighlight(ActorComponent);
		}
	}
}
