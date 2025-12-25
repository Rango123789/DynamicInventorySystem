// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/*note:
 *you would love to add "Item." as well, and then use:
- void Function( UPARAM(meta = (Categories = "ItemTags")) FGameplayTag InTag) - for function param
- UPROPERTY(EditAnywhere, meta = (Categories="Equippable.Armor")) FGameplayTag MyTag - for member
= so that you can only pick those tab from C++
*/
namespace ItemTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equippable_Weapon_Axe);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equippable_Weapon_Sword);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equippable_Armor_Cloak_Red);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equippable_Armor_Mask_Steel);	

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Potion_Small_Red);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Potion_Small_Blue);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Potion_Large_Red);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Potion_Large_Blue);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Craftable_FireFernFruit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Craftable_LuminDaisy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Craftable_ScorchPetalBlossom);
}

namespace ItemFragmentTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fragment_Grid);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fragment_Image);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fragment_Stackable);	
}

/* I dont like this
namespace ItemTags
{
	namespace Equippable
	{
		namespace Weapon
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Axe)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sword)
		}

		namespace Armor
		{
			namespace Cloak
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Red)
			}

			namespace Mask
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Steel)				
			}
		}
	}

	namespace Consumable
	{
		namespace Potion
		{
			namespace Small
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Red)
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blue)
			}

			namespace Medium
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Red)
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blue)				
			}

			namespace Large
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Red)
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blue)
			}
		}
		
	}

	namespace Craftable
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(FireFernFruit)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LuminDaisy)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ScorchPetalBlossom)
	}
}

*/