// Fill out your copyright notice in the Description page of Project Settings.
#include "InventoryTags/InventoryTags.h"

namespace ItemTags
{
	UE_DEFINE_GAMEPLAY_TAG(Item_Equippable_Weapon_Axe, "Item.Equippable.Weapon.Axe");
	UE_DEFINE_GAMEPLAY_TAG(Item_Equippable_Weapon_Sword, "Item.Equippable.Weapon.Sword");

	UE_DEFINE_GAMEPLAY_TAG(Item_Equippable_Armor_Cloak_Red, "Item.Equippable.Armor.Cloak.Red");
	UE_DEFINE_GAMEPLAY_TAG(Item_Equippable_Armor_Mask_Steel, "Item.Equippable.Armor.Mask.Steel");	

	UE_DEFINE_GAMEPLAY_TAG(Item_Consumable_Potion_Small_Red, "Item.Consumable.Potion.Small.Red");
	UE_DEFINE_GAMEPLAY_TAG(Item_Consumable_Potion_Small_Blue, "Item.Consumable.Potion.Small.Blue");
	UE_DEFINE_GAMEPLAY_TAG(Item_Consumable_Potion_Large_Red, "Item.Consumable.Potion.Large.Red");
	UE_DEFINE_GAMEPLAY_TAG(Item_Consumable_Potion_Large_Blue, "Item.Consumable.Potion.Large.Blue");

	UE_DEFINE_GAMEPLAY_TAG(Item_Craftable_FireFernFruit, "Item.Craftable.FireFernFruit");
	UE_DEFINE_GAMEPLAY_TAG(Item_Craftable_LuminDaisy, "Item.Craftable.LuminDaisy");
	UE_DEFINE_GAMEPLAY_TAG(Item_Craftable_ScorchPetalBlossom, "Item.Craftable.ScorchPetalBlossom");
}

namespace ItemFragmentTags
{
	UE_DEFINE_GAMEPLAY_TAG(Fragment_Grid, "Fragment.Grid");
	UE_DEFINE_GAMEPLAY_TAG(Fragment_Image, "Fragment.Image");
	UE_DEFINE_GAMEPLAY_TAG(Fragment_Stackable, "Fragment.Stackable");
}

/* I don't like this
namespace ItemTags
{
	namespace Equippable
	{
		namespace Weapon
		{
			UE_DEFINE_GAMEPLAY_TAG(Axe, "Equippable.Weapon.Axe")
			UE_DEFINE_GAMEPLAY_TAG(Sword, "Equippable.Weapon.Sword")
		}

		namespace Armor
		{
			namespace Cloak
			{
				UE_DEFINE_GAMEPLAY_TAG(Red)
				UE_DEFINE_GAMEPLAY_TAG(Axe, "Equippable.Axe")
			}

			namespace Mask
			{
				UE_DEFINE_GAMEPLAY_TAG(Steel)				
			}
		}
	}

	namespace Consumable
	{
		namespace Potion
		{
			namespace Small
			{
				UE_DEFINE_GAMEPLAY_TAG(Red)
				UE_DEFINE_GAMEPLAY_TAG(Blue)
			}

			namespace Medium
			{
				UE_DEFINE_GAMEPLAY_TAG(Red)
				UE_DEFINE_GAMEPLAY_TAG(Blue)				
			}

			namespace Large
			{
				UE_DEFINE_GAMEPLAY_TAG(Red)
				UE_DEFINE_GAMEPLAY_TAG(Blue)
			}
		}
		
	}

	namespace Craftable
	{
		UE_DEFINE_GAMEPLAY_TAG(FireFernFruit)
		UE_DEFINE_GAMEPLAY_TAG(LuminDaisy)
		UE_DEFINE_GAMEPLAY_TAG(ScorchPetalBlossom)
	}
}
*/