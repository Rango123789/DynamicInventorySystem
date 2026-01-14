// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "UW_Inv_CompositeBase.generated.h"

/**
 * you may want to call it "ElementBase"
 */
UCLASS()
class INVENTORYSYSTEM_API UUW_Inv_CompositeBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
	//so each UUW_Composite or _Leaf will have its own FragmentTag (supposedly appear in ItemData::ItemManifest as well)
	UPROPERTY(EditAnywhere, Category="Inventory")
	FGameplayTag FragmentTag;
	
protected:

public:
	/*relevant IMPORTANT conventions to know:
(1) collapsed parent 
= child will effectively be collapsed too (even if its internal value remain the same)

(2) child visible, parent not hitestible (parent and child) 
= child will be effectively not hitestible as well
; Not Hit-Testable (Self & All Children) blocks hit testing for the entire subtree.

(3) child collapsed, parent not hitestible (parent and child) 
= you don't see the child (verified by GPT)
; Parent hit-test settings don’t matter here because the child is gone from layout/rendering entirely.
 */
	//perhaps the reason stephen it to be virtual is that we may need to do additional code as collapsed, rather than just call SetVisibility(Collapsed) - as collapse WBP_Parent will effectively collapse its children as well (but their internal Visibilities stay the same value)
	virtual void Collapse(){};
	//stephen didn't make this one virtual
	virtual void Expand();

	//the cool thing about this is that you can DYNAMICALLY define and execute any arbitrary function of this signature on all elements of composite pattern
	//composite child will loop through all children and in turn call child->ApplyFunction(FunctionToExecute) that ultimately do what we need (leaf is where the actual code will be done, composite is just forward the function call to all of its children, composite never has its own single separate element, it only has an array of children/elements)
	using FunctionType = TFunction<void(UUW_Inv_CompositeBase*)>; //OPTIONAL
	virtual void ApplyFunction(TFunction<void(UUW_Inv_CompositeBase*)> FunctionToExecute){}
};
