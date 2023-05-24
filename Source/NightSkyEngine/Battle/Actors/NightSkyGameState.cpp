﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "NightSkyGameState.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "NightSkyPlayerController.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "NightSkyEngine/NightSkyGameInstance.h"

// Sets default values
ANightSkyGameState::ANightSkyGameState()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANightSkyGameState::BeginPlay()
{
	Super::BeginPlay();

	Init();
}

void ANightSkyGameState::Init()
{
	UNightSkyGameInstance* GameInstance = Cast<UNightSkyGameInstance>(GetGameInstance());
	
	for (int i = 0; i < 6; i++)
	{
		if (GameInstance != nullptr)
		{
			if (GameInstance->PlayerList.Num() > i)
			{
				if (GameInstance->PlayerList[i] != nullptr)
				{
					Players[i] = GetWorld()->SpawnActor<APlayerObject>(GameInstance->PlayerList[i]);
					Players[i]->PlayerIndex = i * 3 > 6;
					for (int j = 0; j < i; j++)
					{
						if (IsValid(GameInstance->PlayerList[j]))
						{
							if (Players[i]->IsA(GameInstance->PlayerList[j]))
							{
								Players[i]->ColorIndex = 2;
								break;
							}
						}
					}
				}
				else
				{
					Players[i] = GetWorld()->SpawnActor<APlayerObject>(APlayerObject::StaticClass());
				}
				if (i % 3 == 0)
				{
					Players[i]->PlayerFlags |= PLF_IsOnScreen;
				}
			}
			else
			{
				Players[i] = GetWorld()->SpawnActor<APlayerObject>(APlayerObject::StaticClass());
				if (i % 3 == 0)
				{
					Players[i]->PlayerFlags |= PLF_IsOnScreen;
				}
			}
			SortedObjects[i] = Players[i];
		}
		else
		{
			Players[i] = GetWorld()->SpawnActor<APlayerObject>(APlayerObject::StaticClass());
			if (i % 3 == 0)
			{
				Players[i]->PlayerFlags |= PLF_IsOnScreen;
			}
			SortedObjects[i] = Players[i];
		}
		Players[i]->InitPlayer();
		Players[i]->GameState = this;
		Players[i]->ObjNumber = i + 400;
	}
	for (int i = 0; i < 400; i++)
	{
		Objects[i] = GetWorld()->SpawnActor<ABattleObject>(ABattleObject::StaticClass());
		Objects[i]->ResetObject();
		Objects[i]->GameState = this;
		Objects[i]->ObjNumber = i;
		SortedObjects[i + 6] = Objects[i];
	}
	BattleState.RoundFormat = GameInstance->RoundFormat;
	BattleState.RoundTimer = GameInstance->StartRoundTimer * 60;
	
	RoundInit();
}

void ANightSkyGameState::RoundInit()
{
	BattleState.RoundCount++;
	BattleState.TimeUntilRoundStart = 180;
	for (int i = 0; i < 400; i++)
		Objects[i]->ResetObject();
	
	for (int i = 0; i < 6; i++)
		Players[i]->ResetForRound();

	Players[0]->PlayerFlags = PLF_IsOnScreen;
	Players[3]->PlayerFlags = PLF_IsOnScreen;

	const UNightSkyGameInstance* GameInstance = Cast<UNightSkyGameInstance>(GetGameInstance());

	BattleState.RoundTimer = GameInstance->StartRoundTimer * 60;
	BattleState.CurrentScreenPos = 0;
}

void ANightSkyGameState::UpdateLocalInput()
{
	LocalInputs[0] = GetLocalInputs(0);
	LocalInputs[1] = GetLocalInputs(1);
}

// Called every frame
void ANightSkyGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateGameState();
	UpdateCamera();
}

void ANightSkyGameState::UpdateGameState(int32 Input1, int32 Input2)
{
	BattleState.TimeUntilRoundStart--;
	if (BattleState.TimeUntilRoundStart <= 0)
		BattleState.RoundTimer--;

	if (BattleState.RoundTimer < 0)
		BattleState.RoundTimer = 0;
	BattleState.FrameNumber++;

	for (int i = 0; i < 2; i++)
	{
		if (BattleState.Meter[i] > BattleState.MaxMeter[i])
			BattleState.Meter[i] = BattleState.MaxMeter[i];
		if (BattleState.Meter[i] < 0)
			BattleState.Meter[i] = 0;
	}
	
	SortObjects();

	Players[0]->Inputs = Input1;
	Players[3]->Inputs = Input2;
	for (int i = 0; i < 6; i++)
	{
		if (Players[i]->PlayerFlags & PLF_IsOnScreen)
		{
			for (int j = 0; j < 6; j++)
			{
				if (i < 3)
				{
					if (j >= 3 && Players[j]->PlayerFlags & PLF_IsOnScreen)
					{
						Players[i]->Enemy = Players[j];
					}
				}
				else
				{
					if (j < 3 && Players[j]->PlayerFlags & PLF_IsOnScreen)
					{
						Players[i]->Enemy = Players[j];
					}
				}
			}
		}
	}

	for (int i = 0; i < 406; i++)
	{
		if (i == BattleState.ActiveObjectCount)
			break;
		if (!SortedObjects[i]->IsPlayer || SortedObjects[i]->Player->PlayerFlags & PLF_IsOnScreen)
			SortedObjects[i]->Update();
	}
	HandlePushCollision();
	SetScreenBounds();
	SetWallCollision();
}

