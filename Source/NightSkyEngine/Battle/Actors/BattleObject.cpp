﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleObject.h"

#include "PlayerObject.h"
#include "NightSkyEngine/Battle/Bitflags.h"
#include "NightSkyEngine/Battle/Globals.h"

// Sets default values
ABattleObject::ABattleObject()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
}

// Called when the game starts or when spawned
void ABattleObject::BeginPlay()
{
	Super::BeginPlay();
	if (IsPlayer)
	{
		Player = Cast<APlayerObject>(this);
	}
}

void ABattleObject::Move()
{
	//Set previous pos values
	PrevPosX = PosX; 
	PrevPosY = PosY;

	SpeedX = SpeedX * SpeedXRatePerFrame / 100;
	SpeedY = SpeedY * SpeedYRatePerFrame / 100;
	SpeedZ = SpeedZ * SpeedZRatePerFrame / 100;
	const int32 FinalSpeedX = SpeedX * SpeedXRate / 100;
	const int32 FinalSpeedY = SpeedY * SpeedYRate / 100;
	const int32 FinalSpeedZ = SpeedZ * SpeedZRate / 100;

	if (MiscFlags & MISC_InertiaEnable) //only use inertia if enabled
	{
		if (PosY <= GroundHeight && MiscFlags & MISC_FloorCollisionActive) //only decrease inertia if grounded
		{
			Inertia = Inertia - Inertia / 10;
		}
		if (Inertia > -875 && Inertia < 875) //if inertia small enough, set to zero
		{
			Inertia = 0;
		}
		AddPosXWithDir(Inertia);
	}

	if (IsPlayer)
	{
		int32 ModifiedPushback;
		if (Player->Stance == ACT_Crouching)
			ModifiedPushback = Player->Pushback * 86;
		else
			ModifiedPushback = Player->Pushback * 88;

		Player->Pushback = ModifiedPushback / 100;
		
		AddPosXWithDir(Player->Pushback);
	}

	AddPosXWithDir(FinalSpeedX); //apply speed

	if (IsPlayer && Player != nullptr)
	{
		if (Player->AirDashTimer == 0 || (SpeedY > 0 && ActionTime < 5)) // only set y speed if not airdashing/airdash startup not done
		{
			PosY += FinalSpeedY;
			if (PosY > GroundHeight || !(MiscFlags & MISC_FloorCollisionActive))
				SpeedY -= Gravity;
		}
		else
		{
			SpeedY = 0;
		}
	}
	else
	{
		PosY += FinalSpeedY;
		if (PosY > GroundHeight || !(MiscFlags & MISC_FloorCollisionActive))
			SpeedY -= Gravity;
	}
		
	if (PosY < GroundHeight && MiscFlags & MISC_FloorCollisionActive) //if on ground, force y values to zero
	{
		PosY = GroundHeight;
		SpeedY = 0;
	}
	if (MiscFlags & MISC_WallCollisionActive)
	{
		if (PosX < -1800000)
		{
			PosX = -1800000;
		}
		else if (PosX > 1800000)
		{
			PosX = 1800000;
		}
	}

	PosZ += FinalSpeedZ;
}

// Called every frame
void ABattleObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABattleObject::HandlePushCollision(const ABattleObject* OtherObj)
{
	if (MiscFlags & MISC_PushCollisionActive && OtherObj->MiscFlags & MISC_PushCollisionActive)
	{
		if (Hitstop <= 0 && ((!OtherObj->IsPlayer || OtherObj->Player->PlayerFlags & PLF_IsThrowLock) == 0 || (!IsPlayer || Player->PlayerFlags & PLF_IsThrowLock) == 0))
		{
			if (T >= OtherObj->B && B <= OtherObj->T && R >= OtherObj->L && L <= OtherObj->R)
			{
				bool IsPushLeft;
				int CollisionDepth;
				if(PosX == OtherObj->PosX)
				{
					if (PrevPosX == OtherObj->PrevPosX)
					{
						if (IsPlayer == OtherObj->IsPlayer)
						{
							IsPushLeft = Player->PlayerIndex > 0;
						}
						else
						{
							IsPushLeft = IsPlayer > OtherObj->IsPlayer;
						}
					}
					else
					{
						IsPushLeft = PrevPosX < OtherObj->PrevPosX;
					}
				}
				else
				{
					IsPushLeft = PosX < OtherObj->PosX;
				}
				if (IsPushLeft)
				{
					CollisionDepth = OtherObj->L - R;
				}
				else
				{
					CollisionDepth = OtherObj->R - L;
				}
				const int PosXOffset = CollisionDepth / 2;
				PosX += PosXOffset;
			}
		}
	}
}

