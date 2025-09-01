// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BCGameModeBase.h"

#include "BCGameStateBase.h"
#include "EngineUtils.h"
#include "Player/BCPlayerController.h"
#include "Player/BCPlayerState.h"
#include "TimerManager.h"

void ABCGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);
	
	ABCPlayerController* CXPlayerController = Cast<ABCPlayerController>(NewPlayer);
	if (IsValid(CXPlayerController) == true)
	{
		CXPlayerController->NotificationText = FText::FromString(TEXT("Connected to the game server."));
		
		AllPlayerControllers.Add(CXPlayerController);

		ABCPlayerState* CXPS = CXPlayerController->GetPlayerState<ABCPlayerState>();
		if (IsValid(CXPS) == true)
		{
			CXPS->PlayerNameString = TEXT("Player") + FString::FromInt(AllPlayerControllers.Num());
		}

		ABCGameStateBase* CXGameStateBase =  GetGameState<ABCGameStateBase>();
		if (IsValid(CXGameStateBase) == true)
		{
			CXGameStateBase->MulticastRPCBroadcastLoginMessage(CXPS->PlayerNameString);
		}

		// Start first turn once at least one player is present
		StartFirstTurnIfNeeded();
	}
}

void ABCGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();
}

void ABCGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear any pending turn timer to avoid callbacks after teardown
	if (TurnTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	}
	CurrentTurnIndex = INDEX_NONE;
	AllPlayerControllers.Empty();

	Super::EndPlay(EndPlayReason);
}

void ABCGameModeBase::PrintChatMessageString(ABCPlayerController* InChattingPlayerController,
                                             const FString& InChatMessageString)
{
	FString ChatMessageString = InChatMessageString;
	int Index = InChatMessageString.Len() - 3;
	FString GuessNumberString = InChatMessageString.RightChop(Index);
	if (IsGuessNumberString(GuessNumberString) == true)
	{
		// Only the current player's guess is accepted during their turn
		if (IsPlayersTurn(InChattingPlayerController) == false)
		{
			if (IsValid(InChattingPlayerController))
			{
				InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("It's not your turn."));
			}
			return;
		}

		FString JudgeResultString = JudgeResult(SecretNumberString, GuessNumberString);
		IncreaseGuessCount(InChattingPlayerController);
		for (TActorIterator<ABCPlayerController> It(GetWorld()); It; ++It)
		{
			ABCPlayerController* BCPlayerController = *It;
			if (IsValid(BCPlayerController) == true)
			{
				FString CombinedMessageString = InChatMessageString + TEXT(" -> ") + JudgeResultString;
				BCPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
			}
		}
		
		int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
		JudgeGame(InChattingPlayerController, StrikeCount);
		
		// If not a winning guess, advance to next player's turn immediately
		if (StrikeCount != 3)
		{
			AdvanceTurn();
		}
	}
	else
	{
		for (TActorIterator<ABCPlayerController> It(GetWorld()); It; ++It)
		{
			ABCPlayerController* BCPlayerController = *It;
			if (IsValid(BCPlayerController) == true)
			{
				BCPlayerController->ClientRPCPrintChatMessageString(InChatMessageString);
			}
		}
	}
}

FString ABCGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(FDateTime::Now().GetTicks());
	Numbers = Numbers.FilterByPredicate([](int32 Num) { return Num > 0; });
	
	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}

	return Result;
}

bool ABCGameModeBase::IsGuessNumberString(const FString& InNumberString)
{
	bool bCanPlay = false;

	do {

		if (InNumberString.Len() != 3)
		{
			break;
		}

		bool bIsUnique = true;
		TSet<TCHAR> UniqueDigits;
		for (TCHAR C : InNumberString)
		{
			if (FChar::IsDigit(C) == false || C == '0')
			{
				bIsUnique = false;
				break;
			}
			
			UniqueDigits.Add(C);
		}

		if (bIsUnique == false)
		{
			break;
		}

		bCanPlay = true;
		
	} while (false);

	return bCanPlay;
}

