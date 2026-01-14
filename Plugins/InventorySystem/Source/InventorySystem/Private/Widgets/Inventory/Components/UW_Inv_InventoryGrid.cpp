// Fill out your copyright notice in the Description page of Project Settings.
#include "Widgets/Inventory/Components/UW_Inv_InventoryGrid.h"
#include "InventorySystem.h"
#include "ActorComponent/Inv_InventoryComponent.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryTags/InventoryTags.h"
#include "Items/ItemData.h"
#include "Utils/Inv_BPFunctionLibrary.h"
#include "Widgets/Inventory/Components/UW_Inv_InventoryGridSlot.h"
#include "Widgets/Inventory/Components/UW_Inv_SlottedItem.h"
#include "Items/ItemManifest.h"
#include "Widgets/Inventory/Components/HoverItem/UW_Inv_HoverItem.h"
#include "Widgets/ItemPopup/UW_Inv_ItemPopup.h"

void UUW_Inv_InventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//this should be done before bound lol
	ConstructGridSlots();

	//we cache it should the need arise:
	InventoryComponent = UInv_BPFunctionLibrary::GetInventoryComponentFromPC(GetOwningPlayer());
	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::OnItemAddedCallback);
		InventoryComponent->OnStacksAdded.AddDynamic(this, &ThisClass::OnStacksAddedCallback);
	}		
}

void UUW_Inv_InventoryGrid::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	/*UPDATE1: because even when HoverItem is NOT valid, we may still want to highlight the single hovered GridSlot, so we must still let it through - hence remove "if (IsValid(WBP_HoverItem))"
	 *UPDATE2: if mouse just exit canvas - we do unhighlight ONCE - only happen for one frame, but then next frame it will no longer be true*/
		//these are both in local space (origin = Viewport top-left), hence DesiredSize/Geometry::GetLocalSize() is appropriate
	FVector2D  CanvasPositionInViewport=  GetWidgetPositionInViewport(CanvasPanel_GridSlots);
	FVector2D  MousePositionInViewPort = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);

		//only true for single frame, and the next frame it will continue to update TileParams
	if (MouseExitedCanvas(CanvasPositionInViewport, MousePositionInViewPort)) //this also UpdateMouseInCanvasBooleans
	{
		/*UnhighlightSlot() -- You can do this in MouseExitedCanvas itself. But I like to do it here for the sake of overview.
		 *Stephen do it inside MouseExitedCanvas itself, but I do it here*/
		UnHighlightSlots(LastHighlightedIndex, LastHighlightedGridDimensions);

		return; 
	}

	if (bMouseInCanvas)
	{
		UpdateTileParameters(CanvasPositionInViewport, MousePositionInViewPort);

		if (IsValid(WBP_HoverItem))
		{
			UpdateSpaceQueryResult();
			/* HighlightSlot() if QueryResult.bHasSpace = true, UnhighlightSlot() if QueryResult.bHasSpace = false (this helps to guarantee that at the time the DropIndex get out of bounds (invalid DropIndex is stored currently) where Mouse is still in canvas we Unhighlight it immediately, even before Mouse is out of canvas - and when the mouse is out of canvas we stop updating everything - which is perfect!)
			 * FUNNY: this could make the Unhighlight() above REDUNDANT lol? hell no, we're to implement "highlight a single hovered empty GridSlot" even if WBP_HoverItem is NOT valid, hence it is still needed :D :D
			 * Stephen do it inside UpdateSpaceQueryResult itself, but I do it here
			 */
			if (SpaceQueryResult.bHasSpace)
			{
				//if it gets here, DropIndex must be valid. WBP_HoverItem just valid right outside!
				HighlightSlots(DropIndex, WBP_HoverItem->GridDimensions);			
			} else
			{
				/*using DropIndex is is a mistake, DropIndex in the case "SpaceQueryResult.bHasSpace=false" could be "-1, -2 ..." that is the whole reason we cache LastHighlightedIndex right inside HighlightSlots (that is indeed "the last valid DropIndex" :D :D)
				//this not only unhighlight the last valid drop ones, but also re-set "fake one" (TopLeftPreoccupiedIndex will be the fake one and it will help to reset it from  "GrayedOut" back to "occupied")*/
				UnHighlightSlots(LastHighlightedIndex, LastHighlightedGridDimensions);
			}

			/*set GrayedOut Brush for PreoccupiedItemData if valid:
			//TRICK: because we don't highlight when bHasSpace=false as preoccupied item in the way(even if DropIndex is still valid and not negative) and we want to grey out the preoccupied grid, so we use the trick: "we fake TopLeftPreoccupiedIndex = LastValidDropIndex" so that it will naturally help to re-set it back to "occupied" from "grayed out" without needing to create extra LastGrayOutPreoccupiedIndex at all!*/
			if (SpaceQueryResult.PreoccupiedItemData.IsValid() &&
				GridSlots.IsValidIndex(SpaceQueryResult.TopLeftSlotIndexOfPreoccupiedItem))
			{
			//funny stephen does these fake assignments inside SetSlotStateForAllSlotsInGridSize itself, wherever lol
				//we fake (it naturally help reset to occupied if we move mouse away from the preoccupied item).
				LastHighlightedIndex =	SpaceQueryResult.TopLeftSlotIndexOfPreoccupiedItem;
				//Warning: this time it must be PreoccupiedItemData:::GridDimensions , not WBP_HoverItem:::GridDimensions
				LastHighlightedGridDimensions = SpaceQueryResult.PreoccupiedItemData->GetGridDimensions();
				//Warning: we try to change PreoccupiedGrid, not WBP_HoverItem (not even highlight anything in this case)
				SetSlotStateForAllSlotsInGridSize(LastHighlightedIndex, LastHighlightedGridDimensions, ESlotState::GrayedOut);
			}
		}
	}
	
}

void UUW_Inv_InventoryGrid::HighlightSlots(int32 StartIndex, const FIntPoint& InGridDimensions)
{
//step0: REDUNDANT at least for now (you never get upto this function if it is false at first place)
	if (bMouseInCanvas == false) return; 

//step1: we must unhighlight the LastHighlightedIndex->GridSize (the last valid DropIndex, that could be just next to the newest valid one as you move Mouse to new spot) before Highlight the new DropIndex->GridSize = this is key to my puzzle at first place: "do we need to change slot states along as we move the WBP_HoverItem long?"  --- hell yeah! what a smart trick:
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedGridDimensions);
	
//step2: highlight it all (could be redundant on the preoccupied slots, but who cares lol)
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>(GridSlots, StartIndex, InGridDimensions, columns,
	[&](UUW_Inv_InventoryGridSlot* SubGridSlot)
	{
		SubGridSlot->SetSlotStateAndBrush(ESlotState::Occupied);
	});
	
//step3: at the moment you highlight DropIndex->GridSize, you want to cache it (so that you know what to unhighlight later), so unlike what I expected, we don't need to store a separate HoveredHighlightedGridSlots, we need to know where it start and GrisSize only = so basically equivalent! yeah. That's why we do have WBP_SlottedItem --> WBP_HoverItem::SlotDimensions at first place
	LastHighlightedIndex = StartIndex;
	LastHighlightedGridDimensions = InGridDimensions;
}

//we don't unhighlight the preoccupied slots (bAvailable=false or OwningItemData.IsValid=true)
void UUW_Inv_InventoryGrid::UnHighlightSlots(int StartIndex, const FIntPoint& InGridDimensions)
{
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>(GridSlots, StartIndex, InGridDimensions, columns,
		[&](UUW_Inv_InventoryGridSlot* SubGridSlot)
		{
			if (SubGridSlot->bAvailable) //or OwningItemDataIsValid = false
			{
				SubGridSlot->SetSlotStateAndBrush(ESlotState::Unoccupied);
			}
			//this could be redundant, it should be "highlighted" at first place?
			//UPDATE: well we use it to to re-set "GrayedOutPreoccupiedGrid"
			else 
			{
				SubGridSlot->SetSlotStateAndBrush(ESlotState::Occupied); 
			}
		}
	);
}

void UUW_Inv_InventoryGrid::SetSlotStateForAllSlotsInGridSize(const int32& StartIndex,
	const FIntPoint& InGridDimensions, const ESlotState& InSlotState)
{
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>(GridSlots, StartIndex, InGridDimensions, columns,
		[&](UUW_Inv_InventoryGridSlot* SubGridSlot)
		{
			SubGridSlot->SetSlotStateAndBrush(InSlotState);
		}
	);
}

UUserWidget* UUW_Inv_InventoryGrid::GetVisibleCursorWidget()
{
	//stephen decide this:
	if (IsValid(GetOwningPlayer()) == false) return nullptr;
	
	if (IsValid(WBP_Cursor_Visible) == false)
	{
		WBP_Cursor_Visible = CreateWidget<UUserWidget>(GetOwningPlayer(), Cursor_Visible_Class);
	}

	return WBP_Cursor_Visible;
}

UUserWidget* UUW_Inv_InventoryGrid::GetHiddenCursorWidget()
{
	//stephen decide this:
	if (IsValid(GetOwningPlayer()) == false) return nullptr;
	
	if (IsValid(WBP_Cursor_Hidden) == false)
	{
		WBP_Cursor_Hidden = CreateWidget<UUserWidget>(GetOwningPlayer(), Cursor_Hidden_Class);
	}

	return WBP_Cursor_Hidden;
}

void UUW_Inv_InventoryGrid::ShowVisibleCursorWidget()
{
	if (IsValid(GetOwningPlayer()) == false) return;
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Type::Default, GetVisibleCursorWidget()); 
}

void UUW_Inv_InventoryGrid::ShowHiddenCursorWidget()
{
	if (IsValid(GetOwningPlayer()) == false) return;
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Type::Default, GetHiddenCursorWidget()); 
}

//this is different from "Is[GridSize]WithinBounds", this check the MousePosition against CanvasSize 
bool UUW_Inv_InventoryGrid::MouseExitedCanvas(const FVector2D& CanvasPositionInViewport,
	const FVector2D& MousePositionInViewPort)
{
//step1: update bMouseInCanvas and bLastMouseInCanvas (can factorize into: UpdateMouseInCanvasBooleans). Note what the MouseExitedCanvas won't return a correct result if we didn't do UpdateMouseInCanvasBooleans yet for this frame, hence I device to directly do it HERE in the right order with the hosting function.
	//cache before change
	bLastMouseInCanvas = bMouseInCanvas;

	//change
		//you can directly create IsMouseWithinCanvas if you want, but anyway it could be a GENERIC re-usable function
		//CanvasPanel_GridSlots->GetDesiredSize() will also work because current there is no parent constraints on Canvas (but risky anyway)
	FVector2D CanvasSize = CanvasPanel_GridSlots->GetCachedGeometry().GetLocalSize(); 
	bMouseInCanvas = UInv_BPFunctionLibrary::IsLocationWithinWidgetSize(CanvasPositionInViewport,MousePositionInViewPort, CanvasSize);

//step2: decide the result
	if (bMouseInCanvas == false && bLastMouseInCanvas == true)
	{
		return true; //mouse just exit canvas
	}
	
	return false;
}

void UUW_Inv_InventoryGrid::UpdateTileParameters(const FVector2D& CanvasPositionInViewPort,
	const FVector2D& MousePositionInViewPort)
{
//if mouse not on canvas return: this is different from MouseExitedCanvas ( !bInIsCanvas && bLastIsInCanvas) = but at least you should set TileParameters to "FTileParams{}" and SpaceQueryResult to "FSpaceQueryResult{}" right? - well we can do it in MouseExitedCanvas() as well
	
//last frame = this frame first thing:
	LastTileParameters = TileParameters;

	//Calculate HoveredNormalizedPosition (Coordinates with Unit=GridSize):
	TileParameters.HoveredNormalizedPosition =  CalculateNormalizedPositionOfHoveredGridSlot(CanvasPositionInViewPort, MousePositionInViewPort);
	TileParameters.HoveredIndex = UInv_BPFunctionLibrary::GetArrayIndexFromNormalizedPosition(TileParameters.HoveredNormalizedPosition, columns);
	TileParameters.TileQuadrant = CalculateTileQuadrant(CanvasPositionInViewPort, MousePositionInViewPort);
	
	//Handle SlotStates of pertinent GridSlots: --UPDATE: I move it to GLOBAL place for readability and organization
		//UpdateSpaceQueryResult();
}

//you may want to name it "OnTileParametersUpdate" if you want to, because it is literally called after TileParameters update
void UUW_Inv_InventoryGrid::UpdateSpaceQueryResult()
{
//step0: [UPDATE] when only stop it when we need to use WBP_HoverItem, not global skip any more
	if (IsValid(WBP_HoverItem) == false) return;
	
//step1: Get HoverItem GridSize
	FIntPoint GridDimensions = WBP_HoverItem->GridDimensions; //make sure to check it outside at least
	
//step2: Calculating the starting GridSlot for highlighting (not necessarily match the HoveredGridSlot at all, but depending on which exactly of 4 parts the mouse is on - that's the whole reason we have FTileParameters::ETileQuadrant) = I think it will be easy lol?
	//i reckon that they can be INVALID (return and get "negative" Index/Position lol). So my suggestion is to check the DropIndex before use it
	FIntPoint  DropNormalizedPosition = CalculateDropNormalizedPosition(GridDimensions);

	/*this is up to you, you may want to check if it is valid index. and then decide if it is NOT valid you will either:
	(1) don't assign it at all, hence it keeps the last valid DropIndex
	(2) assign it to that invalid DropIndex, and we have no way to what is the the last valid DropIndex when the mouse is still on canvas, but the starting DropIndex is already out of bounds = stephen current does this way 
	*/
	DropIndex = UInv_BPFunctionLibrary::GetArrayIndexFromNormalizedPosition(DropNormalizedPosition, columns);

//step3: checking hover position (I guess starting from the starting GridSlot for highlighting, not necessarily the HoveredGridSlot ) - could be very similar to what we did, however it has some differences
	SpaceQueryResult = GetSpaceQueryResult(DropIndex, GridDimensions);
}

