// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BCGameModeBase.generated.h"

class ABCPlayerController;
/**
 * 
 */
UCLASS()
class BULLSANDCOWS_API ABCGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void OnPostLogin(AController* NewPlayer) override;	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void PrintChatMessageString(ABCPlayerController* InChattingPlayerController, const FString& InChatMessageString);
	
public:
	FString GenerateSecretNumber();
	bool IsGuessNumberString(const FString& InNumberString);
	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);

	void IncreaseGuessCount(ABCPlayerController* InChattingPlayerController);
	
	void ResetGame();

	void JudgeGame(ABCPlayerController* InChattingPlayerController, int InStrikeCount);
protected:
	FString SecretNumberString;

	TArray<TObjectPtr<ABCPlayerController>> AllPlayerControllers;

	// Turn-based play state
protected:
	int32 CurrentTurnIndex = INDEX_NONE;
	FTimerHandle TurnTimerHandle;
	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	float TurnTimeLimitSeconds = 30.f;

	void StartFirstTurnIfNeeded();
	void StartTurnForIndex(int32 NewIndex);
	void AdvanceTurn();
	void OnTurnTimeout();
	bool IsPlayersTurn(ABCPlayerController* PC) const;
};
