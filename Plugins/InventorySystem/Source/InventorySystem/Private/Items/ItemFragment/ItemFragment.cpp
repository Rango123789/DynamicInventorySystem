// Fill out your copyright notice in the Description page of Project Settings.
#include "Items/ItemFragment/ItemFragment.h"

/******Consumable fragments**********/
void FItemFragment_Consumable_Health::OnConsume(APlayerController* PC)
{
	// Get a stats component from the PC or the PC->GetPawn()
	// or get the Ability System Component and apply a Gameplay Effect
	// or call an interface function for Healing()

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Health Potion consumed! Healing by: %f"), Health));
}

void FItemFragment_Consumable_Mana::OnConsume(APlayerController* PC)
{
	// Replenish mana however you wish

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Mana Potion consumed! Mana replenished by: %f"), Mana));
}
/******End Consumable fragments**********/