void ANightSkyGameState::UpdateGameState()
{
	UpdateLocalInput();
	UpdateGameState(LocalInputs[0], LocalInputs[1]);
}

void ANightSkyGameState::SortObjects()
{
	BattleState.ActiveObjectCount = 6;
	for (int i = 6; i < 406; i++)
	{
		for (int j = i + 1; j < 406; j++)
		{
			if (SortedObjects[j]->IsActive && !SortedObjects[i]->IsActive)
			{
				ABattleObject* Temp = SortedObjects[i];
				SortedObjects[i] = SortedObjects[j];
				SortedObjects[j] = Temp;
			}
		}
		if (SortedObjects[i]->IsActive)
		{
			BattleState.ActiveObjectCount++;
		}
	}
}

void ANightSkyGameState::HandlePushCollision() const
{
	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			if (Players[i]->PlayerIndex != Players[j]->PlayerIndex && Players[i]->PlayerFlags & PLF_IsOnScreen && Players[j]->PlayerFlags & PLF_IsOnScreen)
			{
				Players[i]->HandlePushCollision(Players[j]);
			}
		}
	}
}

void ANightSkyGameState::SetScreenBounds()
{
	for (int i = 0; i < 6; i++)
	{
		if (Players[i]->PlayerIndex == 0)
		{
			for (int j = 0; j < 6; j++)
			{
				if (Players[j]->PlayerIndex == 1)
				{
					if (Players[i]->PlayerFlags & PLF_IsOnScreen && Players[j]->PlayerFlags & PLF_IsOnScreen)
					{
						const int NewScreenPos = (Players[i]->PosX + Players[j]->PosX) / 2;
						BattleState.CurrentScreenPos = BattleState.CurrentScreenPos + (NewScreenPos - BattleState.CurrentScreenPos) * 5 / 100;
						if (BattleState.CurrentScreenPos > 1080000)
						{
							BattleState.CurrentScreenPos = 1080000;
						}
						else if (BattleState.CurrentScreenPos < -1080000)
						{
							BattleState.CurrentScreenPos = -1080000;
						}
					}
				}
			}
		}
	}
}

void ANightSkyGameState::SetWallCollision()
{
	for (int i = 0; i < 6; i++)
	{
		if (Players[i] != nullptr)
		{
			if (Players[i]->PlayerFlags & PLF_IsOnScreen)
			{
				if (Players[i]->PosX >= 840000 + BattleState.CurrentScreenPos)
				{
					Players[i]->PosX = 840000 + BattleState.CurrentScreenPos;
				}
				else if (Players[i]->PosX <= -840000 + BattleState.CurrentScreenPos)
				{
					Players[i]->PosX = -840000 + BattleState.CurrentScreenPos;
				}
			}
		}
	}
}


void ANightSkyGameState::UpdateCamera() const
{
	if (CameraActor != nullptr)
	{
		FVector Average = (Players[0]->GetActorLocation() + Players[3]->GetActorLocation()) / 2;
		float NewX = FMath::Clamp(-Average.X,-540, 540);
		float Distance = sqrt(abs((Players[0]->GetActorLocation() - Players[3]->GetActorLocation()).X));
		Distance = FMath::Clamp(Distance,20, 25);
		float NewY = FMath::GetMappedRangeValueClamped(TRange<float>(0, 25), TRange<float>(0, 840), Distance);
		float NewZ = Average.Z + 150;
		FVector NewCameraLocation = FMath::Lerp(CameraActor->GetActorLocation(), FVector(-NewX, NewY, NewZ), 0.15);
		CameraActor->SetActorLocation(NewCameraLocation);
		if (!SequenceActor->SequencePlayer->IsPlaying())
		{
			SequenceCameraActor->SetActorLocation(FVector(0, 840, 175));
		}
	}
}

int ANightSkyGameState::GetLocalInputs(int Index) const
{
	if (const ANightSkyPlayerController* Controller = Cast<ANightSkyPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), Index)); Controller != nullptr)
	{
		return Controller->Inputs;
	}
	return 0;
}

void ANightSkyGameState::ScreenPosToWorldPos(int32 X, int32 Y, int32* OutX, int32* OutY) const
{
	*OutX = BattleState.CurrentScreenPos - 900000 + 1800000 * X / 100;
}