// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BCPlayerController.h"

#include "BCPlayerState.h"
#include "BullsAndCows.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"
#include "Game/BCGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "UI/BCChatInput.h"

ABCPlayerController::ABCPlayerController()
{
	bReplicates = true;
}

void ABCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}
	
	FInputModeUIOnly InputModeUIOnly;
	SetInputMode(InputModeUIOnly);

	if (IsValid(ChatInputWidgetClass) == true)
	{
		ChatInputWidgetInstance = CreateWidget<UBCChatInput>(this, ChatInputWidgetClass);
		if (IsValid(ChatInputWidgetInstance) == true)
		{
			ChatInputWidgetInstance->AddToViewport();
		}
	}
	
	if (IsValid(NotificationTextWidgetClass) == true)
	{
		NotificationTextWidgetInstance = CreateWidget<UUserWidget>(this, NotificationTextWidgetClass);
		if (IsValid(NotificationTextWidgetInstance) == true)
		{
			NotificationTextWidgetInstance->AddToViewport();
		}
	}
}

void ABCPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, NotificationText);
}


void ABCPlayerController::SetChatMessageString(const FString& InChatMessageString)
{
	ChatMessageString = InChatMessageString;
	
	if (IsLocalController() == true)
	{
		ABCPlayerState* CXPS = GetPlayerState<ABCPlayerState>();
		if (IsValid(CXPS) == true)
		{
			FString CombinedMessageString = CXPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString;

			ServerRPCPrintChatMessageString(CombinedMessageString);
		}
	}
}

void ABCPlayerController::PrintChatMessageString(const FString& InChatMessageString)
{
	BCFunctionLibrary::BCPrintString(this, InChatMessageString, 10.0f);
}

void ABCPlayerController::ServerRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	
	AGameModeBase* GM = UGameplayStatics::GetGameMode(this);
	if (IsValid(GM) == true)
	{
		ABCGameModeBase* BCGM = Cast<ABCGameModeBase>(GM);
		if (IsValid(BCGM) == true)
		{
			BCGM->PrintChatMessageString(this, InChatMessageString);
		}
	}
}

void ABCPlayerController::ClientRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	PrintChatMessageString(InChatMessageString);
}
