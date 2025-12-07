// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventorySystem.h"

#define LOCTEXT_NAMESPACE "FInventorySystemModule"

DEFINE_LOG_CATEGORY(LogInventory);

namespace DebugHelpers
{
	//1.funny we're not define it here (cause compile error)
	void Print(const FString& DebugMessage, int32 ColorIndex, int32 InKey)
	{
		FColor Color{};
		if (ColorIndex == 0 || ColorIndex > 8) Color = FColor::MakeRandomColor();
		if (ColorIndex == 1) Color = FColor::Red;
		if (ColorIndex == 2) Color = FColor::Blue;
		if (ColorIndex == 3) Color = FColor::Green;
		if (ColorIndex == 4) Color = FColor::Cyan;
		if (ColorIndex == 5) Color = FColor::Yellow;
		if (ColorIndex == 6) Color = FColor::Magenta;
		if (ColorIndex == 7) Color = FColor::Black;
		if (ColorIndex == 8) Color = FColor::White;	
	
		//print to Screen:
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(InKey, 5.f, Color , DebugMessage);
		}
		//print to log:
		UE_LOG(LogInventory, Warning, TEXT("%s"), *DebugMessage);
	}
}

//2. we must define it like this as Rider suggests, here DebugHelpers:: act as "namespace" (not class)
//3. do not repeat "static" key in .cpp, otherwise it thought you create a new one lol. "exact function signature" with one with "static" and one not consider 2 separate overload (as one can call by Class::F(), where the other call from Instance::F())

void FInventorySystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FInventorySystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInventorySystemModule, InventorySystem)