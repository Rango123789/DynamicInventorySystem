// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventorySystem.h"
#include "GameFramework/PlayerController.h"
#include "Inv_PlayerController.generated.h"

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

protected:
	AInv_PlayerController();
	virtual void Tick(float DeltaSeconds) override;
	void CreateHUDWidget();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void Input_PrimaryInteract();

	void TraceForItem();

	UPROPERTY(EditAnywhere, meta=(Category="Inventory"))
	TSubclassOf<UUW_Inv_HUDWidget> HUDWidget_Class;
	
	UPROPERTY(BlueprintReadOnly,  meta=(Category="Inventory"))
	TObjectPtr<UUW_Inv_HUDWidget> WBP_HUDWidget;

	UPROPERTY(EditAnywhere, meta=(Category="Inventory"))
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
	UPROPERTY(EditAnywhere, meta=(Category="Inventory"))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_ItemTrace;
	
private:
	//AllowPrivateAccess ="true" - only need if we use "BPReadOnly" or "BPReadWrite" to access in Event Graph
	UPROPERTY(EditAnywhere, meta=(Category="Inventory")) 
	TObjectPtr<UInputMappingContext> IMC_Default_PC;
	 
	UPROPERTY(EditAnywhere,  meta=(Category="Inventory"))
	TObjectPtr<UInputAction> IA_PrimaryInteract;

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