FSpaceQueryResult UUW_Inv_InventoryGrid::GetSpaceQueryResult(int32 InDropIndex, FIntPoint& GridDimensions)
{
/*Take a look at IsThisSubGridSlotQualified chain as a reference, and you see that you can re-use many the same functions:
	//1. SubIndex claimed?
	= this is irrelevant here, because we only need to check on a single DropGridSlot/DropIndex here!
	
	//2. Has valid item?
	= OKAY!

	//3. Does the current GridSlot is the UpperLeftSlot of this SubGridSlot
	= irrelevant because we move the EXISTING one, not place a to-be-in one (not offically exist at the moment yet)

	//4. Is this a stackable item?

	//5. Is this preoccupied item the same type as the item we're trying to add?

	//6. Is this slot at the max stack size already?
 */
	FSpaceQueryResult QueryResult{}; //make sure its default value is appropriate
	
	if (GridSlots.IsValidIndex(InDropIndex) == false) return QueryResult; //UPDATE: very important!
	
	UUW_Inv_InventoryGridSlot* DropGridSlot = GridSlots[InDropIndex];
	
//1. Is in the Grid bounds?
	if (IsOutOfBounds(DropGridSlot, GridDimensions)) return QueryResult;

/*You can merge 2. and 3. like Stephen if you want (because both need ForEach2D), but anyway I like to separate them for readability:
	Result.bHasSpace = true;
	
	// If more than one of the indices is occupied with the same item, we need to see if they all have the same upper left index.
	TSet<int32> OccupiedUpperLeftIndices;
	UInv_InventoryStatics::ForEach2D(GridSlots, UInv_WidgetUtils::GetIndexFromPosition(Position, Columns), Dimensions, Columns, [&](const UInv_GridSlot* GridSlot)
	{
		if (GridSlot->GetInventoryItem().IsValid())
		{
			OccupiedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
			Result.bHasSpace = false;
		}
	});

	// any items in the way?
	// if so, is there only one item in the way? (can we swap?)
	if (OccupiedUpperLeftIndices.Num() == 1) // single item at position - it's valid for swapping/combining
	{
		const int32 Index = *OccupiedUpperLeftIndices.CreateConstIterator();
		Result.ValidItem = GridSlots[Index]->GetInventoryItem();
		Result.UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	}

	return Result;
*/
//2. Is any item in the way? if no, set QueryResult.bHasSpace = true and return (we must use ForEach2D but this time we use IsGridSlotPreoccupied instead IsThisSubGridSlotQualified at GLOBAL level)
	if (HasRoomForGridSizeAtThisSlot(GridDimensions, DropGridSlot))
	{
		QueryResult.bHasSpace = true;
		return QueryResult;
	}

	//if any item in the way, set ::bHasSpace to false and continue (NOT return)
	QueryResult.bHasSpace = false; //no need the default value is false already lol, but I just make it clear
	
//3. if yes, is there only one item in the way (I.E one or more grid slots with the same UpperLeftIndex)? (can we swap or combine?)
	TSet<int32> TopLeftIndicesOfPreoccupiedItems; //it could be "1" or more ("0" is included after 2. already)
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>( GridSlots,
		DropGridSlot->GridSlotIndex, GridDimensions, columns,
		[&](UUW_Inv_InventoryGridSlot* SubGridSlot)
		{
			if (IsGridSlotPreoccupied(SubGridSlot))
			{
				TopLeftIndicesOfPreoccupiedItems.Add(SubGridSlot->UpperLeftIndex); //NOT "SubGridSlot->GridSlotIndex"
			}
		}
	);

	//if this TSet (that only accept unique values) contain a single element, then it meets our requirement: "only one ItemData type in the way and we consider for swapping"
	if (TopLeftIndicesOfPreoccupiedItems.Num() == 1)
	{
		int32 TopLeftPreoccupiedIndex = *TopLeftIndicesOfPreoccupiedItems.begin();
		
		QueryResult.bHasSpace = false; //no need we just did it, I just want to make it clear
		QueryResult.TopLeftSlotIndexOfPreoccupiedItem = TopLeftPreoccupiedIndex; // or * __ .CreateConstIterator()
		QueryResult.PreoccupiedItemData = GridSlots[TopLeftPreoccupiedIndex]->OwningItemData.Get();
		return QueryResult;
	}
	//if this TSet contain 2+ element (because the case 0 is excluded), we don't care simply return "QueryResult::bHasSpace = false, Preoccupied=nullptr, TopLeftIndex=INDEX_NONE" ()basically the default QueryResult lol 
	else
	{
		QueryResult.bHasSpace = false; //no need we just did it, I just want to make it clear
		return QueryResult;
	}
	
//4. is the stackable one and of the same type? (can we merge?) = Stephen didn't consider this case currently lol
//UPDATE: stephen didn't decide it is "swapping or combining" here (meaning he didn't create an extra bool to tell it). It will be decided externally (simply because we did NOT pass in DraggingItemData here, we have nothing to compare) 
	
}

FIntPoint UUW_Inv_InventoryGrid::CalculateDropNormalizedPosition(const FIntPoint& GridDimensions)
{
/*
	which GridSlot is hovered on? = Params.Index 
	which part of it is hovered on? = Params.TileQuadrant
---WRONG:---
	if top-left it is the starting is HoveredGridSlot itself
	if top-right then it must be HoverGridSlot::Index + 1
	if bottom-left then  HoverGridSlot::Index + columns
	if bottom-right then HoverGridSlot::Index + columns + 1
	so I don't see any difficulty here lol?
	well it is NOT correct lol, we must consider the case GridSize is 2*3, 3*2, 3*3 and more lol
	hence the result must involve "GridSize" as well (HENCE calculate in step1 lol)

---BETTER DRAW A PICTURE TO SEE!---
	STUPID APPROACH: use ArrayIndex as the base
	SMART APPROACH: use RowIndex and ColumnIndex as the base (and convert it to ArrayIndex in the end or next step)
	I believe I can figure it out my, it is not something technical, it is just a normal challenge :D :D
	GridSize.X and GridSize.Y % 2 = 1 or 0 also affect the result as well I guess? = this is true, Stephen will consider this lol
	all we need is to the STARTING one

---PERFECT code:
	The code below is the perfect after all the years.
	You can in fact check one by one case yourself, but why re-inventing  the wheel  (you must have 4 big cases: old-old, old-even, even-old, even-even , and in each each cases you handle it separately - you will succeed trust me - it just takes time)
	So just see the picture I draw and then follow it.
*/

//step0: ready stuff
	FIntPoint StartingPosition{-1,-1}; //in case it goes know we know.
	
	int32 HoveredIndex  = TileParameters.HoveredIndex; //not used, as I said above we follow the "SMART" approach
	int32 HoveredColumn = TileParameters.HoveredNormalizedPosition.X;
	int32 HoveredRow    = TileParameters.HoveredNormalizedPosition.Y;
	
//step1:
	int32 HasEvenWidth = GridDimensions.X % 2 == 0  ? 1 : 0;    // = GridDimensions.X % 2 directly is in correct, it reverses the wanted result
	int32 HasEvenHeight = GridDimensions.Y % 2 == 0 ? 1 : 0;;  // unless you change the name into HasOddWidth lol

//step2: the FOUR BIG CASES: (you can use switch, but I like to separate them by if for readability) -- unlike my idea, BIG cases separated by odd-odd, odd-even, even-odd, even-even. Here Stephen separate by which part of the HoveredSlot we're exactly on. Both ways come to Paris lol:
	//there is no rule lol, see the PICTURE and figure it out lol:
	if (TileParameters.TileQuadrant == ETileQuadrant::TopLeft)
	{
		/*update we can't not use FMath::FloorToInt32 with "GridDimensions.X / 2.f" here 
		StartingPosition.X = HoveredColumn - FMath::FloorToInt32<float>(GridDimensions.X / 2);
		StartingPosition.Y = HoveredRow - FMath::FloorToInt32<float>(GridDimensions.Y / 2);*/
		StartingPosition.X = HoveredColumn - (int32)(GridDimensions.X / 2); //even if you don't put (int32) it will auto-convert and floor down
		StartingPosition.Y = HoveredRow - (GridDimensions.Y / 2);
	}

	//there is no rule lol, see the PICTURE and figure it out lol:
	if (TileParameters.TileQuadrant == ETileQuadrant::TopRight)
	{
		StartingPosition.X = HoveredColumn - (GridDimensions.X / 2) + HasEvenWidth;
		StartingPosition.Y = HoveredRow    - (GridDimensions.Y / 2);
	}

	//there is no rule lol, see the PICTURE and figure it out lol:
	if (TileParameters.TileQuadrant == ETileQuadrant::BottomLeft)
	{
		StartingPosition.X = HoveredColumn - (GridDimensions.X / 2);
		StartingPosition.Y = HoveredRow    - (GridDimensions.Y / 2) + HasEvenHeight;
	}

	//there is no rule lol, see the PICTURE and figure it out lol:
	if (TileParameters.TileQuadrant == ETileQuadrant::BottomRight)
	{
		StartingPosition.X = HoveredColumn - (GridDimensions.X / 2) + HasEvenWidth;
		StartingPosition.Y = HoveredRow    - (GridDimensions.Y / 2) + HasEvenHeight;
	}

	return StartingPosition;
}

ETileQuadrant UUW_Inv_InventoryGrid::CalculateTileQuadrant(const FVector2D& CanvasPositionInViewPort,
                                                           const FVector2D& MousePositionInViewPort)
{
/* it is very easy, very similar to ColumnIndex = ArrayIndex % columns
 * we simply compare "the remainder in float" with "TileSize" (in both direction)
 * and we will know whether the mouse is in top-left, top-right, bottom-left or bottom-right exactly! yeah
 * so the question is: is where any helper that help use to get "the remainder in float" of "A float/B float"? (again unlike remainder in int32 we use A % B and done) = luckily we have! hell yeah!
 * FMod HERE = "Floating-point" modulus! (not "mod" in mathematics back in high school lol)
 */
	int32 DeltaX = MousePositionInViewPort.X - CanvasPositionInViewPort.X;
	int32 DeltaY = MousePositionInViewPort.Y - CanvasPositionInViewPort.Y;

	float RemainderX = FMath::Fmod(DeltaX, GridSlotSize);
	float RemainderY = FMath::Fmod(DeltaY, GridSlotSize);


	//this clearly better code:
	bool bIsTop = RemainderY <= GridSlotSize / 2.0f;
	bool bIsLeft = RemainderX <= GridSlotSize / 2.0f;

	if (bIsTop && bIsLeft) return ETileQuadrant::TopLeft;
	if (bIsTop && !bIsLeft) return ETileQuadrant::TopRight;
	if (!bIsTop && bIsLeft) return ETileQuadrant::BottomLeft;
	if (!bIsTop && !bIsLeft) return ETileQuadrant::BottomRight;
	
	/*Rider auto-complete lol: (which one is "=" is upto you, but shouldn't be both)
	bool bIsTopLeft = (RemainderX <= GridSlotSize / 2.0f) && (RemainderY <= GridSlotSize / 2.0f);
	bool bIsTopRight = (RemainderX > GridSlotSize / 2.0f) && (RemainderY <= GridSlotSize / 2.0f);
	bool bIsBottomLeft = (RemainderX <= GridSlotSize / 2.0f) && (RemainderY > GridSlotSize / 2.0f);
	bool bIsBottomRight = (RemainderX > GridSlotSize / 2.0f) && (RemainderY > GridSlotSize / 2.0f);

	if (bIsTopLeft) return ETileQuadrant::TopLeft;
	if (bIsTopRight) return ETileQuadrant::TopRight;
	if (bIsBottomLeft) return ETileQuadrant::BottomLeft;
	if (bIsBottomRight) return ETileQuadrant::BottomRight;
	*/
	
	return ETileQuadrant();
}

FIntPoint UUW_Inv_InventoryGrid::CalculateNormalizedPositionOfHoveredGridSlot(const FVector2D& CanvasPositionInViewPort,
	const FVector2D& MousePositionInViewPort)
{
/*They're current in local space (where TileSize (if not constrained) and Geometry.GetLocalSize() is appropriate to be used) with origin is viewport top-left
 *But you know what, the origin doesn't really matter at all if we compare 2 points as long as they're relative to the same origin (where doesn't matter) but in the same unit space (which they're)
 */
	int32 DeltaX = MousePositionInViewPort.X - CanvasPositionInViewPort.X;
	int32 DeltaY = MousePositionInViewPort.Y - CanvasPositionInViewPort.Y;

	//OPTION1: it will be auto-converted and rounded down
		//return FIntPoint( DeltaX / GridSlotSize , DeltaY / GridSlotSize);
	//OPTION2: (DO NOT use "RoundToInt" - also to int32 but not 5-5. not floor)

	if ( GridSlots.IsValidIndex(0))
	{
		float LocalSizeX = GridSlots[0]->GetCachedGeometry().GetLocalSize().X;
		float LocalSizeY = GridSlots[0]->GetCachedGeometry().GetLocalSize().Y;
		return FIntPoint(
			FMath::FloorToInt32(DeltaX /  LocalSizeX),
			FMath::FloorToInt32(DeltaY / LocalSizeY)
		);
	}
//this time use FloorToInt32 is safe here
	return FIntPoint(
				FMath::FloorToInt32(DeltaX / GridSlotSize),
				FMath::FloorToInt32(DeltaY / GridSlotSize)
			);
}

