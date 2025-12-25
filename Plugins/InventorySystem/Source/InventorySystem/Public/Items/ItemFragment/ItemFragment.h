// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "ItemFragment.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FItemFragment
{
	GENERATED_BODY()

//always do these when you inherit from FCustomStruct for polymorphic behavior (via FInstancedStruct/LIKE)
	FItemFragment(){}
	FItemFragment(const FItemFragment &other) = default;
	FItemFragment(FItemFragment &&other) = default;
	FItemFragment& operator=(const FItemFragment &other) = default;
	FItemFragment& operator=(FItemFragment &&other) = default;
	virtual ~FItemFragment(){}

	UPROPERTY(EditAnywhere, Category="Inventory", meta = (Categories = "Fragment"))
	FGameplayTag FragmentTag = FGameplayTag::EmptyTag;
};

/*automatically inherit the virtual destructor, where "constructors+" in child and separate, hence as long as you don't define touch anything special, you don't to repeat the rule of 5 (if I recall correctly, define a constructor with any param set will auto-remove the default constructor T(){} , where copy/assignment constructor/operator are still there by default)
 *GPT: What Happens When You Declare a Constructor With Parameters?
	❌ Default constructor T() is NOT generated

	✅ Copy constructor is still generated

	✅ Copy assignment operator is still generated

	❌ Move constructor is NOT generated (C++11+) = needVERIFIED

	❌ Move assignment operator is NOT generated = needVERIFIED
-->in any case, I believe, USTRUCT and UCLASS should cover the case you just define a custom constructor, and you don't have to redefine default move constructor and move assignment operator I beleive (I never see anyone do it nor did I see any built-in unreal code did it -- let's recheck next time)
= GPT said USTRUCT doesn't make any difference lol:
🔹If your USTRUCT is stored in UE containers (TArray, TMap, replicated arrays, FInstancedStruct, etc.)
👉 Yes — explicitly default move constructor & move assignment.

🔹 If your USTRUCT is never moved (stack-only, local params, transient)
👉 No — you don’t need to.

---------------
Question1: why I never see the Unreal built-in code doing such: T(T&& other) = default; UObject/USTRUCT doing it behind the scene?
Unreal doesn’t need to write T(T&&) = default; because it deliberately designs most engine types so the compiler can generate moves implicitly — or it avoids C++ move semantics entirely.
--------------
Question2: so whenever I do USTRUCT() struct FMyStruct { } - and I define a custom constructor, so I should define move constructor to?
🔹If your USTRUCT is stored in UE containers (TArray, TMap, replicated arrays, FInstancedStruct, etc.)
👉 Yes — explicitly default move constructor & move assignment.

🔹 If your USTRUCT is never moved (stack-only, local params, transient)
👉 No — you don’t need to.

+When you write:
USTRUCT()
struct FMyStruct
{
	GENERATED_BODY()
	FMyStruct(int32 InValue) : Value(InValue) {}
	int32 Value;
};

+You just did two things:
❌ Removed the implicit default constructor
❌ Disabled implicit move constructor & move assignment

+Now UE containers must fall back to:
Copy constructor
Or slower relocation paths
+That’s usually fine, but often unintended.

-------------
Question3: how about UCLASS and UMyObject : UObject case?
Short Answer (Very Important)
❌ You should NOT define move constructors or move assignment operators for UCLASS / UObject types.
UObjects are conceptually non-movable.
This is by design, not a limitation.

*/
USTRUCT(BlueprintType)
struct FItemFragment_Grid : public FItemFragment //to keep track of how many slots/spaces an item should take you
{
	GENERATED_BODY()

	//just mean "the total size: a slot * b slot" of the WBP_SlottedItem in the Grid.
	//GridSize is quite okay, but Dimensions is more perfect (better use GridSize for float)
	//In fact UGM use ImageSize to mean "width * height" (FVector2D) all the times
	//So anyway This time I use the term "dimensions" so that is meant slightly different
	UPROPERTY(EditAnywhere, Category="Inventory")
	FIntPoint GridDimensions = FIntPoint(1, 1);

	/*This is NOT GridPadding of "Slot_i <-> Slot_i+1" (not so far GridSlot has no padding between them, it is its background image to do the VISUAL padding, so yeah)
	 *This is the padding of whole WBP_Item within [GridDimensions], i,e within Canvas::GridSlot_start->GridSlot_end 
	 - UPDATE: NEXT time better off do "PaddingX,PaddingY" so that we can twist to avoid WBP_Item::SourceImage to be distorted when it try to fix into GridSize/GridDimensions lol
	*/
	UPROPERTY(EditAnywhere, Category="Inventory")
	float GridPadding = 0.f;
	
};

USTRUCT(BlueprintType)
struct FItemFragment_Image : public FItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Inventory")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, Category="Inventory")
	FVector2D IconSize = FVector2D(44.f, 44.f); //Stephen call it IconDimensions
};

/*IMPORTANT:
 *because we don't make "bStackable" member here (we shouldn't neither), hence if the return of GetFragment< FItemFragment_Stackable> = nullptr we decide it is "non-stackable item" (and so values in this struct don't need to even exist nor will it make any sense for "non-stackable item" if it exists)
 *hence those default values doesn't account for/fallback to "non-stackable items", explain why they has the values of 1 or such 
 */
USTRUCT(BlueprintType)
struct FItemFragment_Stackable : public FItemFragment
{
	GENERATED_BODY()

	//how many the BP_Item:::ItemComp::ItemManifest::Fragments::Fragment_Stackable::StackCount in world (can be changed differently in world if you want to or LIKE)
	//we may need to modify this (i,e modify BP_Item:::ItemComp::ItemManifest::Fragments::Fragment_Stackable::StackCount) in the case we can't pick all of it because inventory is full
	UPROPERTY(EditAnywhere, Category="Inventory")
	int32 StackCount = 1; 

	//the maximum count per "GridDimensions" it can stack on
	UPROPERTY(EditAnywhere, Category="Inventory")
	int32  MaxStackSize = 1; //OR MaxStackCountPerGridDimensions
};