void ABattleObject::HandleFlip()
{
	EObjDir CurrentDir = Direction;
	if (!Player->Enemy) return;
	if (PosX < Player->Enemy->PosX)
	{
		SetFacing(DIR_Right);
	}
	else if (PosX > Player->Enemy->PosX)
	{
		SetFacing(DIR_Left);
	}
	if (CurrentDir != Direction)
	{
		SpeedX = -SpeedX;
		Inertia = -Inertia;
		if (IsPlayer)
		{
			Player->StoredInputBuffer.FlipInputsInBuffer();
			if (Player->Stance == ACT_Standing && Player->EnableFlags & ENB_Standing)
				Player->JumpToState("StandFlip");
			else if (Player->Stance == ACT_Crouching && Player->EnableFlags & ENB_Crouching)
				Player->JumpToState("CrouchFlip");
			else if (Player->EnableFlags & ENB_Jumping)
				Player->JumpToState("JumpFlip");
		}
	}
}

void ABattleObject::TriggerEvent(EEventType EventType)
{
	UState* State = ObjectState;
	if (IsPlayer)
		State = Player->StoredStateMachine.CurrentState;
	UFunction* const Func = State->FindFunction(FName(EventHandlers[EventType].FunctionName.GetString()));
	if (IsValid(Func) && Func->ParmsSize == 0)
	{
		State->ProcessEvent(Func, nullptr);
	}
}

void ABattleObject::UpdateVisualLocation()
{
	SetActorLocation(FVector(static_cast<float>(PosX) / COORD_SCALE, static_cast<float>(PosZ) / COORD_SCALE, static_cast<float>(PosY) / COORD_SCALE));
}

void ABattleObject::FuncCall(FName FuncName) const
{
	UState* CurrentState = ObjectState;
	if (IsPlayer)
		CurrentState = Player->StoredStateMachine.CurrentState;

	UFunction* const Func = CurrentState->FindFunction(FuncName);
	if (IsValid(Func) && Func->ParmsSize == 0)
	{
		CurrentState->ProcessEvent(Func, nullptr);
	}
}

void ABattleObject::GetBoxes()
{
	if (Player->CommonCollisionData != nullptr)
	{
		for (int i = 0; i < Player->CommonCollisionData->CollisionFrames.Num(); i++)
		{
			if (Player->CommonCollisionData->CollisionFrames[i].CelName == CelName.GetString())
			{
				AnimName.SetString(Player->CommonCollisionData->CollisionFrames[i].AnimName);
				AnimFrame = Player->CommonCollisionData->CollisionFrames[i].AnimFrame;
				for (int j = 0; j < CollisionArraySize; j++)
				{
					if (j < Player->CommonCollisionData->CollisionFrames[i].Boxes.Num())
					{
						FCollisionBox CollisionBoxInternal;
						CollisionBoxInternal.Type = Player->CommonCollisionData->CollisionFrames[i].Boxes[j].Type;
						CollisionBoxInternal.PosX = Player->CommonCollisionData->CollisionFrames[i].Boxes[j].PosX;
						CollisionBoxInternal.PosY = Player->CommonCollisionData->CollisionFrames[i].Boxes[j].PosY;
						CollisionBoxInternal.SizeX = Player->CommonCollisionData->CollisionFrames[i].Boxes[j].SizeX;
						CollisionBoxInternal.SizeY = Player->CommonCollisionData->CollisionFrames[i].Boxes[j].SizeY;
						Boxes[j] = CollisionBoxInternal;
					}
					else
					{
						Boxes[j].Type = BOX_Hurt;
						Boxes[j].PosX = -10000000;
						Boxes[j].PosY = -10000000;
						Boxes[j].SizeX = 0;
						Boxes[j].SizeY = 0;
					}
				}
				return;
			}
		}
		for (int j = 0; j < CollisionArraySize; j++)
		{
			Boxes[j].Type = BOX_Hurt;
			Boxes[j].PosX = -10000000;
			Boxes[j].PosY = -10000000;
			Boxes[j].SizeX = 0;
			Boxes[j].SizeY = 0;
		}
	}
	if (Player->CollisionData != nullptr)
	{
		for (int i = 0; i < Player->CollisionData->CollisionFrames.Num(); i++)
		{
			if (Player->CollisionData->CollisionFrames[i].CelName == CelName.GetString())
			{
				AnimName.SetString(Player->CollisionData->CollisionFrames[i].AnimName);
				AnimFrame = Player->CollisionData->CollisionFrames[i].AnimFrame;
				for (int j = 0; j < CollisionArraySize; j++)
				{
					if (j < Player->CollisionData->CollisionFrames[i].Boxes.Num())
					{
						FCollisionBox CollisionBoxInternal;
						CollisionBoxInternal.Type = Player->CollisionData->CollisionFrames[i].Boxes[j].Type;
						CollisionBoxInternal.PosX = Player->CollisionData->CollisionFrames[i].Boxes[j].PosX;
						CollisionBoxInternal.PosY = Player->CollisionData->CollisionFrames[i].Boxes[j].PosY;
						CollisionBoxInternal.SizeX = Player->CollisionData->CollisionFrames[i].Boxes[j].SizeX;
						CollisionBoxInternal.SizeY = Player->CollisionData->CollisionFrames[i].Boxes[j].SizeY;
						Boxes[j] = CollisionBoxInternal;
					}
					else
					{
						Boxes[j].Type = BOX_Hurt;
						Boxes[j].PosX = -10000000;
						Boxes[j].PosY = -10000000;
						Boxes[j].SizeX = 0;
						Boxes[j].SizeY = 0;
					}
				}
				return;
			}
		}
		for (int j = 0; j < CollisionArraySize; j++)
		{
			Boxes[j].Type = BOX_Hurt;
			Boxes[j].PosX = -10000000;
			Boxes[j].PosY = -10000000;
			Boxes[j].SizeX = 0;
			Boxes[j].SizeY = 0;
		}
	}
}

