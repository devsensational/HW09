// Fill out your copyright notice in the Description page of Project Settings.


#include "BCPawn.h"

#include "BullsAndCows.h"


void ABCPawn::BeginPlay()
{
	Super::BeginPlay();
	
	FString NetModeString = BCFunctionLibrary::GetRoleString(this);
	FString CombinedString = FString::Printf(TEXT("CXPawn::BeginPlay() %s [%s]"), *BCFunctionLibrary::GetNetModeString(this), *NetModeString);
	BCFunctionLibrary::BCPrintString(this, CombinedString, 10.f);
}

void ABCPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	FString NetModeString = BCFunctionLibrary::GetRoleString(this);
	FString CombinedString = FString::Printf(TEXT("CXPawn::PossessedBy() %s [%s]"), *BCFunctionLibrary::GetNetModeString(this), *NetModeString);
	BCFunctionLibrary::BCPrintString(this, CombinedString, 10.f);
}
