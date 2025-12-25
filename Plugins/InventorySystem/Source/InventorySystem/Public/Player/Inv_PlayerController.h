// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventorySystem.h"
#include "GameFramework/PlayerController.h"
#include "Types/CustomFastArrayTypes.h"
#include "Inv_PlayerController.generated.h"

class UInv_InventoryComponent;
class UUW_Inv_HUDWidget;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API AInv_PlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	//this to be added in BP_InventoryComponent, this is not the place construct it, hence I decide to make "TWeakObjectPtr" (whatever)
	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	
protected:
	AInv_PlayerController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void CreateHUDWidget();

	void Input_PrimaryInteract();
	UFUNCTION(BlueprintCallable , Category = "Inventory")
	void Input_ToggleInventory();

	void TraceForItem();

	UPROPERTY(EditAnywhere, Category="Inventory")
	TSubclassOf<UUW_Inv_HUDWidget> HUDWidget_Class;
	
	UPROPERTY(BlueprintReadOnly,  Category="Inventory")
	TObjectPtr<UUW_Inv_HUDWidget> WBP_HUDWidget;

	UPROPERTY(EditAnywhere, Category="Inventory")
	float TraceLength = 500.f;

	/*Shocking news
	+you can NOT add "EditAnywhere" for "UENUM enum"
	, it only works for "UENUM class enum" (or "enum class" with an explicit underlying type)
	, hence such a legacy must be wrapped in "TEnumAsByte<>" for it to work

	+you must wrap it it "TEnumAsByte<>", and it will work lol
	, so know I start to realize why they use TEnumAsByte

	+ECollionChannel is a legacy that only declared with "UENUM enum" lol

	@@note: the reason why stephen even let it EditAnywhere is that we want to select it from BP so that it can't be wrong (rather than create macro constant like in other courses that could be error-prone)
	 */
	UPROPERTY(EditAnywhere, Category="Inventory")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_ItemTrace;
	
private:
	//AllowPrivateAccess ="true" - only need if we use "BPReadOnly" or "BPReadWrite" to access in Event Graph
	UPROPERTY(EditAnywhere, Category="Inventory") 
	TObjectPtr<UInputMappingContext> IMC_Default_PC;
	 
	UPROPERTY(EditAnywhere,  Category="Inventory")
	TObjectPtr<UInputAction> IA_PrimaryInteract;

	UPROPERTY(EditAnywhere,  Category="Inventory")
	TObjectPtr<UInputAction> IA_ToggleInventory;

	/*GAS course: and then use IsValid(Actor)
		UPROPERTY()
		TObjectPtr<AActor> LastActor;
		UPROPERTY()
		TObjectPtr<AActor> ThisActor;
	*/
	TWeakObjectPtr<AActor> LastActor;
	TWeakObjectPtr<AActor> ThisActor;	


public:

};
