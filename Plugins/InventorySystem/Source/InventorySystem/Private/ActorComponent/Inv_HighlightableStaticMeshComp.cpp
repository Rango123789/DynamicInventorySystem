// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponent/Inv_HighlightableStaticMeshComp.h"

void UInv_HighlightableStaticMeshComp::Highlight_Implementation()
{
	SetOverlayMaterial(MyOverlayMaterial);
}

void UInv_HighlightableStaticMeshComp::UnHighlight_Implementation()
{
	SetOverlayMaterial(nullptr);
}