FVector2D UUW_Inv_InventoryGrid::GetWidgetPositionInViewport(UWidget* InWidget)
{
	/* testing
	FGeometry WidgetGeometry = InWidget->GetCachedGeometry();
	FVector2D LocalTopLeftPosition = USlateBlueprintLibrary::GetLocalTopLeft(WidgetGeometry);
	DebugHelpers::Print(FString("LocalTopLeftPosition: ") + LocalTopLeftPosition.ToString()); //same result as "WidgetGeometry.Position"

	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::LocalToViewport(this, WidgetGeometry, LocalTopLeftPosition, PixelPosition, ViewportPosition);

	DebugHelpers::Print(FString("GetAbsolutePosition(): ") + WidgetGeometry.GetAbsolutePosition().ToString()); //the actual draw&render size (hence smaller)
	DebugHelpers::Print(FString("GetLocalPosition(): ") + WidgetGeometry.Position.ToString());  //before DPI relative it its direct parent
	DebugHelpers::Print(FString("PixelPosition: ") + PixelPosition.ToString());                 
	DebugHelpers::Print(FString("ViewportPosition: ") + ViewportPosition.ToString());           //bigger like DesiredSize before DPI, just like in local space but move the origin to Viewport top-left, instead of direct-parent top-left
	
	if (GridSlots.IsValidIndex(0))
	{
		FGeometry SlotGeometry = GridSlots[0]->GetCachedGeometry();
		DebugHelpers::Print(FString("LocalSize: ") + SlotGeometry.GetLocalSize().ToString()); // smaller
		DebugHelpers::Print(FString("AbsoluteSize: ") + SlotGeometry.GetAbsoluteSize().ToString()); //65-65
	}
	*/
	FGeometry WidgetGeometry = InWidget->GetCachedGeometry();
	FVector2D LocalTopLeftPosition = USlateBlueprintLibrary::GetLocalTopLeft(WidgetGeometry); //or WidgetGeometry.Position (they're the same I test it)

	FVector2D PixelPosition;
	FVector2D ViewportPosition; //you can name it "local viewport position" (because it is in fact in "local space" with parent = Viewport)
	USlateBlueprintLibrary::LocalToViewport(this, WidgetGeometry, LocalTopLeftPosition, PixelPosition, ViewportPosition);
	
	return ViewportPosition;
}

bool UUW_Inv_InventoryGrid::DoesItemMatchGridCategory(UItemData* ItemData)
{
	if (IsValid(ItemData) == false) return false;
	
	FItemManifest ItemManifest = ItemData->GetItemManifest();
	return ItemManifest.ItemCategory == ItemCategory;
}

//GLOBALLY we only need this one. Hence this one needs to be PUBLIC, the rest can be private if you want to:
FInventoryAvailabilityInfo UUW_Inv_InventoryGrid::GetAvailabilityInfoForItem(UInv_ItemComponent* ItemComponent)
{
	return GetAvailabilityInfoForItem(ItemComponent->SourceItemManifest);
}

//but LOCALLY we need this one. because "OnItemAddedCallback(UItemData* ItemData)"
FInventoryAvailabilityInfo UUW_Inv_InventoryGrid::GetAvailabilityInfoForItem(UItemData* InItemData)
{
	return GetAvailabilityInfoForItem(InItemData->GetItemManifest());
}

//this version re-use by both of overloads above, we will use "InItemManifest" against this WBP_InventoryGrid to decide the return "AvailabilityInfo" (hence ultimately the 2 overloads above)
//ultimately, the manifest that contains the interesting information we need to check for rooms such as the grid fragment how many spaces in our grid should we take up and other fragments that we add as well
FInventoryAvailabilityInfo UUW_Inv_InventoryGrid::GetAvailabilityInfoForItem(const FItemManifest& InItemManifest)
{
	/*THE PLAN:
		// Determine if the item is stackable.
    	// Determine how many stacks to add.
    	// For each Grid Slot:
    		// If we don't have anymore to fill, break out of the loop early.
    		// Is this index claimed yet?  = what do you mean? at starting index?
    		
    		// Can the item fit here? (i.e. is it out of grid bounds?)
    		
    		// Is there room at this index? (i.e. are there other items in the way?)
    		
    		// Check any other important conditions - ForEach2D over a 2D range
    			// SubIndex claimed? = what do you mean? for each in  the group within GridSize?

    			// Has valid item (pre-occupied by any owning ItemData)?
    			
    			// If so, is this a stackable item?
    			
    			// Is this preoccupied item the same type as the item we're trying to add?

				// If stackable, does it belongs to an UpperLeftSlot
    			// Is this slot at the max stack size already?
    			
    		// How much to fill?
    		// Update the amount left to fill
    	// How much is the Remainder?
	*/
	FInventoryAvailabilityInfo AvailabilityInfo{};

	// Determine if the item is stackable. = easy
	const FItemFragment_Stackable* ItemFragment_Stackable = InItemManifest.GetItemFragmentByType<FItemFragment_Stackable>();
	bool bStackable = (ItemFragment_Stackable != nullptr); // bool = ptr also valid, ptr will be auto-converted to "bool"
	
	// Determine how many stacks to add.
		//stephen: "AmountToFill" = could be misleading , me: "Total[Room/Amount][Left]ToFill"
	const int32 MaxStackSize = bStackable? ItemFragment_Stackable->MaxStackSize : 1; //so if it is non-stackable, we consider it is "1", not "0"
	int32 SourceAmount = bStackable? ItemFragment_Stackable->StackCount : 1; //the same logic here (even if it can be modified to "0" for some purpose at sub-chain as a signal for non-stackable item in the end - up to you anyway)

	/*You can also use TArray with .AddUnique, but since we don't care about order nor need to use it, TSet is the perfect optimization!
	 * No point to store as member, we only need it for temporary need
	 * This will store/append several PotentialClaimedIndices if pass the check
	 */
	TSet<int32> ClaimedSlotIndices{};

// For each Grid Slot:
	for (auto& GridSlot : GridSlots)
	{
    // If we don't have anymore to fill, break out of the loop early.
		if (SourceAmount == 0) break;
		
    /* Is "this potential STARTING index" claimed yet?
     (1) "this index" = the current GridSlot::SlotIndex (in the loop)
     - looks like "a potential starting slot" in SlotInfo::SlotIndex for non-stackable item? well it is irrelevant anyway because in this course it look like non-stackable item only have a single "AvailabilityInfo::SlotInfo" and ::TotalAmountToFill=1
     - well it is in deed the starting slot of the whole stackable group [10-10-10-5]
     (2) "ClaimedSlotIndices"? = well as we're searching through the grid we need to start checking grid slots and if we have to add items to multiple slots, in other words let's say we have 35 Stacks the Mac stack count is 10 we have to add three stacks of 10 to three different slots and one stack of five to another slot, well we need to claim those slots (technically by adding their indices to the ClaimedSlotIndices declared right outside) we need to set them as claimed so that we don't try to add two items to the same slot
     UPDATE: this is in fact REDUNDANT, because it will be checked again in ForEach2D, but perhaps it may offer better overall performance?
	*/
		if (ClaimedSlotIndices.Contains(GridSlot->GridSlotIndex)) continue;

		if (IsOutOfBounds(GridSlot, InItemManifest.GetGridDimensions())) continue;
		
		TSet<int32> PotentialClaimedIndices{}; 

	// Can the item fit here? (i.e. is it out of grid bounds?) = check if we start here, all indices in GridSize is still valid an index?
		if (!HasRoomForGridSizeAtThisSlot(GridSlot, ClaimedSlotIndices, PotentialClaimedIndices, InItemManifest, MaxStackSize ))
		{
			continue;
		}

		ClaimedSlotIndices.Append(PotentialClaimedIndices);

    // How much to fill?
		/*if it pass the if check HasRoomForGridSizeAtThisSlot, then the Current GridSlot must be the starting Slot of the group 
		but anyway Stephen cover the case that it is not the starting slot but any slot in the group which is redundant here:
			int32 AvailableRoomOfThisGridSize = MaxStackSize - GridSlot->StackCount;
			int32 AmountToFillOnThisGridSize = bStackable ? FMath::Min(SourceAmount, AvailableRoomOfThisGridSize) : 1;
		//this can't be the case because we already check it, but anyway
			if (AmountToFillOnThisGridSize == 0) continue;
		*/
			int32 AmountToFillOnThisGridSize = CalculateAmountToFillForGridSlot(GridSlot, bStackable, MaxStackSize, SourceAmount);
		
    // Update the amount left to fill and AvailabilityInfo::TotalRoomToFill (it will be accumulated:)
		SourceAmount -= AmountToFillOnThisGridSize;
		AvailabilityInfo.TotalRoomToFill += AmountToFillOnThisGridSize; //it will be accumulated for non-stackable item as well!

	// Generate a SlotInfo to this qualified starting GridSlot and its whole GrizSize:
		/*the inner one "0" as signature, the outer one "1" at first (also signature to pass the first if check, and then modify to "0" as another signature - what a stupid reuse lol :D :D)
		 * so I decide to NOT modify it in the RPC2 lol, just pass in "1" && bStackable lol
		 */
	    FInventorySlotInfo SlotInfo;
			SlotInfo.AmountToFill = bStackable ? AmountToFillOnThisGridSize : 0; 
			SlotInfo.SlotArrayIndex = GridSlot->GridSlotIndex;
			SlotInfo.IsItemAtIndex = IsGridSlotPreoccupied(GridSlot); //Or = GridSlot->OwningItemData.IsValid()
		AvailabilityInfo.SlotInfos.Add(SlotInfo);
	}
	
    // How much is the Remainder? well the SourceAmount last after the loop is the Remainder itself (it could be 0 or bigger)
	AvailabilityInfo.Remainder = SourceAmount;

	//settings the rest: (What about AvailabilityInfo::OwningItemData? we don't set it too lol)
	AvailabilityInfo.bStackable = bStackable;

	return AvailabilityInfo;
}

//approach: we check the false, if passed all false we return true - except a special case: the slot isn't have any owning ItemData (i,e none of WBP_SlottedItem cover it so far)
bool UUW_Inv_InventoryGrid::HasRoomForGridSizeAtThisSlot(UUW_Inv_InventoryGridSlot* GridSlot,
	TSet<int32>& ClaimedSlotIndices, TSet<int32>& PotentialClaimedIndices, const FItemManifest& ItemManifest,
	int32 MaxStackSize)
{
	bool bHasRoomForGridSizeAtThisSlot = true;
	/*0. Is there room at this index? (i.e. are there other items in the way?) = check if we start here all GridSlots in GridSize are all with ::SlotState == unoccupied (in case of non-stackable item) || ::StackCount < MaxSize?
	// Check any other important conditions - ForEach2D over a 2D range
		//again must add <T> (that is <GridSlot*>) for it to work
		//question: can I capture other params in a param that is also a param of the same function = it hardly work lol, you don't need it anyway, because if you can pass in values for other params, you can always create local variables to be locally captured instead!  */
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>(
		GridSlots,
		GridSlot->GridSlotIndex,
		ItemManifest.GetGridDimensions(),
		columns,
		[&](UUW_Inv_InventoryGridSlot* SubGridSlot)
		{
			//the PotentialClaimedIndices param is REDUDANT for the sub helper:
			if (IsThisSubGridSlotQualified(GridSlot, SubGridSlot, ClaimedSlotIndices, PotentialClaimedIndices, ItemManifest, MaxStackSize))
			{
				PotentialClaimedIndices.Add(SubGridSlot->GridSlotIndex); 
			}
			//if any of those in GridSize for "current" GridSlot not qualified, then it's not qualified as the starting Slot,proceed to the NEXT one
			else
			{
				PotentialClaimedIndices.Empty(); //stephen don't have this line, but no need anyway
				bHasRoomForGridSizeAtThisSlot = false;
			}
		}
	);

	return bHasRoomForGridSizeAtThisSlot;
}

bool UUW_Inv_InventoryGrid::IsThisSubGridSlotQualified(UUW_Inv_InventoryGridSlot* GridSlot,
	UUW_Inv_InventoryGridSlot* SubGridSlot, TSet<int32>& ClaimedSlotIndices, TSet<int32>& PotentialClaimedIndices,
	const FItemManifest& ItemManifest, int32 MaxStackSize)
{
	//1. SubIndex claimed? = what do you mean? for each in  the group within GridSize?
	if (IsIndexClaimed(ClaimedSlotIndices, SubGridSlot->GridSlotIndex)) return false;
			
	//2. Has valid item? (to be more clear "IsThisSlotPreoccupied") = the only special case that return "true", hence the only case we need to .Add to the PotentialClaimedIndices before return too
	if (IsGridSlotPreoccupied(SubGridSlot) == false)
	{
		/*this is redundant, you already add at the outer function but luckily it doesn't hurt  even if you add it here again
		 *because "add the same value to TSet will be ignored" lol:  PotentialClaimedIndices.Add(SubGridSlot->GridSlotIndex);*/
		return true;
	}
						
	/*3. Does it belongs to an UpperLeftSlot (to be clear: Does the current GridSlot is the UpperLeftSlot of this SubGridSlot) = this can be done after check IsStackable for the case "stackable item" that occupied GridSize > {1,1}. But you know what we can also pick a lot of non-stackable items at the same time right (it doesn't appear to be the case in this course, but who know you want to expand it!), hence I decide to check it first
	*Explain why we need this check:
	-Outer loop: GridSlots: 0->n
	-Inner loop: GridSize: [a1->b1, a2->b2] – that could claim a group of GridSlot right in this turn
	-hence the next Outer turn (+1) it could deal with the "claimed" slots in previous turn already!
	@note: it also make sense in the case that allow non-stackable to pick many number at once (which doesn't support in this course)
	 */
	if (IsUpperLeftSlotOfThisSlot(GridSlot, SubGridSlot) == false) return false; //the order matter lol
			
	//4. Is this a stackable item?
	if (ItemManifest.IsStackable() == false) return false;

	//5. Is this preoccupied item the same type as the item we're trying to add?
	UItemData* PreoccupiedItemData = SubGridSlot->OwningItemData.Get();
	if (PreoccupiedItemData->IsItemOfType(ItemManifest.ItemTag) == false) return false;
		
	//6. Is this slot at the max stack size already?
	//warning: it is the upper-left/starting slot that hold the stack count for a stackable item (with GrizeSize > {1,1})
	// not "SubGridSlot" (even if they're potentially the same one)
	if (GridSlot->StackCount >= MaxStackSize) return false;
	
	return true;
}

