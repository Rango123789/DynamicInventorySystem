// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/Inv_BPFunctionLibrary.h"

#include "ActorComponent/Inv_InventoryComponent.h"
#include "Player/Inv_PlayerController.h"

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
