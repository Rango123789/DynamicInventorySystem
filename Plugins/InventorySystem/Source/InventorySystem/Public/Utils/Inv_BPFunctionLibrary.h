// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inv_BPFunctionLibrary.generated.h"

class UInv_InventoryComponent;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UInv_BPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//they're the same thing but I name it differently so that you understand it
	static int32 GetArrayIndexFromRowAndColumnIndices(int32 InRowIndex, int32 InColumnIndex, int32 cols);
	static int32 GetArrayIndexFromNormalizedPosition(const FIntPoint& Indices, int32 cols);

	//they're the same thing but I name it differently so that you understand it
	static FIntPoint GetColumnAndRowIndicesFromArrayIndex(int32 InArrayIndex, int32 cols);
	static FIntPoint GetNormalizedPotionFromArrayIndex(int32 InArrayIndex, int32 cols);
	
	static UInv_InventoryComponent* GetInventoryComponentFromPC(APlayerController* OwningPC);

	//ME: I make it clear that I want ForEachFunc to be a specific signature (I prefer this one)
	template <typename T>
	static void ForEachInGridDimensions(const TArray<T>& SourceArray, int32 StartIndex, const FIntPoint& GridDimensions, const int32& InColumns, TFunction<void(T)> ForEachFunc);
	
	//STEPHEN: as long as you pass in a function/lambda that make Function(Array[TileIndex] make sense then it will work (I don't like this one, it is prone to error in both definition and using)
	template<typename T, typename Func>
	static void ForEach2D(TArray<T>& Array, int32 Index, const FIntPoint& Range2D, int32 Columns, const Func& Function);
};

template <typename T>
void UInv_BPFunctionLibrary::ForEachInGridDimensions(const TArray<T>& SourceArray, int32 StartIndex, const FIntPoint& GridDimensions, const int32& InColumns, TFunction<void(T)> ForEachFunc)
{
	for (int32 j = 0; j < GridDimensions.Y; j++)
	{
		for (int32 i = StartIndex; i < StartIndex + GridDimensions.X; i++)
		{
			int32 CurrentArrayIndex = i + j * InColumns;
			
			if (SourceArray.IsValidIndex(CurrentArrayIndex ))
			{
				ForEachFunc(SourceArray[CurrentArrayIndex]);
			}
		}
	}
	/*
	for (int32 i = StartIndex; i < StartIndex + GridDimensions.X; i++)
	{
		for (int32 j = 0; j < GridDimensions.Y; j++)
		{
			int32 CurrentArrayIndex = i + j * InColumns;
			
			if (SourceArray.IsValidIndex(CurrentArrayIndex ))
			{
				ForEachFunc(SourceArray[CurrentArrayIndex]);
			}
		}
	}
	 */
}

template<typename T, typename Func>
void UInv_BPFunctionLibrary::ForEach2D(TArray<T>& Array, int32 Index, const FIntPoint& Range2D, int32 Columns, const Func& Function)
{
	//basically the same idea, but he start from "0,0" and then must " + (i,j")". Where I start from "i,j" directly hence I don't need to + it
	for (int32 j = 0; j < Range2D.Y; ++j)
	{
		for (int32 i = 0; i < Range2D.X; ++i)
		{
			const FIntPoint Coordinates =
				UInv_BPFunctionLibrary::GetNormalizedPotionFromArrayIndex(Index, Columns) + FIntPoint(i, j);

			const int32 TileIndex =
				UInv_BPFunctionLibrary::GetArrayIndexFromNormalizedPosition(Coordinates, Columns);

			if (Array.IsValidIndex(TileIndex))
			{
				Function(Array[TileIndex]);
			}
		}
	}
}