bool UUW_Inv_InventoryGrid::IsOutOfBounds(UUW_Inv_InventoryGridSlot* GridSlot, const FIntPoint& GridDimensions)
{
	/* Unfortunately this won't work, it only consider the case the GridSize go down (bottom-right), but didn't consider the case it is "top-right"
		int32 GridSlotIndex = GridSlot->GridSlotIndex;
		//it must be "columns *  __", not "GridSize.X * __"
		int32 FinalGridSlotIndex = GridSlotIndex + columns * (GridDimensions.Y - 1) + (GridDimensions.X - 1);
	
		//out of bounds when theFinalGridSlotIndex goes beyond the actual final index of the GridSlots
		return FinalGridSlotIndex > (GridSlots.Num() - 1) ;
	*/
	// perhaps < 0 when you didn't even assign any index for our GridSlot (this is NOT true anyway, we assign it for all of them at creation already, and it is not subject to change). hence this line is "REDUDANT"
	if (GridSlot->GridSlotIndex < 0 || GridSlot->GridSlotIndex > GridSlots.Num() - 1) return true;
	
	int32 GridSlotIndex = GridSlot->GridSlotIndex;

	//the HoveredNormalizedPosition is (X,Y) , hence Row is Y lol
	const int32 row = GridSlotIndex / columns; 
	const int32 column = GridSlotIndex % columns;
	int32 EndRow = row + GridDimensions.Y;
	int32 EndColumn = column + GridDimensions.X;

	//either of them will make GridSize out of bounds:
	return (EndRow > rows) || (EndColumn > columns);
}

int32 UUW_Inv_InventoryGrid::CalculateAmountToFillForGridSlot(UUW_Inv_InventoryGridSlot*& GridSlot, bool bStackable, const int32 MaxStackSize, int32 TotalRoomLeftToFill)
{
	//OPTION1: if we pass in StartingGridSlot
		//int32 AvailableRoomOfThisGridSize = MaxStackSize - GridSlot->StackCount;
	//OPTION2: can passing any in the GridSize
	int32 AvailableRoomOfThisGridSize = MaxStackSize - GetStackCountFromAnySlotInGridSize(GridSlot); 
	return bStackable ? FMath::Min(TotalRoomLeftToFill, AvailableRoomOfThisGridSize) : 1;
}

//I refer to any slot in GridSize GENERALLY as "SubGridSlot" (including the starting one). But if it could be only the starting one specifically I will call it "GridSlot" or "UpperLeftSlot" or "StartingGridSlot"
int32 UUW_Inv_InventoryGrid::GetStackCountFromAnySlotInGridSize(UUW_Inv_InventoryGridSlot* SubGridSlot)
{
	if (SubGridSlot->UpperLeftIndex != INDEX_NONE && //redundant as IsValidIndex did it too
		GridSlots.IsValidIndex(SubGridSlot->UpperLeftIndex))
	{
		return GridSlots[SubGridSlot->UpperLeftIndex]->StackCount;
	}
	else
	{
		return SubGridSlot->StackCount;	
	}
}

bool UUW_Inv_InventoryGrid::IsIndexClaimed(const TSet<int32>& ClaimedIndices, int32 IndexToCheck)
{
	return ClaimedIndices.Contains(IndexToCheck);
}

//so we need to do GridSlot::OwningItemData.Reset() on all those in associate GridSize when we remove WBP_SlottedItem in order for this to work properly
bool UUW_Inv_InventoryGrid::IsGridSlotPreoccupied(UUW_Inv_InventoryGridSlot* GridSlot)
{
	return IsValid(GridSlot) && GridSlot->OwningItemData.IsValid();
}

bool UUW_Inv_InventoryGrid::IsUpperLeftSlotOfThisSlot( UUW_Inv_InventoryGridSlot* PotentialUpperLeftGridSlotToCheck, UUW_Inv_InventoryGridSlot* ThisGridSlot)
{
	return ThisGridSlot->UpperLeftIndex == PotentialUpperLeftGridSlotToCheck->GridSlotIndex;
}

bool UUW_Inv_InventoryGrid::HasRoomForGridSizeAtThisSlot(const FIntPoint& GridDimensions, UUW_Inv_InventoryGridSlot* StartGridSlot)
{
	bool bHasRoomForGridSizeAtThisSlot = true;
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>(
		GridSlots,
		StartGridSlot->GridSlotIndex,
		GridDimensions,
		columns,
		[&](UUW_Inv_InventoryGridSlot* SubGridSlot)
		{
			if (IsGridSlotPreoccupied(SubGridSlot))
			{
				bHasRoomForGridSizeAtThisSlot = false;
			}
		});
	
	return bHasRoomForGridSizeAtThisSlot;
}

//this callback is called whenever a ItemEntry is added to PC::InventoryComp::ItemFastArray, but only the one of the same ItemCategory can pass the first if check
void UUW_Inv_InventoryGrid::OnItemAddedCallback(UItemData* ItemData)
{
	if (DoesItemMatchGridCategory(ItemData) == false) return;

	DebugHelpers::Print("OnItemAddedCallback trigger");

	//step1: this is will be exactly the AvailabilityInfo get from PC::Input_E ~ PC::InventoryComp::TryAddItemToPlayerInventory
	//this is kind of lame, I can get AvailabilityInfo with ItemData inside of AddItemWidgetsToIndices itself lol
	FInventoryAvailabilityInfo AvailabilityInfo = GetAvailabilityInfoForItem(ItemData);

	//step2: create a widget to show ItemIcon and add it to the correct slot[s] on the Grid:
	AddItemWidgetsToIndices(AvailabilityInfo, ItemData);
	
}

void UUW_Inv_InventoryGrid::AddItemWidgetsToIndices(const FInventoryAvailabilityInfo& AvailabilityInfo, UItemData* ItemData)
{
	if (IsValid(ItemData) == false) return;

	//we create an WBP_Item instance per AvailabilityInfo::SlotInfo (we will understand why later). so if you want to factorize the code inside the for loop, you may want to push the shared code above into it too (accepting performance cost for readability)
	for (const FInventorySlotInfo& SlotInfo : AvailabilityInfo.SlotInfos)
	{
		AddItemWidgetToIndexFromSlotInfo(SlotInfo, AvailabilityInfo, ItemData);
	}
}

/*IMPORTANT: each SlotInfo will have a separate WBP_SlottedItem
, meaning SlotInfo doesn't mean one WBP_GridSlot
, it is a group of WBP_GridSlot[s] in GridSize holding a single WBP_SlottedItem
 */
void UUW_Inv_InventoryGrid::AddItemWidgetToIndexFromSlotInfo(const FInventorySlotInfo& SlotInfo, const FInventoryAvailabilityInfo& AvailabilityInfo, UItemData* ItemData)
{
/****this can be outside the loop*/
	//step0: [this can be outside the loop, but I move it in for READABILITY and REUSABILITY) ready ItemData::ItemManifest::ItemFragment_1,2,3... (shared for all AvailabilityInfo/Item::SlotInfo)
	//get Fragment_Grid to know the size of the item to occupy how many of the WBP_Grid::Slots
	const FItemFragment_Grid* ItemFragment_Grid = GetItemFragmentByTag<FItemFragment_Grid>(ItemData, ItemFragmentTags::Fragment_Grid);
	//get Fragment_Image so that we have an image to show
	const FItemFragment_Image* ItemFragment_Image = GetItemFragmentByTag<FItemFragment_Image>(ItemData, ItemFragmentTags::Fragment_Image);

	//UPDATE: you can make it in the way that if it doesn't have Fragment_Grid you give it GridDimensions={1,1} by default. This is a good practice I like it:
	if (ItemFragment_Image == nullptr /*|| ItemFragment_Grid == nullptr */ ) return;

	float GridPadding =  ItemFragment_Grid? ItemFragment_Grid->GridPadding : 0.f; //or a different "default" padding
	FIntPoint GridDimensions = ItemFragment_Grid? ItemFragment_Grid->GridDimensions : FIntPoint(1, 1);
	bool bStackable = AvailabilityInfo.bStackable;
/****this can be outside the loop*/
		
	//step1: CreateWidget<WBP_Item>(WBP_Item_Class)
	/*set OwningObject to PC or Canvas both okay I guess
		-but the one we set here will be "UObject::Outer" (assigned at creation), it is totally different from "potential parent" of this WBP_SlottedItem in WBP_Host's tree
		-if WBP_Sub is part of WBP_Host at compile time (i.e WBP_Sub is not dynamically spawned and added to WBP_Host::SomeContainer) if you CreateWidget<WBP_Host>(PC, WBP_Host_Class), then WBP_Sub::Outer will be PC by default (where WBP_Sub::ParentInTree=WBP_Host::SomeContainer)
		-only when you pass in (WBP_Host::SomeContainer, WBP_Host_Class) at first place it now because the Outer (of all WBP_Host and its static subwidgets)
		- Universal rule: All widgets created as part of a Widget Blueprint share the same Outer as the root widget created by CreateWidget.
		this not true when I dynamically spawn WBP_Sub and add it to WBP_Host at run time right
		, for the case you dynamically do CreateWidget<WBP_Sub>( 2ndOne, ) and added later, its Outer will be set to the 2ndOne
		, hence highly recommend set 2ndOne to the SAME one as you use to CreateWidget<WBP_Host>
		*/
	UUW_Inv_SlottedItem* WBP_SlottedItem = CreateWidget<UUW_Inv_SlottedItem>(GetOwningPlayer(), SlottedItem_Class);

	//step2A: set WBP_Item::BindWidgets::Values 
	/*UImage::SetBrush, SetBrushFromTexture, SetBrushFromMaterial - in fact will in turn call Brush::SetSourceObject(Texture/Material) - it is versatile function!
		 USlateBrush::SetSourceObject(UTexture2D/UMaterial/...) all will work, hence we don't see USlateBrush::SetImage*/
	FSlateBrush IconBrush;
	/*the image size of our WBP_Item is "a slot * b slot" in general for non-stackable item (not necessarily 1*1 for				stackable item or non-stackable item with that minimum size)
		just for fun. WBP_Item global size will be constraint to "WBP_Item::CanvasSlot::SizeInCanvas() anyway I guess
		- stephen don't even use this, hence it is REDUNDANT at first place lol
		- the FItemFragment_Image::IconSize will be in fact used for another purpose in other chapter! we'll see
		IconBrush.ImageSize = ItemFragment_Image->IconSize; //FOR FUN

		***hence stephen try to calculate the ultimate size here:
		- stephen "GridSlotSize - 2 * padding"  * GridDimensions, and it is not correct, it must be subtracted final.
		- Unless I misunderstand the FItemFragment::GridPadding meaning!

		IconBrush.ImageSize  = FDeprecateSlateVector2D (one inherit from FVector2f)
		FIntPoint will be converted to "FDeprecateSlateVector2D", hence "Stephen" work
		FVector2D won't be auto-converted to "FDeprecateSlateVector2D, hence "me" either:
		(1) do like the bellow
		(2) just "(ItemFragment_Grid->GridDimensions) * GridSlotSize - ItemFragment_Grid->GridPadding * 2.f" and it will be auto-converted magically behind the scene: A - B converted both to FIntPoint first and then FIntPoint is converted to  FDeprecateSlateVector2D with the cost of less inaccuracy (because IntPoint=int32 -> float on both A, B)
		*/
	IconBrush.ImageSize = GridDimensions * (GridSlotSize - GridPadding * 2.f); //STEPHEN
	IconBrush.ImageSize =
		FDeprecateSlateVector2D(GridDimensions * GridSlotSize) -
		FDeprecateSlateVector2D( GridPadding * 2.f, (GridPadding * 2.f)); //ME
		
	IconBrush.SetResourceObject(ItemFragment_Image->Icon);
	IconBrush.DrawAs = ESlateBrushDrawType::Type::Image;
	WBP_SlottedItem->SetImageIcon(IconBrush);
	WBP_SlottedItem->UpdateStackCount( bStackable? SlotInfo.AmountToFill : 0); //0 so that it collapses

	//step2B: set WBP_Item::SideValues (we need them because we'll need them lol, only time will tell lol)
	WBP_SlottedItem->GridIndex = SlotInfo.SlotArrayIndex;
	WBP_SlottedItem->bStackable = AvailabilityInfo.bStackable; //bStackable is shared for all potential SlotInfo that is for stackable item, hence you don't find it in AvailabilityInfo::SlotInfo but AvailabilityInfo itself
	WBP_SlottedItem->GridDimensions = GridDimensions;
	WBP_SlottedItem->OwningItemData = ItemData; //this mean that "one stackable ItemData" can be associated with "many WBP_Items of the same type" (for stackable item) - it is not "one-one" <=> "many - many for stackable case.

	//Step2C: (NEW) bind WBP_Grid::callback to WBP_SlottedItem::Delegate, doing it here mean this same callback is bound to all created WBP_SlottedItem in inventory (and it is bound right WBP_SlottedItem creation, even before it is being added as child of canvas and it is totally fine, why not) 
	WBP_SlottedItem->OnSlottedItemClickedDelegate.AddDynamic(this, &ThisClass::OnSlottedItemClicked);
	
	/*step3: calculate:
- GLOBAL size (fixed, can be calculated locally) 
- starting LOCATION (dynamic, represent by starting index of our WBP_Item in the Canvas)
Currently AvailabilityInfo::SlotInfo::GridSlotIndex
*/
	/*in CanvasPanel, there is "Anchor", CanvasSizeOfSlot, PositionOfSlot - no padding (at least by default), hence you need to do it yourself if you want it: (B is relying on auto-conversion: B = FVector2D(B) = FVector2D(B,B))
		-I put -B + A. so that it won't convert to FIntPoint before convert back to FVector (more convert of int32->float, more inaccuracy)
		-exactly the size we set for WBP_Item::Image_Icon above (meaning the step above is merely REDUNDANT, it will be constraint to the CanvasSize we set here anyway! yeah!)*/ 
	FVector2D SizeInCanvasSlot = - FVector2D(GridPadding * 2.f) + GridDimensions * GridSlotSize;

	/*again, you TOTAL WBP_Item::Image Size is " - 2*padding" already, hence if you draw exactly at "the associate GridSlot" the top-left of your image still go and match the top-left of the GridSlot, where on the right it is "2 padding" of space, which is not symmetric.
		 *Hence you would want to push it to the right (hence X+padding) and bottom (hence Y+padding) a bit*/
	FIntPoint NormalizePosition = UInv_BPFunctionLibrary::GetNormalizedPotionFromArrayIndex(SlotInfo.SlotArrayIndex, columns);
	FVector2D PositionToDawInCanvas = FVector2D(GridPadding) + NormalizePosition * GridSlotSize; //"1padding" here, not 2
		
	/*step4: add it into WBP_Grid::Canvas and get the Canvas slot
- now you can set SlotSize <=> become the size of WBP_Item (within canvas)
- now you can set SlotLocation <=> become the location of WBP_Item (within canvas)*/
	UCanvasPanelSlot* CanvasSlot = CanvasPanel_GridSlots->AddChildToCanvas(WBP_SlottedItem);
	CanvasSlot->SetSize(SizeInCanvasSlot);
	CanvasSlot->SetPosition(PositionToDawInCanvas);

	/*step5: Add the WBP_SlottedItem to TMap/TArray for bookkeeping
 **/
	SlottedItemMap.Add(SlotInfo.SlotArrayIndex, WBP_SlottedItem);

	/*step6: change SlotState of occupied WBP_GridSlot[s] by GridSize (WBP_Grid::GridSlots)
    && set its ::Values
	Goal:
	[StartIndex]->[StartIndex+Dim.X] 
	[(StartIndex+colums)]->[(StartIndex+colums+Dim.X)]
	But current: (also work)
	[StartIndex]              [StartIndex+Dim.X]
	     |                             |
	     v                             v
	[(StartIndex+colums)]     [(StartIndex+colums+Dim.X)]
 */
	/*this should work if I don't misunderstand the meaning of SlotInfo.SlotArrayIndex
		-yes it works like a charm so far!
		-funny: it just so happen that we don't need "rows", it will go as low as GridDimensions.Y
			int32 StartIndex = SlotInfo.SlotArrayIndex;
			for (int32 i = StartIndex; i < StartIndex + GridDimensions.X; i++)
			{
				for (int32 j = 0; j < GridDimensions.Y; j++)
				{
					int32 CurrentArrayIndex = i + j * columns;
					if (GridSlots.IsValidIndex(CurrentArrayIndex ))
					{
						GridSlots[CurrentArrayIndex]->SetSlotStateAndBrush(ESlotState::Occupied);				
					}
				}
			}
		*/
		
	//So we only assign StackCount to the FIRST
	GridSlots[SlotInfo.SlotArrayIndex]->StackCount = SlotInfo.AmountToFill;
		
	//funny, it must specify <T> for it to work, it can not deduce somehow. it should be because I use TFunction<void(T)> specifically instead of using < , typename FunctType> && "const FunctType& Function"
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>(
		GridSlots, SlotInfo.SlotArrayIndex, GridDimensions, columns,
		[&](UUW_Inv_InventoryGridSlot* WBP_GridSlot)
		{
			if (IsValid(WBP_GridSlot)) WBP_GridSlot->SetSlotStateAndBrush(ESlotState::Occupied);
			WBP_GridSlot->bAvailable	 = false;
			WBP_GridSlot->OwningItemData = ItemData;
			WBP_GridSlot->UpperLeftIndex = SlotInfo.SlotArrayIndex;
	     	
			/*I need to think about this lol:
	     	 (1) if I call it here, it means it set it on all occupied WBP_GridSlot of the same owning ItemData
	     	 - if I call it on the starting WBP_GridSlot (outside), then only that one contain this info

			 (2) it doesn't look correct with  "WBP_GridSlot->StackCount = SlotInfo.AmountToFill" at all if I understand its meaning correctly
			 
	     	 (3)  it does look more correct if we set it to WBP_GridSlot->StackCount = AvailabilityInfo.TotalRoomToFill instead, because
	     	AvailabilityInfo.TotalRoomToFill = AvailabilityInfo.SlotInfo_1::AmountToFill +  AvailabilityInfo.SlotInfo_2::AmountToFill + ...
	     	- but again even so it doesn't look correct in case the stackable item already exist and you pick again
	     			WBP_GridSlot->StackCount = SlotInfo.AmountToFill; 
	     	 */
		}
	);
}

