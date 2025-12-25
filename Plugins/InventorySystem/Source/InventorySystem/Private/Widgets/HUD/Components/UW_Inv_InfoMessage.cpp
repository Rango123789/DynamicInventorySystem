// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/Components/UW_Inv_InfoMessage.h"

#include "Components/TextBlock.h"

void UUW_Inv_InfoMessage::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	TextBlock_Message->SetText(FText::GetEmpty());
	MessageHide();
}

void UUW_Inv_InfoMessage::SetMessageAndSetTimer(const FText& NewMessage)
{
//set text:
	TextBlock_Message->SetText(NewMessage);

	//by OPTION2: animation is running, so if you don't check it may re-play animation again as we spam the "E" key
	if (bIsShown == false)
	{
		//SetVisibility(ESlateVisibility::HitTestInvisible); //OPTION1
		MessageShow();
		bIsShown = true;
	}
	
//set timer to hide it
	GetWorld()->GetTimerManager().SetTimer(
	TimerHandle,
	[this]()
		{
			//SetVisibility(ESlateVisibility::Collapsed); //OPTION1
			MessageHide();
			bIsShown = false;
		},
		ShowTime, false
	);
}