void ABattleObject::InitObject()
{
	if (IsPlayer)
		return;
	ObjectState->Parent = this;
	SetActorLocation(FVector(static_cast<float>(PosX) / COORD_SCALE, static_cast<float>(PosZ) / COORD_SCALE, static_cast<float>(PosY) / COORD_SCALE)); //set visual location and scale in unreal
	if (Direction == DIR_Left)
	{
		SetActorScale3D(FVector(-1, 1, 1));
	}
	else
	{
		SetActorScale3D(FVector(1, 1, 1));
	}
}

void ABattleObject::Update()
{
	if (Direction == DIR_Left)
	{
		SetActorScale3D(FVector(-1, 1, 1));
	}
	else
	{
		SetActorScale3D(FVector(1, 1, 1));
	}

	L = PosX - PushWidth / 2; //sets pushboxes
	R = PosX + PushWidth / 2;
	if (Direction == DIR_Right)
		R += PushWidthFront;
	else
		L -= PushWidthFront;
	T = PosY + PushHeight;
	B = PosY - PushHeightLow;
	
	if (MiscFlags & MISC_FlipEnable)
		HandleFlip();

	if (SuperFreezeTimer > 0)
	{
		if (SuperFreezeTimer == 1)
		{
			TriggerEvent(EVT_SuperFreezeEnd);
		}
		SuperFreezeTimer--;
		return;
	}
		
	if (Hitstop > 0) //break if hitstop active.
	{
		Hitstop--;
		return;
	}

	TriggerEvent(EVT_Update);
	GetBoxes();
	Move();
	
	if (PosY == GroundHeight && PrevPosY != GroundHeight)
	{
		TriggerEvent(EVT_Landing);
	}

	if (!IsPlayer)
	{
		if (ActionTime == 0)
		{
			TriggerEvent(EVT_Enter);
		}
		
		ObjectState->CallExec();
		TimeUntilNextCel--;
		if (TimeUntilNextCel == 0)
			CelIndex++;
	}

	ActionTime++;
	
	UpdateVisualLocation();
}

void ABattleObject::ResetObject()
{
	if (IsPlayer)
		return;
	IsActive = false;
	PosX = 0;
	PosY = 0;
	PosZ = 0;
	PrevPosX = 0;
	PrevPosY = 0;
	PrevPosZ = 0;
	SpeedX = 0;
	SpeedY = 0;
	SpeedZ = 0;
	Gravity = 1900;
	Inertia = 0;
	ActionTime = 0;
	PushHeight = 0;
	PushHeightLow = 0;
	PushWidth = 0;
	PushWidthFront  = 0;
	Hitstop = 0;
	L = 0;
	R = 0;
	T = 0;
	B = 0;
	HitCommon = FHitDataCommon();
	NormalHit = FHitData();
	CounterHit = FHitData();
	ReceivedHitCommon = FHitDataCommon();
	ReceivedHit = FHitData();
	AttackFlags = ATK_AttackProjectileAttribute;
	StunTime = 0;
	Hitstop = 0;
	MiscFlags = 0;
	Direction = DIR_Right;
	SpeedXRate = 100;
	SpeedXRatePerFrame = 100;
	SpeedYRate = 100;
	SpeedYRatePerFrame = 100;
	SpeedZRate = 100;
	SpeedZRatePerFrame = 100;
	GroundHeight = 0;
	ReturnReg = 0;
	ActionReg1 = 0;
	ActionReg2 = 0;
	ActionReg3 = 0;
	ActionReg4 = 0;
	ActionReg5 = 0;
	ActionReg6 = 0;
	ActionReg7 = 0;
	ActionReg8 = 0;
	ObjectReg1 = 0;
	ObjectReg2 = 0;
	ObjectReg3 = 0;
	ObjectReg4 = 0;
	ObjectReg5 = 0;
	ObjectReg6 = 0;
	ObjectReg7 = 0;
	ObjectReg8 = 0;
	IsPlayer = false;
	SuperFreezeTimer = 0;
	CelName.SetString("");
	AnimName.SetString("");
	AnimFrame = 0;
	CelIndex = 0;
	TimeUntilNextCel = 0;
	for (auto& Handler : EventHandlers)
		Handler = FEventHandler();
	EventHandlers[EVT_Enter].FunctionName.SetString("Init");	
	HitPosX = 0;
	HitPosY = 0;
	for (auto& Box : Boxes)
	{
		Box = FCollisionBox();
	}
	ObjectStateName.SetString("");
	ObjectID = 0;
	Player = nullptr;
}