//the reason why it works is that when we create GetAvailabilityInfo(ItemData/ItemComponent/ItemManifest) we also consider the case some WBP_SlottedItem instance already in WBP_GridSlot[s]/GridSize and some doesn't create yet. Absolutely amazing!
void UUW_Inv_InventoryGrid::OnStacksAddedCallback(const FInventoryAvailabilityInfo& AvailabilityInfo)
{
	//because this chain assume the owning stackable ItemData already exist in FastArray (that doesn't necessarily mean WBP_SlottedItem is enough for new pickup):
	if (DoesItemMatchGridCategory(AvailabilityInfo.ItemData.Get()) == false) return;

	DebugHelpers::Print("OnStacksAddedCallback trigger");
		
	for (FInventorySlotInfo SlotInfo : AvailabilityInfo.SlotInfos)
	{
		//we expect the first WBP_SlottedItem already exist
		if (SlotInfo.IsItemAtIndex) // = WBP_GridSlot::OwningItemData.IsValid() - go back GetAvaiInfo and see!
		{
			//these lines are just for fun. I believe if it reaches here it must be valid (unless something wrong)
			if (GridSlots.IsValidIndex(SlotInfo.SlotArrayIndex) == false) break;
			if (SlottedItemMap.Contains(SlotInfo.SlotArrayIndex) == false) break;

			//this is the starting WBP_GridSlot:
			UUW_Inv_InventoryGridSlot* WBP_GridSlot = GridSlots[SlotInfo.SlotArrayIndex];
			WBP_GridSlot->StackCount += SlotInfo.AmountToFill; //+=, not = 

			//this is the associate WBP_SlottedItem occupied GridSize: (luckily we save it into TMap<SlotArrayIndex, WBP_SlottedItem>)
			TObjectPtr<UUW_Inv_SlottedItem> WBP_SlottedItem = SlottedItemMap[SlotInfo.SlotArrayIndex];
			WBP_SlottedItem->UpdateStackCount(WBP_GridSlot->StackCount); //very smart move! DO NOT +again, you just update Grid::StackCount!
		}
		//we expect the next ones doesn't create yet, hence create them exactly like the way we did in the final sub chain of AddNewItem ~> AddItemWidgetsToIndices
		else
		{
			//this handle setting "WBP_SlottedItem::StackCount" && "WBP_GridSlot::StackCount itself
			AddItemWidgetToIndexFromSlotInfo(SlotInfo, AvailabilityInfo, AvailabilityInfo.ItemData.Get());
		}
	}
}

/* What to know:
(1) row and column to 1DIndex
1DIndex/GridSlotIndex = i * columns + j //provided that rows(i) is outer loop, column(j) is inner loop

(2) 1DIndex to Row and column:
RowIndex    = i = GridSlotIndex / columns; //floor is no need, it will be truncated anyway
ColumnIndex = j = GridSlotIndex % columns; 
*/
void UUW_Inv_InventoryGrid::ConstructGridSlots()
{
	//good practice: if you're to know how exactly many elements you're to add for your array then just do ".Preserve(n)" for the better performance:
	GridSlots.Reserve(rows * columns);
	
	for (int32 i = 0 ; i < rows; i++)
	{
		for (int32 j = 0 ; j < columns; j++)
		{
			//construct WBP_GridSlot: (review: if it is not UUW+, then use NewObject<UWidget> and then .AddChild() )
			UUW_Inv_InventoryGridSlot* GridSlot = CreateWidget<UUW_Inv_InventoryGridSlot>(this, GridSlot_Class);
			GridSlot->GridSlotIndex = i * columns + j;

			//add it to WBP_InventoryGrid::Canvas (if you don't use AddChild[ToContainer] version, you must then use UWidgetLayoutLibrary::GetSlotAsXSlot(InChildOfXContainer) to get back that slot )
			UCanvasPanelSlot* CanvasPanelSlot = CanvasPanel_GridSlots->AddChildToCanvas(GridSlot);

			//set its size and position within canvas: there is no need of FIntPoint SlotPosition(j, i) and then "x SlotSize" at all
			CanvasPanelSlot->SetSize(FVector2D(GridSlotSize, GridSlotSize)); //it also has constructor accept "a" -->"a,a"

			//it must be "X ~ which column/j" and "Y ~ which row/i" so don't do it by habit lol
			CanvasPanelSlot->SetPosition(FVector2D( j * GridSlotSize, i * GridSlotSize ));
			
			//add it to GridSlots array as well:
			GridSlots.Add(GridSlot);

			//bind delegates, it is pointer so bind before or after .Add doesn't matter:
			GridSlot->OnGridSlotHovered.AddDynamic(this, &ThisClass::OnGridSlotHovered);
			GridSlot->OnGridSlotUnhovered.AddDynamic(this, &ThisClass::OnGridSlotUnhovered);
			GridSlot->OnGridSlotClicked.AddDynamic(this, &ThisClass::OnGridSlotClicked);
		}
	}	
}

void UUW_Inv_InventoryGrid::OnGridSlotHovered(const int32& AffectedGridSlot, const FPointerEvent& PointerEvent)
{
	if (IsValid(WBP_HoverItem)) return; //when this is valid it handle itself already, we shouldn't alter it
	if (GridSlots.IsValidIndex(AffectedGridSlot) == false) return; //for fun, no need

	UUW_Inv_InventoryGridSlot* WBP_GridSlot = GridSlots[AffectedGridSlot];
	if (WBP_GridSlot->bAvailable)
	{
		WBP_GridSlot->SetSlotStateAndBrush(ESlotState::Occupied);
	}
}

void UUW_Inv_InventoryGrid::OnGridSlotUnhovered(const int32& AffectedGridSlot, const FPointerEvent& PointerEvent)
{
	if (IsValid(WBP_HoverItem)) return; //when this is valid it handle itself already, we shouldn't alter it
	if (GridSlots.IsValidIndex(AffectedGridSlot) == false) return; //for fun, no need

	UUW_Inv_InventoryGridSlot* WBP_GridSlot = GridSlots[AffectedGridSlot];
	if (WBP_GridSlot->bAvailable) //even for unhovered, we don't want to touch on a preoccupied one
	{
		WBP_GridSlot->SetSlotStateAndBrush(ESlotState::Unoccupied);
	}
}

