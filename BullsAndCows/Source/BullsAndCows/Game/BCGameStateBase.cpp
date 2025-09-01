// Fill out your copyright notice in the Description page of Project Settings.


#include "BCGameStateBase.h"

#include "Kismet/GameplayStatics.h"
#include "Player/BCPlayerController.h"

void ABCGameStateBase::MulticastRPCBroadcastLoginMessage_Implementation(const FString& InNameString)
{
	if (HasAuthority() == false)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (IsValid(PC) == true)
		{
			ABCPlayerController* CXPC = Cast<ABCPlayerController>(PC);
			if (IsValid(CXPC) == true)
			{
				FString NotificationString = InNameString + TEXT(" has joined the game.");
				CXPC->PrintChatMessageString(NotificationString);
			}
		}
	}
}