FString ABCGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0, BallCount = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			StrikeCount++;
		}
		else 
		{
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			if (InSecretNumberString.Contains(PlayerGuessChar))
			{
				BallCount++;
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ABCGameModeBase::IncreaseGuessCount(ABCPlayerController* InChattingPlayerController)
{
	ABCPlayerState* CXPS = InChattingPlayerController->GetPlayerState<ABCPlayerState>();
	if (IsValid(CXPS) == true)
	{
		CXPS->CurrentGuessCount++;
	}
}

void ABCGameModeBase::ResetGame()
{
	SecretNumberString = GenerateSecretNumber();

	for (const auto& CXPlayerController : AllPlayerControllers)
	{
		ABCPlayerState* CXPS = CXPlayerController->GetPlayerState<ABCPlayerState>();
		if (IsValid(CXPS) == true)
		{
			CXPS->CurrentGuessCount = 0;
		}
	}

	// Restart turns for a new round, keeping current turn owner if present
	if (AllPlayerControllers.Num() > 0)
	{
		StartTurnForIndex(CurrentTurnIndex == INDEX_NONE ? 0 : CurrentTurnIndex);
	}
}

void ABCGameModeBase::JudgeGame(ABCPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (3 == InStrikeCount)
	{
		ABCPlayerState* CXPS = InChattingPlayerController->GetPlayerState<ABCPlayerState>();
		for (const auto& CXPlayerController : AllPlayerControllers)
		{
			if (IsValid(CXPS) == true)
			{
				FString CombinedMessageString = CXPS->PlayerNameString + TEXT(" has won the game.");
				CXPlayerController->NotificationText = FText::FromString(CombinedMessageString);

				ResetGame();
			}
		}
	}
	else
	{
		bool bIsDraw = true;
		for (const auto& CXPlayerController : AllPlayerControllers)
		{
			ABCPlayerState* CXPS = CXPlayerController->GetPlayerState<ABCPlayerState>();
			if (IsValid(CXPS) == true)
			{
				if (CXPS->CurrentGuessCount < CXPS->MaxGuessCount)
				{
					bIsDraw = false;
					break;
				}
			}
		}

		if (true == bIsDraw)
		{
			for (const auto& CXPlayerController : AllPlayerControllers)
			{
				CXPlayerController->NotificationText = FText::FromString(TEXT("Draw..."));

				ResetGame();
			}
		}
	}
}

// Turn-based helpers
void ABCGameModeBase::StartFirstTurnIfNeeded()
{
	if (CurrentTurnIndex == INDEX_NONE && AllPlayerControllers.Num() > 0)
	{
		StartTurnForIndex(0);
	}
}

void ABCGameModeBase::StartTurnForIndex(int32 NewIndex)
{
	// Remove invalid controllers
	AllPlayerControllers.RemoveAll([](const TObjectPtr<ABCPlayerController>& PC){ return !IsValid(PC); });

	if (AllPlayerControllers.Num() == 0)
	{
		CurrentTurnIndex = INDEX_NONE;
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);
		return;
	}

	CurrentTurnIndex = (NewIndex % AllPlayerControllers.Num() + AllPlayerControllers.Num()) % AllPlayerControllers.Num();
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ABCGameModeBase::OnTurnTimeout, TurnTimeLimitSeconds, false);

	// Announce whose turn it is
	ABCPlayerController* TurnOwner = AllPlayerControllers[CurrentTurnIndex];
	ABCPlayerState* TurnPS = IsValid(TurnOwner) ? TurnOwner->GetPlayerState<ABCPlayerState>() : nullptr;
	FString TurnOwnerName = IsValid(TurnPS) ? TurnPS->PlayerNameString : TEXT("Player");
	FString TurnMsg = FString::Printf(TEXT("It's %s's turn (%ds)."), *TurnOwnerName, (int32)TurnTimeLimitSeconds);
	for (TActorIterator<ABCPlayerController> It(GetWorld()); It; ++It)
	{
		ABCPlayerController* BCPlayerController = *It;
		if (IsValid(BCPlayerController))
		{
			BCPlayerController->ClientRPCPrintChatMessageString(TurnMsg);
		}
	}
}

void ABCGameModeBase::AdvanceTurn()
{
	if (AllPlayerControllers.Num() == 0)
	{
		return;
	}
	if (CurrentTurnIndex == INDEX_NONE)
	{
		StartTurnForIndex(0);
		return;
	}
	StartTurnForIndex((CurrentTurnIndex + 1) % AllPlayerControllers.Num());
}

void ABCGameModeBase::OnTurnTimeout()
{
	if (AllPlayerControllers.Num() == 0 || CurrentTurnIndex == INDEX_NONE)
	{
		return;
	}
	ABCPlayerController* TurnOwner = AllPlayerControllers[CurrentTurnIndex];
	ABCPlayerState* TurnPS = IsValid(TurnOwner) ? TurnOwner->GetPlayerState<ABCPlayerState>() : nullptr;
	FString TurnOwnerName = IsValid(TurnPS) ? TurnPS->PlayerNameString : TEXT("Player");

	FString TimeoutMsg = FString::Printf(TEXT("%s timed out. Passing the turn."), *TurnOwnerName);
	for (TActorIterator<ABCPlayerController> It(GetWorld()); It; ++It)
	{
		ABCPlayerController* BCPlayerController = *It;
		if (IsValid(BCPlayerController))
		{
			BCPlayerController->ClientRPCPrintChatMessageString(TimeoutMsg);
		}
	}
	
	AdvanceTurn();
}

bool ABCGameModeBase::IsPlayersTurn(ABCPlayerController* PC) const
{
	if (CurrentTurnIndex == INDEX_NONE || AllPlayerControllers.Num() == 0 || PC == nullptr)
	{
		return false;
	}
	if (AllPlayerControllers.IsValidIndex(CurrentTurnIndex) == false)
	{
		return false;
	}
	return AllPlayerControllers[CurrentTurnIndex] == PC;
}