void UUW_Inv_InventoryGrid::OnGridSlotClicked(const int32& AffectedGridSlot, const FPointerEvent& PointerEvent)
{
//if WBP_HoverItem is NOT valid. In case we click on WBP_SlottedItem it is self-handled, otherwise we didn't need to do anything. Either case we don't need to do anything here, simply return is enough:
	if (IsValid(WBP_HoverItem) == false) return;
	if (GridSlots.IsValidIndex(DropIndex) == false) return; /*no need  -- GetSpaceQueryResult already check and return a default FSpaceQueryResult if it is not valid at first place (hence if it is not valid we'll know below anyway)*/
	
/*if WBP_HoverItem is valid we move SlottedItem (WBP_HoverItem-->WBP_SlottedItem on the new index = DropIndex) or swap [WBP_PreoccupiedSlottedItem -> WBP_NewHoverItem (on Mouse)] <-->
 [WBP_HoverItem              -> WBP_SlottedItem (on PreoccupiedGridSlot::UpperLeftIndex)]
 */
	/*this is the case we want to put WBP_HoverItem down (no exchange simply as that), we has a AddItemWidgetToIndex function can be re-used? well it require SlotInfo and AvailabilityInfo, can we fake it? well just use it as a reference lol.
	-well because we have WBP_HoverItem that more than a backup of WBP_OldSlotItem, hence we simply transition it! hell yeah!I totally forget it lol = but still we need to re-use the inner content of AddItemWidgetToIndex, you can either factorize it to re-use or just repeat what need here*/
	if (SpaceQueryResult.bHasSpace)
	{
	//step1: add back WBP_SlottedItem
		//OPTION1: refactor AddItemWidgetToIndex{ [Other Ready params; CreateSlottedItem(___), UpdateGridSlotsInGridSize(___)} and then re-use it
		//OPTION2: fake "SlotInfo and AvailabilityInfo" (only need to filter in what is need) and then re-use AddItemWidgetToIndex directly:
		FInventorySlotInfo FakeSlotInfo{};
			FakeSlotInfo.SlotArrayIndex = DropIndex;
			FakeSlotInfo.AmountToFill   = WBP_HoverItem->StackCount; //kind of fake
			FakeSlotInfo.IsItemAtIndex  = false;                     //(it is false by default anyway) surely no when bHasSpace=true 
			
		FInventoryAvailabilityInfo FakeAvailabilityInfo{};
			FakeAvailabilityInfo.bStackable = WBP_HoverItem->bStackable; //this matter!
			FakeAvailabilityInfo.ItemData = WBP_HoverItem->OwningItemData; //this matter!
			FakeAvailabilityInfo.SlotInfos.Add(FakeSlotInfo);  //this doesn't matter here
			FakeAvailabilityInfo.TotalRoomToFill = 1;          //this doesn't matter here
			FakeAvailabilityInfo.Remainder = 0;                //this doesn't matter here
			
		AddItemWidgetToIndexFromSlotInfo(FakeSlotInfo, FakeAvailabilityInfo, WBP_HoverItem->OwningItemData.Get());

	//step2: set WBP_HoverItem back to nullptr 
		/*funny this doesn't really make WBP_HoverItem copy for PC::MouseCursorWidget to go away?
		-this won't have any effect, because you didn't add it to Viewport at first place
		-change WBP_HoverItem::Values does change PC::MouseCursorWidget visually, no why it is conventional for good*/
		WBP_HoverItem->RemoveFromParent(); //for fun, has no effect 
		WBP_HoverItem = nullptr; //this do have an effect that reduce reference count
		DropIndex = -1;
		
	//step3: re-set MouseCursorWidget to "default one" or "custom one"
		//OPTION1: when passing in "nullptr" I get the very default mouse cursor back, how cool is that :D :D
			//GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Type::Default, nullptr);
		//OPTION2: (if you want a custom one lol - we plan to make it have different color when on different WBP_Grid = good practice)
		ShowVisibleCursorWidget();
	}
	/*this is the case we want to exchange (we consider "merge" later don't worry)
	//should we just assume it is the else case? hell no! it could be HasSpace=false and PreoccupiedItemData=invalid at the same time (when it overlap with 2+ preoccupied items at once or when it is out of bounds)*/
	else if (SpaceQueryResult.PreoccupiedItemData.IsValid())
	{
		/*
		 (1) we only reach this code  when WBP_HoverItem is valid, and if so, it will only execute the "bottom" part of OnSlottedItemClicked it self (you can can create a separate small function to be called in both here and there if you want = better readibility) 
		 (2) you don't necessarily exactly click the WBP_PreoccupiedSlottedItem, you click mouse wherever when it overlap with our current WBP_HoverItem, a perfect re-use :D :D
		-it is like "forward" to the SlottedItemClicked chain (PointerEvent is passed, not even care to filter key, because it is self-handled there already! yeah!)
		-this chain will naturally SetMouseCursorWidget to the WBP_PreoccupiedHoverItem! how cool is that (but it doesn't handle put the WBP_HoverItem down yet) */
		OnSlottedItemClicked(SpaceQueryResult.TopLeftSlotIndexOfPreoccupiedItem, PointerEvent);
	}
}

bool UUW_Inv_InventoryGrid::IsTheSameStackableItemAsHoverItem(UItemData* ClickedItemData)
{
	return WBP_HoverItem->OwningItemData == ClickedItemData &&
		//we can do this pointer check because UItemData is replicated
		ClickedItemData->IsStackable() &&
		ClickedItemData->GetItemTag() == WBP_HoverItem->OwningItemData->GetItemTag(); //this one is redundant 
}

/****WBP_SlottedItem && OnSlottedItemClicked chain
 *___Callback is bound to all WBP_SlottedItem::OnSlottedItemClicked in AddItemWidgetToIndexFromSlotInfo
 * its job is to create new WBP_Hover && destroy and remove WBP_SlottedItem from WBP_Grid::Canvas and SlottedItemMap
 * - but the ItemData is still in the FastArray (waiting for cases to decide in the end)
 */
void UUW_Inv_InventoryGrid::OnSlottedItemClicked(int32 ClickedUpperLeftIndex, const FPointerEvent& MouseEvent)
{
//remove WBP_ItemDescription before create WBP_HoverItem: (luckily we create the helper that can call anywhere!)
	UInv_BPFunctionLibrary::OnItemUnhovered(GetOwningPlayer());
	
//STEP_A: access back the WBP_SlottedItem, [starting] WBP_GridSlot, WBP_GridSlot::OwningItemData from the GridSlot with that GridIndex:
	if (GridSlots.IsValidIndex(ClickedUpperLeftIndex) == false) return;
	if (SlottedItemMap.Contains(ClickedUpperLeftIndex) == false) return;

	//we may not need all of them lol, GridSlots and SlottedItemMap are members, hence you can access its associate WBP_SlottedItem and starting WBP_GridSlot any time as long as you have its SlotIndex:
		//this is upper left one:
	UUW_Inv_InventoryGridSlot* ClickedUpperLeftGridSlot = GridSlots[ClickedUpperLeftIndex];
		//this occupies the whole GridSize including upper left above:
	UUW_Inv_SlottedItem* ClickedSlottedItem = SlottedItemMap[ClickedUpperLeftIndex]; //it stands here as an alternative for ::Values

	/*ItemData is where we get back ItemManifest (and even AvailabilityInfo in other context - but it is appropriate to be used here now). if it isn't valid - something wrong, you better re-check if WBP_GridSlot::OwningItemData is set (it is, it is stored on every WBP_GridSlot in GridSize which I think overkill and give extra work as we need to reset them back to nullptr when we move/remove the ItemData in Grid).
	 *You can also get it back from WBP_SlottedItem::OwningItemData (store per GridSize)*/
	UItemData* ClickedItemData = GridSlots[ClickedUpperLeftIndex]->OwningItemData.Get();
	if (IsValid(ClickedItemData) == false) return; 
	
	
//STEP_EXTRA: handle RightClick <-> WBP_ItemPopup, this time I will create a sub function so that I don't pollute this place lol:	
	if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		CreateItemPopupWidget(ClickedUpperLeftIndex);
	}
	
/*STEP_B: only create WBP_HoverItem if "WBP_HoverItem isn't valid && MouseEvent.GetKey() = EKeys::LClick" ( create a new one && Set  WBP_HoverItem::Values exactly the way you did for WBP_SlottedItem in step3)
 * why we only proceed if(!WBP_HoverItem) ? well because here is what we will do
 WBP_HoverItem will be generated and assign as we LClick on WBP_SlottedItem
 WBP_HoverItem will be destroyed and set back to nullptr at the beginning or done moving
 * meaning if it is valid, then we assume is currently in progress
 */
	if (IsValid(WBP_HoverItem) == false &&
		MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		//STEP_C + STEP_D:
		CreateHoverItemAndRemoveClickedSlottedItem(ClickedUpperLeftIndex, ClickedUpperLeftGridSlot, ClickedSlottedItem, ClickedItemData);
		return;
	}

	/*STEP_E: if WBP_HoverItem is valid, considering "the incomplete swapping" or "merging/like" (for stackable Item) */
	/*case1: Are WBP_HoverItem and WBP_PreoccupiedSlottedItem the same type and stackable?
	 -because  a specific stackable item is stored as a single ItemData in FastArray (and UItemData currently replicated as sub object of UInventoryComp), so we only need to check if the pointers are equal and they're stackable is enough (trying to do A->GetItemTag() == B->GetItemTag() is redundant)*/
	if (IsValid(WBP_HoverItem) == false) return; 
	if (WBP_HoverItem->OwningItemData == ClickedItemData && //we can do this pointer check because UItemData is replicated
		ClickedItemData->IsStackable() &&
		ClickedItemData->GetItemTag() == WBP_HoverItem->OwningItemData->GetItemTag() //this one is redundant
	) 
	{
	//ready side values
		//NOT ClickedItemData->TotalStackCount = the actual total of all WBP_SlottedItem 's StackCount of the same stackable type, not of... 
		//ClickedSlottedItem->StackCount doesn't work, we didn't create this var nor assign it at first place = which is stupid I think
		//but luckily the TopLeftGridSlot hold it :D :D (you can assign it for all grid slots in GridSize if you want)
		int32 StackCount_PreoccupiedSlottedItem = ClickedUpperLeftGridSlot->StackCount; 
		int32 StackCount_HoverItem = WBP_HoverItem->StackCount;
		int32 MaxStackCount = ClickedItemData->GetMaxStackCount();
		
		//Should we swap their stack counts? = why don't we just do nothing? = why don't consider the other way?
		if (StackCount_PreoccupiedSlottedItem == MaxStackCount && StackCount_HoverItem < MaxStackCount)
		{
			//because the ItemData::TotalStackCount for a single stackable type won't change, so swapping stack counts is merely swap SlottedItem::StackCount (didn't create, so must use GridSlot[Index]->StackCount) and WBP_HoverItem::StackCount - as well as update their TextBlock_StackCount
			//Review: UpperLeftGridSlot always need StackCount variable because we need it to GetAvailabilityInfo(), where WBP_SlottedItem::StackCount remain optional
			ClickedUpperLeftGridSlot->StackCount = StackCount_HoverItem;   
			ClickedSlottedItem->UpdateStackCount(StackCount_HoverItem);
			
			WBP_HoverItem->StackCount = StackCount_PreoccupiedSlottedItem; //no need
			WBP_HoverItem->UpdateStackCount(StackCount_PreoccupiedSlottedItem); //already update here
			return; //the reason why i can still swap them back and forth is I forget to return here? no because after that it move down to the case (StackCount_PreoccupiedSlottedItem < MaxStackCount) - explain why stephen didn't do a symetric "||" HERE!yeah! I got it!
		}
		
		//consume the WBP_HoverItem::Stack when there is still enough room  in WBP_PreoccupiedSlottedItem for them
		if (StackCount_PreoccupiedSlottedItem + StackCount_HoverItem <= MaxStackCount)
		{
			ClickedUpperLeftGridSlot->StackCount += StackCount_HoverItem;   
			ClickedSlottedItem->UpdateStackCount(ClickedUpperLeftGridSlot->StackCount);

			ShowVisibleCursorWidget();
			WBP_HoverItem = nullptr; //(*)
			
			//we need to reset [PreoccupiedGridSize::GridSlot::State from [GrayedOut -> Occupied], because by doing (*) we're not re-using anything - for each 2D will do? well there is already "HighlightSlots" you can use lol!
			HighlightSlots(ClickedUpperLeftIndex, ClickedSlottedItem->GridDimensions);
			return;
		}
		
		//otherwise just fill in WBP_PreoccupiedSlottedItem and keep WBP_HoverItem (with Stack reduced)
		//this is where you feel like it is swapping (explain why Stephen don't need to use symmetric || for the swapping case)
		if (StackCount_PreoccupiedSlottedItem < MaxStackCount)
		{
			//we already consider the "consume" case, meaning it must be amount left for WBP_HoverItem, so we work on that assumption:
			int32 AmountToAdd = MaxStackCount - StackCount_PreoccupiedSlottedItem;
			int32 Remainder = StackCount_HoverItem - AmountToAdd;
			ClickedUpperLeftGridSlot->StackCount = MaxStackCount;   
			ClickedSlottedItem->UpdateStackCount(MaxStackCount);

			WBP_HoverItem->StackCount = Remainder; //no need
			WBP_HoverItem->UpdateStackCount(Remainder);
			
			//ShowVisibleCursorWidget(); //hell no, this time we still want WBP_HoverItem remain
			return;
		}
		
		//the only possibility to reach down here is StackCount_PreoccupiedSlottedItem == StackCount_HoverItem == MaxStackCount, hence think of it like a backup so that we return early, but you can simply put "return" directly so that it won't go down to the "case2" code outside of this if
		//anyway do nothing (may be play a sound if you want to)
		if (StackCount_PreoccupiedSlottedItem == MaxStackCount)
		{
			return;		
		}
	}

//case2: if not, swap WBP_HoverItem and WBP_PreoccupiedSlottedItem
	//(0) cache the current WBP_HoverItem::Values before remove it:
	FInventorySlotInfo FakeSlotInfo{};
	FakeSlotInfo.SlotArrayIndex = DropIndex; //DropIndex if want it start where the mouse is or ClickedUpperLeftIndex if you want it start where the WBP_PreOccupiedSlottedItem - stephen choose DropIndex
	FakeSlotInfo.AmountToFill   = WBP_HoverItem->StackCount; //this matter! (kind of fake)
	FakeSlotInfo.IsItemAtIndex  = false;                     //(it is false by default anyway) surely no when bHasSpace=true 
    				
	FInventoryAvailabilityInfo FakeAvailabilityInfo{};
	FakeAvailabilityInfo.bStackable = WBP_HoverItem->bStackable;   //this matter!
	FakeAvailabilityInfo.ItemData = WBP_HoverItem->OwningItemData; //this matter!
	FakeAvailabilityInfo.SlotInfos.Add(FakeSlotInfo);  //this doesn't matter here
	FakeAvailabilityInfo.TotalRoomToFill = 1;          //this doesn't matter here
	FakeAvailabilityInfo.Remainder = 0;                //this doesn't matter here
	
	/*(1) WBP_PreoccupiedSlottedItem -> WBP_NewHoverItem (only one WBP_HoverItem can exist at a time currently) = re-use CreateHoverItemAndRemoveClickedSlottedItem (also used above in case WBP_HoverItem isn't valid) = this has side effect that it changes GridSlots in GridSize::State hence let it be done first
	- it is OnGridSlotClicked 's responsibility to pass in and forward the right params for this to work! (*)
	- funny the code of line is exactly above (but the only different is (*)) - explain why I recommend to factonize STEP_E into a sub function to be called both in OnGridSlotClicked and OnSlottedItemClicked for better readability (rather forward from OnGridSlotClicked)*/
	CreateHoverItemAndRemoveClickedSlottedItem(ClickedUpperLeftIndex, ClickedUpperLeftGridSlot, ClickedSlottedItem, ClickedItemData);
	
	//(2) WBP_HoverItem -> WBP_SlottedItem (replace the exact index of WBP_PreoccupiedSlottedItem) = re-use "AddItemAtIndex" (or look at the "PutDown" case in OnGridSlotClicked)	= this doesn't actually need WBP_HoverItem to exist, but it does need WBP_HoverItem::Values to be cached at first place				
	AddItemWidgetToIndexFromSlotInfo(FakeSlotInfo, FakeAvailabilityInfo, FakeAvailabilityInfo.ItemData.Get()); //not " WBP_HoverItem->OwningItemData.Get()"
	
}


