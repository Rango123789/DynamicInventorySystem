// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponent/Inv_ItemComponent.h"

#include "Net/UnrealNetwork.h"

UInv_ItemComponent::UInv_ItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInv_ItemComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, SourceItemManifest); //ThisClass works, I recall it didn't work?
}

//currently called from Server in ServerRPC
void UInv_ItemComponent::PickedUp()
{
	//better remove this line, it is called in server and won't even reach clients, just use BP_Item::OnDestroyed for cosmetic effect (define it from BP is fine!) 
	OnPickedUp();
	
	GetOwner()->Destroy();
}


