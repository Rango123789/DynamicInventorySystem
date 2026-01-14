// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/Inv_BPFunctionLibrary.h"

#include "ActorComponent/Inv_InventoryComponent.h"
#include "Components/Widget.h"
#include "Player/Inv_PlayerController.h"
#include "Widgets/Inventory/UW_Inv_InventoryBase.h"

//so basically "rows" param doesn't matter at all! because the direction we count [0 ---> [colum] ]
int32 UInv_BPFunctionLibrary::GetArrayIndexFromRowAndColumnIndices(int32 InRowIndex, int32 InColumnIndex, int32 cols)
{
	return InRowIndex * cols + InColumnIndex;
}

//X-> = ColumnIndex , Y = RowIndex 
int32 UInv_BPFunctionLibrary::GetArrayIndexFromNormalizedPosition(const FIntPoint& Indices, int32 cols)
{
	return	Indices.Y * cols + Indices.X;
}

//Hence I change the name to match the normalize position, hence Y=row,  X=column
FIntPoint UInv_BPFunctionLibrary::GetColumnAndRowIndicesFromArrayIndex(int32 InArrayIndex, int32 cols)
{
	//no need to use FMath::Floor(InArrayIndex / rows), it will be truncated anyway
	return FIntPoint(InArrayIndex % cols, InArrayIndex / cols);
}

//the NormalizedPosition is (X,Y) , hence Row=Y, Col=X
FIntPoint UInv_BPFunctionLibrary::GetNormalizedPotionFromArrayIndex(int32 InArrayIndex, int32 cols)
{
	//              (            X       ,              Y      )
	return FIntPoint(InArrayIndex % cols, InArrayIndex / cols);
}

/*
FIntPoint UInv_BPFunctionLibrary::GetNormalizedPotionFromArrayIndex(int32 InArrayIndex, const FIntPoint& RowsAndColumns)
{
	return FIntPoint(InArrayIndex / RowsAndColumns.Y, InArrayIndex % RowsAndColumns.X);
}
*/

UInv_InventoryComponent* UInv_BPFunctionLibrary::GetInventoryComponentFromPC(APlayerController* OwningPC)
{
	AInv_PlayerController* Inv_PlayerController = Cast<AInv_PlayerController>(OwningPC);
	if (IsValid(Inv_PlayerController) == false) return nullptr;

	return Inv_PlayerController->InventoryComponent.IsValid() ?
		   Inv_PlayerController->InventoryComponent.Get() :
		   Inv_PlayerController->FindComponentByClass<UInv_InventoryComponent>();
}

//this is just a little mathematics challenge, this is my favorite: (Rider auto-fill, but I can do it myself)
bool UInv_BPFunctionLibrary::IsLocationWithinWidgetSize(const FVector2D& Origin,
	const FVector2D& PositionToCheck, const FVector2D& WidgetSize)
{
	bool bIsWithinX = PositionToCheck.X >= Origin.X && PositionToCheck.X <= Origin.X + WidgetSize.X;
	bool bIsWithinY = PositionToCheck.Y >= Origin.Y && PositionToCheck.Y <= Origin.Y + WidgetSize.Y;
	
	return bIsWithinX && bIsWithinY;
}

//FDeprecatedVector2DResult will be auto-converted to FVector2D
FVector2D UInv_BPFunctionLibrary::GetWidgetSize(const UWidget* Widget)
{
	return Widget->GetCachedGeometry().GetLocalSize();
}

void UInv_BPFunctionLibrary::OnItemHovered(APlayerController* PC, UItemData* HoveredItemData)
{
	if (IsValid(HoveredItemData) == false) return;
	
	UInv_InventoryComponent* InventoryComp = GetInventoryComponentFromPC(PC);
	if (IsValid(InventoryComp) == false) return;

	//it will be polymorphic, don't worry:
	TObjectPtr<UUW_Inv_InventoryBase> WBP_InventoryBase = InventoryComp->WBP_Inventory_Spacial;
	if (IsValid(WBP_InventoryBase) == false) return;

	//why we return in this case, but not return in hover case? it is in fact no need, because you can't unhover it when it is not first hovered at first place lol.
	//anyway this could be redudant too as WBP_ItemDescription would never be created when WBP_HoverItem is in act at first place, whatever lol.
	if (WBP_InventoryBase->HasHoverItemInAction()) return;
	
	//even if now it keeps the name but its pointer is " _Base" lol
	WBP_InventoryBase->OnItemHovered(HoveredItemData); 

}

void UInv_BPFunctionLibrary::OnItemUnhovered(APlayerController* PC)
{
	UInv_InventoryComponent* InventoryComp = GetInventoryComponentFromPC(PC);
	if (IsValid(InventoryComp) == false) return;
	
	TObjectPtr<UUW_Inv_InventoryBase> WBP_InventoryBase = InventoryComp->WBP_Inventory_Spacial;
	if (IsValid(WBP_InventoryBase) == false) return;
	
	//stephen didn't have this, but I add this so that it look "symmetric" lol
	if (WBP_InventoryBase->HasHoverItemInAction()) return;
	
	WBP_InventoryBase->OnItemUnhovered();
}


/*MY equivalent idea (imperfect because it only checks on on side lol), read for fun:
(1)
- get MousePositionInViewport
- make
"WBP_ItemDesc".X = MousePositionInViewport.X + GetDesiredSize().X
"WBP_ItemDesc".Y = MousePositionInViewport.Y

(2) adapt to apply "claim", not let bottom edge to go off "Viewport bottom"
; optionally "claim", not let right edge to go off "Viewport right" or "Viewport right - SomeArbitrary_X_Offset"

"WBP_ItemDesc".Y = Min(MousePositionInViewport.Y, ViewportSize.Y - GetDesiredSize().Y)
"WBP_ItemDesc"X = Min(MousePositionInViewport.Y + GetDesiredSize().X, ViewportSize.X - GetDesiredSize().X - SomeArbitrary_X_Offset)  */
FVector2D UInv_BPFunctionLibrary::GetClampedMousePosition(const FVector2D& MousePosition, const FVector2D& BoundarySize,
	const FVector2D& WidgetSize)
{
//step1: set ClampedMousePosition = MousePosition by default (only clamp in X and Y directly separately if needed)
	FVector2D ClampedMousePosition = MousePosition;

//step2: clamp in X direction if needed (we in fact need to clamp on both left and right sides, the order does make a slightly different)
	if (ClampedMousePosition.X + WidgetSize.X > BoundarySize.X)
	{
		ClampedMousePosition.X = BoundarySize.X - WidgetSize.X;
	}
	if (ClampedMousePosition.X < 0)
	{
		ClampedMousePosition.X = 0;
	}
	
//step3: clamp in Y direction if needed (we in fact need to clamp on both bottom and top sides, the order does make a slightly different)
	if (ClampedMousePosition.Y + WidgetSize.Y > BoundarySize.Y)
	{
		ClampedMousePosition.Y = BoundarySize.Y - WidgetSize.Y;
	}
	if (ClampedMousePosition.Y < 0)
	{
		ClampedMousePosition.Y = 0;
	}

	return ClampedMousePosition;
}
