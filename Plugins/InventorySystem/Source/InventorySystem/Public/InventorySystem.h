// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

//again if it asks for "ETraceChannel" - start from 1->n , it asks for "ETraceQueryType" start from "3" (consider TraceType only) and in this case you don't care about this constant macro:
#define ECC_ItemTrace ECollisionChannel::ECC_GameTraceChannel1

DECLARE_LOG_CATEGORY_EXTERN(LogInventory, Log, All);
//DEFINE_LOG_CATEGORY(LogInventory); //define it here does get a warning

namespace DebugHelpers
{
	void Print(const FString& DebugMessage, int32 ColorIndex = 0, int32 InKey = -1);
}


class FInventorySystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
