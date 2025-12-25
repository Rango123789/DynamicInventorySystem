// Fill out your copyright notice in the Description page of Project Settings.
#include "Widgets/Inventory/Components/UW_Inv_InventoryGrid.h"

#include "InventorySystem.h"
#include "ActorComponent/Inv_InventoryComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryTags/InventoryTags.h"
#include "Items/ItemData.h"
#include "Utils/Inv_BPFunctionLibrary.h"
#include "Widgets/Inventory/Components/UW_Inv_InventoryGridSlot.h"
#include "Widgets/Inventory/Components/UW_Inv_SlottedItem.h"
#include "Items/ItemManifest.h"

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
		}
	}	
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
						
	/*3. Does it belongs to an UpperLeftSlot = this can be done after check IsStackable for the case "stackable item" that occupied GridSize > {1,1}. But you know what we can also pick a lot of non-stackable items at the same time right (it doesn't appear to be the case in this course, but who know you want to expand it!), hence I decide to check it first
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

	//the NormalizedPosition is (X,Y) , hence Row is Y lol
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
	//step0: [this can be ouTside the loop, but I move it in for READIBILITY and REUSABILITY) ready ItemData::ItemManifest::ItemFragment_1,2,3... (shared for all AvailabilityInfo/Item::SlotInfo)
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
			WBP_GridSlot->bAvailable = false;
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