//this function triggers when we RClick on WBP_SlottedItem. Should we create "GetItemPopup" instead? well I feel the need
//however for WBP_ItemDescription that need to show more options you may want to cache and "modify" it instead of create a new one for better performance (like the logic of items of UListView)
void UUW_Inv_InventoryGrid::CreateItemPopupWidget(const int32& OwningIndex)
{
//step1: create widget
	//UUW_Inv_ItemPopup* WBP_ItemPopup = CreateWidget<UUW_Inv_ItemPopup>(GetOwningPlayer(), ItemPopup_Class); //or this
	//if (IsValid(WBP_ItemPopup) == false) return; //It will fail when you forget to select the class

	if (IsValid(WBP_ItemPopup)) return; //if there is one ... then shouldn't create more until it is destroyed
	WBP_ItemPopup = CreateWidget<UUW_Inv_ItemPopup>(GetOwningPlayer(), ItemPopup_Class);

	//to make sure it is destroyed when the inventory is closed (but not now, because we didn't know when WBP_ItemPopup::RemoveFromParent() is called). Go to OnNativeDestruct and you will see that the delegate require "UUW* " param
	WBP_ItemPopup->OnNativeDestruct.AddLambda([this](UUserWidget* Menu) { WBP_ItemPopup = nullptr; }); 
	
//step2: assign OwningIndex+
	WBP_ItemPopup->OwningIndex = OwningIndex;
	
//step3: add as a child of OuterCanvas,  set position and size for the returning Slot:
	UCanvasPanelSlot* OuterCanvasSlot = OuterCanvas->AddChildToCanvas(WBP_ItemPopup.Get());

	/*try1: we can't use this because PC::GetMousePosition() return absolute position
	float X,Y;
	GetOwningPlayer()->GetMousePosition(X,Y); //this is absolute lol (smaller in value), I test it!
	OuterCanvasSlot->SetPosition(FVector2D(X,Y) + ItemPopupOffset);
	OuterCanvasSlot->SetAutoSize(true);
	OuterCanvasSlot->SetSize(WBP_ItemPopup->GetSizeBox()); //no need
	
	DebugHelpers::Print("PC::GetMousePosition: " + FString::SanitizeFloat(X) + ", " + FString::SanitizeFloat(Y));
	DebugHelpers::Print("UWidgetLayoutLibrary::GetMousePositionOnViewport: " + MousePositionOnViewport.ToString());
	*/
	
	//try2: we must do this because it returns local position. we need local position because Setting location in canvas or any sub widget container::SetPosition(InLocalPosition) need you to pass in "Local position" 
	FVector2D MousePositionOnViewport = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
	OuterCanvasSlot->SetPosition(MousePositionOnViewport + ItemPopupOffset);
	OuterCanvasSlot->SetAutoSize(true);
	OuterCanvasSlot->SetSize(WBP_ItemPopup->GetSizeBox()); //no need

//step4: callbacks to WBP_ItemPopup::Delegates
	//always bind Drop:
	WBP_ItemPopup->OnDropDelegate.BindDynamic(this, &ThisClass::OnDropButtonClicked);
	
	//only bind Split if it is stackable item and StackCount >= 2 <=> MaxSplitAmount >= 1
	UItemData* ItemData = GridSlots[OwningIndex]->OwningItemData.Get();
	if (IsValid(ItemData) == false) return;
	
	int32 MaxSplitAmount = GridSlots[OwningIndex]->StackCount - 1;
	bool bIsStackable = ItemData->IsStackable();
	if (MaxSplitAmount >=1 &&
		bIsStackable)         //redundant, because if it non-stackable, StackCount=0 and the first condition can't be met
	{
		WBP_ItemPopup->SetSliderValueAndParams( FMath::Max(1.f , MaxSplitAmount / 2.f ), MaxSplitAmount);
		WBP_ItemPopup->OnSplitDelegate.BindDynamic(this, &ThisClass::OnSplitButtonClicked);
	}
	else
	{
		WBP_ItemPopup->CollapseSplit();
	}
	
	//only Consume if it is ItemCategory is consumable: (consumable, equippable, craftable  -- totally independent from stackable)
	if (ItemData->GetItemCategory() == EItemCategory::Consumable)
	{
		WBP_ItemPopup->OnConsumeDelegate.BindDynamic(this, &ThisClass::OnConsumeButtonClicked);
	}
	else
	{
		WBP_ItemPopup->CollapseConsume();
	}
}

void UUW_Inv_InventoryGrid::OnDropButtonClicked(int32 OwningIndex)
{
//step0: ready (copy almost everything from OnSplitButtonClicked)
	//if we already reach here, most of the checks are not needed, but anyway:
	if (GridSlots.IsValidIndex(OwningIndex) == false) return;
	if (SlottedItemMap.Contains(OwningIndex) == false) return;

	UUW_Inv_InventoryGridSlot* GridSlot = GridSlots[OwningIndex];
	UItemData* ItemData = GridSlot->OwningItemData.Get();
	if (IsValid(ItemData) == false) return;
	UUW_Inv_SlottedItem* SlottedItem = SlottedItemMap[OwningIndex];

/*OPTION1: clean option but not re-usable*/
	/*step1: remove from WBP_Grid (cosmetic). Basically you may feel to do it last, but since ServerRPC is sent to be executed in the server after a ping delay, so this code always run before code in ServerRPC no matter where it is in this function! hence Stephen decide to do it first:
	//this set GridSlot::StackCount=0, re-set states of grid slots in GridSize (directly - no HoverItem appear, no need "ShowVisibleCursor()" back neither! -- without create HoverItem and then remove HoverItem like Stephen lol - waste too much free time :D :D )
	//cache the GridSlot->StackCount before it is set back to zero:*/
	int32 StackCountToDrop = GridSlot->StackCount;
	
	//this has side effect and set StackCount=0, hence we need to cache it or call the step2 first!
	RemoveClickedSlottedItem(OwningIndex, GridSlot, SlottedItem, ItemData->GetGridDimensions());
	
	//step2: remove from FastArray (must be done in Server) && spawn BP_Item back to world (must be done in Server as well, hence wrapped in the same ServerRPC)
	UInv_BPFunctionLibrary::GetInventoryComponentFromPC(GetOwningPlayer())->ServerRPC_DropItem( ItemData, StackCountToDrop);

/*OPTION2: replace step1,2 and factorize it into DropItem so that it can be re-used = you can still always re-use lol, just create the DropHoverItem()
	//this make WBP_SlottedItem disappear and WBP_HoverItem appear:
	CreateHoverItemAndRemoveClickedSlottedItem(OwningIndex, GridSlot, SlottedItem, ItemData);

	//factorize this into "DropHoverItem()":
	DropHoverItem();
 */
}

void UUW_Inv_InventoryGrid::ClearHoverItem()
{
	if (IsValid(WBP_HoverItem) == false) return;
	
	WBP_HoverItem->RemoveFromParent(); 
	WBP_HoverItem = nullptr;
	ShowVisibleCursorWidget();
}

void UUW_Inv_InventoryGrid::DropHoverItem()
{
	if (IsValid(WBP_HoverItem) == false) return;
	if (WBP_HoverItem->OwningItemData.IsValid() == false) return; 
	UInv_BPFunctionLibrary::GetInventoryComponentFromPC(GetOwningPlayer())->ServerRPC_DropItem( WBP_HoverItem->OwningItemData.Get(), WBP_HoverItem->StackCount);

	ClearHoverItem();
}

//from WBP_ItemPopupSize, Players adjust Slider and see the wanted SplitAmount. And then press the SplitButton. the current value of slider, that is the SplitAmount, will be broadcast here. And we only need to handle it
void UUW_Inv_InventoryGrid::OnSplitButtonClicked(int32 OwningIndex, int32 SplitAmount)
{
//ready:
	//if we already reach here, most of the checks are not needed, but anyway:
	if (GridSlots.IsValidIndex(OwningIndex) == false) return;
	if (SlottedItemMap.Contains(OwningIndex) == false) return;

	UUW_Inv_InventoryGridSlot* GridSlot = GridSlots[OwningIndex];
	UItemData* ItemData = GridSlot->OwningItemData.Get();
		if (IsValid(ItemData) == false) return;
		if (ItemData->IsStackable() == false) return; 
	UUW_Inv_SlottedItem* SlottedItem = SlottedItemMap[OwningIndex]; //let it crash if it doesn't contain
	
//reduce StackCount of WBP_SlottedItem:
	GridSlot->StackCount -= SplitAmount;  //you don't want forget this step lol
	SlottedItem->UpdateStackCount(GridSlot->StackCount); //we just reduce it, don't reduce twice lol
	
//create WBP_HoverItem with OverrideStackCount = SplitAmount (not removing WBP_SlottedItem)
	CreateHoverItem(GridSlot, ItemData, SplitAmount);
}


void UUW_Inv_InventoryGrid::OnConsumeButtonClicked(int32 OwningIndex)
{
//ready:
	//if we already reach here, most of the checks are not needed, but anyway:
	if (GridSlots.IsValidIndex(OwningIndex) == false) return;
	if (SlottedItemMap.Contains(OwningIndex) == false) return;

	UUW_Inv_InventoryGridSlot* GridSlot = GridSlots[OwningIndex];
	UItemData* ItemData = GridSlot->OwningItemData.Get();
	if (IsValid(ItemData) == false) return;
	if (ItemData->IsStackable() == false) return; 
	UUW_Inv_SlottedItem* SlottedItem = SlottedItemMap[OwningIndex]; //let it crash if it doesn't contain
	
//step1: (COSMETIC ~ Split, but WBP_HoverItem need not to create, cause' we consume it)
	//reduce StackCount of WBP_SlottedItem or remove it if StackCount reach zero:
	GridSlot->StackCount -= 1;  //you don't want forget this step lol

	//the "if" will cover 2 cases: "non-stackable" and "stackable" reaching "0".
	//"<" will be important in case it is non-stackable item start of with StackCount=0
	if (GridSlot->StackCount <= 0) 
	{
		RemoveClickedSlottedItem(OwningIndex, GridSlot, SlottedItem, ItemData->GetGridDimensions());
	}
	else
	{
		SlottedItem->UpdateStackCount(GridSlot->StackCount); //we just reduce it, don't reduce twice lol	
	}
	
//step2: (REPLICATION ~ Drop, but we don't need to spawn BP_DroppedItem - we literally consume it)
	UInv_BPFunctionLibrary::GetInventoryComponentFromPC(GetOwningPlayer())->ServerRPC_ConsumeItem(ItemData, 1);
}