void ABattleObject::InitEventHandler(EEventType EventType, FName FuncName)
{
	EventHandlers[EventType].FunctionName.SetString(FuncName.ToString());
}

FString ABattleObject::GetCelName()
{
	return CelName.GetString();
}

FString ABattleObject::GetAnimName()
{
	return AnimName.GetString();
}

FString ABattleObject::GetLabelName()
{
	return LabelName.GetString();
}

void ABattleObject::SetCelName(FString InName)
{
	CelName.SetString(InName);
}

void ABattleObject::GotoLabel(FString InName, bool ResetState)
{
	LabelName.SetString(InName);
	GotoLabelActive = true;
	if (IsPlayer && ResetState)
		Player->JumpToState(Player->GetCurrentStateName());
}

void ABattleObject::AddPosXWithDir(int InPosX)
{
	if (Direction == DIR_Right)
	{
		PosX += InPosX;
	}
	else
	{
		PosX -= InPosX;
	}
}

void ABattleObject::SetSpeedXRaw(int InSpeedX)
{
	if (Direction == DIR_Right)
	{
		SpeedX = InSpeedX;
	}
	else
	{
		SpeedX = -InSpeedX;
	}
}

void ABattleObject::AddSpeedXRaw(int InSpeedX)
{
	if (Direction == DIR_Right)
	{
		SpeedX += InSpeedX;
	}
	else
	{
		SpeedX -= InSpeedX;
	}
}

void ABattleObject::SetFacing(EObjDir NewDir)
{
	Direction = NewDir;
}

void ABattleObject::FlipCharacter()
{
	if (Direction == DIR_Right)
		Direction = DIR_Left;
	else
		Direction = DIR_Right;
}

void ABattleObject::FaceOpponent()
{
	EObjDir CurrentDir = Direction;
	if (!Player->Enemy) return;
	if (PosX < Player->Enemy->PosX)
	{
		SetFacing(DIR_Right);
	}
	else if (PosX > Player->Enemy->PosX)
	{
		SetFacing(DIR_Left);
	}
	if (CurrentDir != Direction)
	{
		SpeedX = -SpeedX;
		Inertia = -Inertia;
	}
}

void ABattleObject::DeactivateObject()
{
	if (IsPlayer) // Don't use on players
		return;
	// Remove from player cache
	for (int i = 0; i < 32; i++)
	{
		if (this == Player->ChildBattleObjects[i])
		{
			Player->ChildBattleObjects[i] = nullptr;
			break;
		}
	}
	for (int i = 0; i < 16; i++)
	{
		if (this == Player->StoredBattleObjects[i])
		{
			Player->StoredBattleObjects[i] = nullptr;
			break;
		}
	}

	// Wait until the next frame to complete
	MiscFlags |= MISC_DeactivateOnNextUpdate;
}

void ABattleObject::EnableFlip(bool Enabled)
{
	if (Enabled)
	{
		MiscFlags |= MISC_FlipEnable;
	}
	else
	{
		MiscFlags = MiscFlags & ~MISC_FlipEnable;
	}
}

void ABattleObject::EnableInertia()
{
	MiscFlags |= MISC_InertiaEnable;
}

void ABattleObject::DisableInertia()
{
	MiscFlags = MiscFlags & ~MISC_InertiaEnable;
}

void ABattleObject::HaltMomentum()
{
	SpeedX = 0;
	SpeedY = 0;
	SpeedZ = 0;
	Gravity = 0;
	Inertia = 0;
}