// Fill out your copyright notice in the Description page of Project Settings.


#include "BCPlayerState.h"

#include "Net/UnrealNetwork.h"

ABCPlayerState::ABCPlayerState(): PlayerNameString(TEXT("None")), CurrentGuessCount(0), MaxGuessCount(3)
{
	bReplicates = true;
}

void ABCPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerNameString);
	DOREPLIFETIME(ThisClass, CurrentGuessCount);
	DOREPLIFETIME(ThisClass, MaxGuessCount);
}

FString ABCPlayerState::GetPlayerInfoString()
{
	int32 DisplayGuessCount = FMath::Clamp(CurrentGuessCount + 1, 1, MaxGuessCount);
	FString PlayerInfoString = PlayerNameString + TEXT("(") + FString::FromInt(DisplayGuessCount) + TEXT("/") + FString::FromInt(MaxGuessCount) + TEXT(")");
	return PlayerInfoString;
}