/*this function can be also re-used when you swap the WBP_PreoccupiedSlottedItem, specifically:
- the WBP_PreoccupiedSlottedItem will be removed and become WBP_PreoccupiedHover (replace WBP_HoverItem)
= this function will do it as long as you know info about this WBP_PreoccupiedSlottedItem (its index will be passed as ClicjedGridIndex and so on)
- the WBP_SlottedItem will be added back at WBP_PreoccupiedSlottedItem::Index (no need to add to FastArray because it was never removed)
= you must do this additionally (can re-use AddItemWidgetAtIndex by faking SlotInfo or refactor the body function to be re-used)
, I may consider to factorize it this time.
*/
bool UUW_Inv_InventoryGrid::CreateHoverItemAndRemoveClickedSlottedItem(int32 ClickedGridIndex, UUW_Inv_InventoryGridSlot* ClickedGridSlot, UUW_Inv_SlottedItem* ClickedSlottedItem, UItemData* ClickedItemData)
{
/*********YOU CAN FACTORIZE THE BELOW INTO "CreateHoverItemFromClickedSlot" if you want to"*************/
	/*STEP_C: create && set  WBP_HoverItem::Values exactly the way you did for WBP_SlottedItem::Values However this time slightly different
		- WBP_SlottedItem get all values mostly from SlotInfo and some from ItemData/ItemManifest
		- here we can steal some info from WBP_SlottedItem itself if you want
		- however the fun fact is that  WBP_SlottedItem currently only store GridIndex, bStackable, GridSize OwningItemData (not even have StackCount, nor cache TextureImage of Image_Icon)
		- hence we should simply do it the same way as WBP_SlottedItem from scratch lol (because better off do not choose a way between them that really hard for code maintenance lol, next time if you want to shadow - you better cache all such TextureImage, GridSize and all thing needed on WBP_SlottedItem - but in the end it is optional and cost unnecessarily memory - so stephen don't do it in this course)

		*The small difference: we don't add WBP_HoverItem as child of WBP_Grid::Canvas in the end
		* we SetMouseCursor(WBP_HoverItem) and so it moves where the mouse is (that a trick!)
		* because WBP_HoverItem is dynamically spawned directly to the ViewPort (not spawned and added as child of anything), hence we may need to multiply DrawSize with "ViewPortScale" (just test before)

		@@Idea:
		+copy all code from "AddWidgetItemToIndexFromSlotInfo" to ready to adapt
		+change "SlottedItem" to "HoverItem" (type/class)
		+cache in WBP_HoverItem instead of temp var
		+replace SlotInfo with information from ItemData/WBP_SlottedItems/Whatever as long as it works 
		*/
	const FItemFragment_Grid* ItemFragment_Grid = GetItemFragmentByTag<FItemFragment_Grid>(ClickedItemData, ItemFragmentTags::Fragment_Grid);
	//get Fragment_Image so that we have an image to show
	const FItemFragment_Image* ItemFragment_Image = GetItemFragmentByTag<FItemFragment_Image>(ClickedItemData, ItemFragmentTags::Fragment_Image);

	if (ItemFragment_Image == nullptr /*|| ItemFragment_Grid == nullptr */ ) return true;

	float GridPadding =  ItemFragment_Grid? ItemFragment_Grid->GridPadding : 0.f; //or a different "default" padding
	//~GridSize can be retrieved from WBP_SlottedItem::GridDimensions (but anyway I don't bother to do it)
	FIntPoint GridDimensions = ItemFragment_Grid? ItemFragment_Grid->GridDimensions : FIntPoint(1, 1);
	//~you can also get bStackable from WBP_SlottedItem::Stackable as well
	bool bStackable = ClickedItemData->IsStackable() /*AvailabilityInfo.bStackable*/;
			
	//step1: CreateWidget<WBP_Item>(WBP_Item_Class)
	WBP_HoverItem = CreateWidget<UUW_Inv_HoverItem>(GetOwningPlayer(), HoverItem_Class);

	//step2A: set WBP_Item::BindWidgets::Values 
	FSlateBrush IconBrush;
	IconBrush.ImageSize = GridDimensions * (GridSlotSize - GridPadding * 2.f); //STEPHEN
	IconBrush.ImageSize =
		FDeprecateSlateVector2D(GridDimensions * GridSlotSize) -
		FDeprecateSlateVector2D( GridPadding * 2.f, (GridPadding * 2.f)); //ME
	IconBrush.ImageSize = IconBrush.ImageSize  * UWidgetLayoutLibrary::GetViewportScale(this); //MODIFIED: this time it won't fit into Canvas (but follow MouseCursor) - hence stephen scale it so that WBP_HoverItem will have appropriate size as the Viewport is scaled weirdly (WBP_HoverItem global size currently determine by UImage_Icon::ImageSize as it doesn't have SizeBox and it dynamically spawned to viewport on MouseCursor location)			
			
	IconBrush.SetResourceObject(ItemFragment_Image->Icon);
	IconBrush.DrawAs = ESlateBrushDrawType::Type::Image;
	WBP_HoverItem->SetImageIcon(IconBrush);
	WBP_HoverItem->UpdateStackCount( bStackable?  ClickedGridSlot->StackCount /*SlotInfo.AmountToFill*/ : 0); //0 so that it collapses

	//step2B: set WBP_Item::SideValues. I set WBP_HoverItem::StackCount in UpdateStackCount above already!
	WBP_HoverItem->GridIndex = ClickedGridSlot->GridSlotIndex /*SlotInfo.SlotArrayIndex*/;
	WBP_HoverItem->bStackable = bStackable /*AvailabilityInfo.bStackable*/; 
	WBP_HoverItem->GridDimensions = GridDimensions;
	WBP_HoverItem->OwningItemData = ClickedItemData; 

	/*Step2C: (NEW) bind WBP_Grid::callback to WBP_HoverItem::Delegate, doing it here mean this same callback is bound to all created WBP_HoverItem in inventory (and it is bound right WBP_HoverItem creation
		 , even before it is being added as child of canvas and it is totally fine, why not) 
			//WBP_HoverItem->OnHoverItemClickedDelegate.AddDynamic(this, &ThisClass::OnHoverItemClickedCallback);
		*/
		
	/*Step3&4: this time you don't need CanvasSize nor DrawPosition because WBP_HoverItem will follow MouseLocation!*/
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Type::Default, WBP_HoverItem);
		
	/*step5: bookkeeping? it is stored right at step1 above lol! **/

/***************YOU CAN FACTORIZE THE BELOW TO "RemoveClickedSlottedItem" if you want to*****/
	/*step6: change SlotState of occupied WBP_GridSlot[s] by GridSize (WBP_Grid::GridSlots)
	    && set its ::Values*/
	ClickedGridSlot->StackCount = 0; //I forget this step
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>(
		GridSlots, ClickedGridSlot->GridSlotIndex /*SlotInfo.SlotArrayIndex*/, GridDimensions, columns,
		[&](UUW_Inv_InventoryGridSlot* WBP_GridSlot)
		{
			if (IsValid(WBP_GridSlot) == false) return;
			WBP_GridSlot->SetSlotStateAndBrush(ESlotState::Unoccupied); //Occupied back to Unoccupied
			WBP_GridSlot->bAvailable = true; //false back to true
			WBP_GridSlot->OwningItemData.Reset(); //ClickedItemData back to "nullptr"
			WBP_GridSlot->UpperLeftIndex = INDEX_NONE /*SlotInfo.SlotArrayIndex*/; // ClickedGridSlot->GridSlotIndex back to INDEX_NONE
			//WBP_GridSlot->StackCount = 0; //this is overkill better off do it on the upperleft gridslot only
		}
	);
	/*STEP_D: remove the WBP_SlottedItem from WBP_Grid::Canvas and so SlottedItemMap (you're not gonna remove WBP_GridSlot lol, you only change its values and background brush like above)
	- Meaning FastArray still contain the Owning ItemData, either we spawn WBP_SlottedItem back (say to new location in canvas) or destroy it is up to whether you drag it out of Inventory or else!
	- this also work:
			TObjectPtr<UUW_Inv_SlottedItem> OutValue;
			SlottedItemMap.RemoveAndCopyValue(ClickedGridIndex, OutValue);
			OutValue->RemoveFromParent();
	 
	 */
	SlottedItemMap.Remove(ClickedGridIndex);
	ClickedSlottedItem->RemoveFromParent();   //CanvasPanel_GridSlots->RemoveChild(ClickedSlottedItem); also works, but not preferred!
	return false;
}

void UUW_Inv_InventoryGrid::CreateHoverItem(UUW_Inv_InventoryGridSlot* ClickedGridSlot, UItemData* ClickedItemData, int32 StackOverride)
{
		const FItemFragment_Grid* ItemFragment_Grid = GetItemFragmentByTag<FItemFragment_Grid>(ClickedItemData, ItemFragmentTags::Fragment_Grid);
	//get Fragment_Image so that we have an image to show
	const FItemFragment_Image* ItemFragment_Image = GetItemFragmentByTag<FItemFragment_Image>(ClickedItemData, ItemFragmentTags::Fragment_Image);

	if (ItemFragment_Image == nullptr /*|| ItemFragment_Grid == nullptr */ ) return;

	float GridPadding =  ItemFragment_Grid? ItemFragment_Grid->GridPadding : 0.f; //or a different "default" padding
	//~GridSize can be retrieved from WBP_SlottedItem::GridDimensions (but anyway I don't bother to do it)
	FIntPoint GridDimensions = ItemFragment_Grid? ItemFragment_Grid->GridDimensions : FIntPoint(1, 1);
	//~you can also get bStackable from WBP_SlottedItem::Stackable as well
	bool bStackable = ClickedItemData->IsStackable() /*AvailabilityInfo.bStackable*/;
			
	//step1: CreateWidget<WBP_Item>(WBP_Item_Class)
	WBP_HoverItem = CreateWidget<UUW_Inv_HoverItem>(GetOwningPlayer(), HoverItem_Class);

	//step2A: set WBP_Item::BindWidgets::Values 
	FSlateBrush IconBrush;
	IconBrush.ImageSize = GridDimensions * (GridSlotSize - GridPadding * 2.f); //STEPHEN
	IconBrush.ImageSize =
		FDeprecateSlateVector2D(GridDimensions * GridSlotSize) -
		FDeprecateSlateVector2D( GridPadding * 2.f, (GridPadding * 2.f)); //ME
	IconBrush.ImageSize = IconBrush.ImageSize  * UWidgetLayoutLibrary::GetViewportScale(this); //MODIFIED: this time it won't fit into Canvas (but follow MouseCursor) - hence stephen scale it so that WBP_HoverItem will have appropriate size as the Viewport is scaled weirdly (WBP_HoverItem global size currently determine by UImage_Icon::ImageSize as it doesn't have SizeBox and it dynamically spawned to viewport on MouseCursor location)			
			
	IconBrush.SetResourceObject(ItemFragment_Image->Icon);
	IconBrush.DrawAs = ESlateBrushDrawType::Type::Image;
	WBP_HoverItem->SetImageIcon(IconBrush);

	//modified:
	if (StackOverride >= 0)
	{
		WBP_HoverItem->UpdateStackCount(StackOverride);
	}
	else
	{
		WBP_HoverItem->UpdateStackCount( bStackable?  ClickedGridSlot->StackCount /*SlotInfo.AmountToFill*/ : 0); //0 so that it collapsed	
	}

	//step2B: set WBP_Item::SideValues. I set WBP_HoverItem::StackCount in UpdateStackCount above already!
	WBP_HoverItem->GridIndex = ClickedGridSlot->GridSlotIndex /*SlotInfo.SlotArrayIndex*/;
	WBP_HoverItem->bStackable = bStackable /*AvailabilityInfo.bStackable*/; 
	WBP_HoverItem->GridDimensions = GridDimensions;
	WBP_HoverItem->OwningItemData = ClickedItemData; 

	/*Step2C: (NEW) bind WBP_Grid::callback to WBP_HoverItem::Delegate, doing it here mean this same callback is bound to all created WBP_HoverItem in inventory (and it is bound right WBP_HoverItem creation
		 , even before it is being added as child of canvas and it is totally fine, why not) 
			//WBP_HoverItem->OnHoverItemClickedDelegate.AddDynamic(this, &ThisClass::OnHoverItemClickedCallback);
		*/
		
	/*Step3&4: this time you don't need CanvasSize nor DrawPosition because WBP_HoverItem will follow MouseLocation!*/
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Type::Default, WBP_HoverItem);
		
	/*step5: bookkeeping? it is stored right at step1 above lol! **/
}

void UUW_Inv_InventoryGrid::RemoveClickedSlottedItem(int32 ClickedGridIndex, UUW_Inv_InventoryGridSlot* ClickedGridSlot, UUW_Inv_SlottedItem* ClickedSlottedItem, FIntPoint GridDimensions)
{
	ClickedGridSlot->StackCount = 0; //I forget this step
	UInv_BPFunctionLibrary::ForEach2D<UUW_Inv_InventoryGridSlot*>(
		GridSlots, ClickedGridSlot->GridSlotIndex /*SlotInfo.SlotArrayIndex*/, GridDimensions, columns,
		[&](UUW_Inv_InventoryGridSlot* WBP_GridSlot)
		{
			if (IsValid(WBP_GridSlot) == false) return;
			WBP_GridSlot->SetSlotStateAndBrush(ESlotState::Unoccupied); //Occupied back to Unoccupied
			WBP_GridSlot->bAvailable = true; //false back to true
			WBP_GridSlot->OwningItemData.Reset(); //ClickedItemData back to "nullptr"
			WBP_GridSlot->UpperLeftIndex = INDEX_NONE /*SlotInfo.SlotArrayIndex*/; // ClickedGridSlot->GridSlotIndex back to INDEX_NONE
			//WBP_GridSlot->StackCount = 0; //this is overkill better off do it on the upperleft gridslot only
		}
	);
	/*STEP_D: remove the WBP_SlottedItem from WBP_Grid::Canvas and so SlottedItemMap (you're not gonna remove WBP_GridSlot lol, you only change its values and background brush like above)
	- Meaning FastArray still contain the Owning ItemData, either we spawn WBP_SlottedItem back (say to new location in canvas) or destroy it is up to whether you drag it out of Inventory or else!
	- this also work:
			TObjectPtr<UUW_Inv_SlottedItem> OutValue;
			SlottedItemMap.RemoveAndCopyValue(ClickedGridIndex, OutValue);
			OutValue->RemoveFromParent();
	 
	 */
	//the order may matter, because reference from Container may keep the widget alive unintentionally, so perhaps remove this reference first.
	SlottedItemMap.Remove(ClickedGridIndex); 
	ClickedSlottedItem->RemoveFromParent();   //CanvasPanel_GridSlots->RemoveChild(ClickedSlottedItem); also works, but not preferred!
}

/*MoveTemp(InObject) <=> std::move(InObject) in C++
	 *
	 *Move(X) <=> This is NOT meant for local variables in functions, it is more like "Move" function
	Conditionally moves
	Only moves if X is not const
	Used for perfect-forwarding / template code
	Almost never used in regular Unreal code
	Simplified:
		template <typename T>
		decltype(auto) Move(T&& Obj)
		{
			return static_cast<T&&>(Obj);
		}
	*
		HERE:
		AvailabilityInfo.SlotInfos.Add(MoveTemp(SlotInfo)); //OPTION2 - better performance in this case (since FInventorySlotInfo is not a huge struct, so not that different in performance lol, but anyway)
		AvailabilityInfo.SlotInfos.Add(Move(SlotInfo));     //OPTION3 - not applied to local var, RED error


FInventoryAvailabilityInfo AvailabilityInfo;
	AvailabilityInfo.TotalRoomToFill = 15; //suggesting it can fill 15 with AvailabilityInfo::Remainder after checking?
	AvailabilityInfo.bStackable = true;

	//one more update for testing purpose
	FInventorySlotInfo SlotInfo1;
		SlotInfo1.SlotArrayIndex = 0;
		SlotInfo1.AmountToFill = 5;     //won't make sense for non-stackable items
		SlotInfo1.IsItemAtIndex =false; //by default
	AvailabilityInfo.SlotInfos.Add(SlotInfo1);    //OPTION1: this make a copy, work fine

	FInventorySlotInfo SlotInfo2;
	SlotInfo2.SlotArrayIndex = 1;
	SlotInfo2.AmountToFill = 10;     //won't make sense for non-stackable items
	SlotInfo2.IsItemAtIndex =false;
	AvailabilityInfo.SlotInfos.Add(MoveTemp(SlotInfo2));
	
	return AvailabilityInfo;
	*/


