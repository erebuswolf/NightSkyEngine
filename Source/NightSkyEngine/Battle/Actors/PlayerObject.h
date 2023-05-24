﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BattleObject.h"
#include "NightSkyEngine/Battle/InputBuffer.h"
#include "NightSkyEngine/Battle/State.h"
#include "NightSkyEngine/Battle/StateMachine.h"
#include "NightSkyEngine/Data/CollisionData.h"
#include "NightSkyEngine/Data/StateData.h"
#include "PlayerObject.generated.h"

constexpr int32 DamageReactionCelCount = 64;
constexpr int32 ExtraGaugeCount = 5;
constexpr int32 CancelArraySize = 64;
constexpr int32 MaxComponentCount = 64;

class USubroutineData;
class UStateData;

UENUM()
enum EActionStance
{
	ACT_Standing,
	ACT_Crouching,
	ACT_Jumping,
};

USTRUCT()
struct FExtraGauge
{
	GENERATED_BODY()

	uint32 Value;
	uint32 InitialValue;
	uint32 MaxValue;
	uint32 Sections;
};

USTRUCT()
struct FSavedInputCondition
{
	GENERATED_BODY()
	
	FInputBitmask Sequence[32];
	int Lenience = 8;
	int ImpreciseInputCount = 0;
	bool bInputAllowDisable = true;
	EInputMethod Method = EInputMethod::Normal;
};

/**
 * 
 */
UCLASS()
class NIGHTSKYENGINE_API APlayerObject : public ABattleObject
{
	GENERATED_BODY()

public:
	APlayerObject();
	
	//Starting from this until PlayerSyncEnd, everything is saved/loaded for rollback.
	unsigned char PlayerSync;
	
	/*
	 * Default values
	 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FWalkSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BWalkSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FDashInitSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FDashAccel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FDashMaxSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FDashFriction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BDashSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BDashHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BDashGravity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 JumpHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FJumpSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BJumpSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 JumpGravity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SuperJumpHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FSuperJumpSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BSuperJumpSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SuperJumpGravity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AirDashMinimumHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FAirDashSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BAirDashSpeed;
	// Time until forward air dash properties stop applying.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FAirDashTime;
	// Time until backward air dash properties stop applying.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BAirDashTime;
	// Time until forward air dash can be cancelled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FAirDashNoAttackTime;
	// Time until backward air dash can be cancelled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BAirDashNoAttackTime; 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AirJumpCount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AirDashCount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StandPushWidth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StandPushHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CrouchPushWidth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CrouchPushHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AirPushWidth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AirPushHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AirPushHeightLow;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CloseNormalRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHealth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ComboRate = 60;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OtgProration = 65;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ForwardWalkMeterGain;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ForwardJumpMeterGain;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ForwardDashMeterGain;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ForwardAirDashMeterGain;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MeterPercentOnHit = 72;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MeterPercentOnHitGuard = 18;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MeterPercentOnReceiveHitGuard = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MeterPercentOnReceiveHit = 40;
	FixedString<32> DamageReactionCels[DamageReactionCelCount];

	/*
	 * Player registers. These are only touched by the engine to reset per round.
	 * Use these to keep track of values (timers, toggles, counters, etc) that are character-specific.
	 */
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerReg1 = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerReg2 = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerReg3 = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerReg4 = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerReg5 = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerReg6 = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerReg7 = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerReg8 = 0;

	/*
	 * Action data.
	 */

	FixedString<32> StateName;

	UPROPERTY()
	FStateMachine StoredStateMachine;

	/*
	 * Input data.
	 */
	uint32 Inputs;
	
	UPROPERTY()
	FInputBuffer StoredInputBuffer;
	
	/*
	 * Miscellaneous values.
	 */
	
	// If true, inputs will operate as if the character is facing the opposite direction.
	UPROPERTY(BlueprintReadWrite)
	bool FlipInputs;
	
	UPROPERTY(BlueprintReadOnly)
	int32 PlayerIndex;
	
	EActionStance Stance;
	int32 Pushback;
	int32 CurrentHealth;
	int32 TotalProration = 10000;
	int32 ComboCounter;
	int32 ComboTimer;
	int32 ThrowTechWindow = 6;
	uint32 InvulnFlags = 0;
	uint32 PlayerFlags = 0;
	uint32 EnableFlags = 0;
	uint32 StrikeInvulnerableTimer = 0;
	uint32 ThrowInvulnerableTimer = 0;
	FExtraGauge ExtraGauges[ExtraGaugeCount];
	uint32 AirDashTimer = 0;
	int32 OTGCount;
	
	/*
	 * Object pointers.
	 */
	
	UPROPERTY(BlueprintReadOnly)
	APlayerObject* Enemy;
	UPROPERTY()
	ABattleObject* ChildBattleObjects[32];
	UPROPERTY()
	ABattleObject* StoredBattleObjects[16];

	bool ComponentVisible[MaxComponentCount];
	FSavedInputCondition SavedInputCondition;

protected:
	/*
	 * Internal values.
	 */
	uint32 CurrentAirJumpCount = 0;
	uint32 CurrentAirDashCount = 0;
	uint32 AirDashTimerMax = 0;
	uint32 CancelFlags = 0;
	uint32 AirDashNoAttackTime = 0;
	uint32 InstantBlockLockoutTimer = 0;
	uint32 MeterCooldownTimer = 0;

	//Chain cancels (copied from TArray to static array)
	int32 ChainCancelOptionsInternal[CancelArraySize] = {};
	//Whiff cancels (copied from TArray to static array)
	int32 WhiffCancelOptionsInternal[CancelArraySize] = {};
	FixedString<64> ExeStateName;
	FixedString<64> BufferedStateName;
	
public:
	//Anything past here isn't saved or loaded for rollback.
	unsigned char PlayerSyncEnd; 

	/*
	 * Data to copy from blueprint TArray to internal object
	 */
	
	UPROPERTY(EditAnywhere)
	TArray<FString> DamageReactionCelsBP;
	TArray<FString> ChainCancelOptions;
	TArray<FString> WhiffCancelOptions; 

	/*
	 * States and subroutines
	*/
	
	UPROPERTY()
	TArray<USubroutine*> CommonSubroutines;
	TArray<FString> CommonSubroutineNames;
	UPROPERTY()
	TArray<USubroutine*> Subroutines;
	TArray<FString> SubroutineNames;
	
	UPROPERTY()
	TArray<UState*> CommonObjectStates;
	TArray<FString> CommonObjectStateNames;	
	UPROPERTY()
	TArray<UState*> ObjectStates;
	TArray<FString> ObjectStateNames;	

	/*
	 * Data assets
	 */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCollisionData* CommonCollisionData;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCollisionData* CollisionData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USubroutineData* CommonSubroutineData;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USubroutineData* CharaSubroutineData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStateData* CommonObjectStateData;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStateData* ObjectStateData;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStateData* CharaStateData;

	UPROPERTY(BlueprintReadOnly)
	int32 ColorIndex = 1;
	
	UPROPERTY()
	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;

private:
	virtual void BeginPlay() override;
	void HandleStateMachine(bool Buffer);

	//buffer state
	void HandleBufferedState();
	//check state conditions
	bool HandleStateCondition(EStateCondition StateCondition);
	//check if chain cancel option exists
	bool FindChainCancelOption(const FString& Name);
	//check if whiff cancel option exists
	bool FindWhiffCancelOption(const FString& Name);
	//checks kara cancel
	bool CheckKaraCancel(EStateType InStateType);
	//checks if a child object with a corresponding object id exists. if so, do not enter state 
	bool CheckObjectPreventingState(int InObjectID);

public:
	void InitPlayer();
	virtual void Update() override;
	//called whenever state changes
	void OnStateChange();
	//resets object for next round
	void ResetForRound();
	//disables last input
	void DisableLastInput();
	
	//ONLY CALL WHEN INITIALIZING MATCH! OTHERWISE THE GAME WILL CRASH
	UFUNCTION(BlueprintImplementableEvent)
	void InitBP();
	//ONLY CALL AT THE START OF InitStateMachine! OTHERWISE THE GAME WILL CRASH
	UFUNCTION(BlueprintCallable)
	void EmptyStateMachine();
	
	/*
	 * Blueprint callable functions.
	 */
	//add state to state machine
	UFUNCTION(BlueprintCallable)
	void AddState(FString Name, UState* State); 
	//add subroutine to state machine
	UFUNCTION(BlueprintCallable)
	void AddSubroutine(FString Name, USubroutine* Subroutine);
	//calls subroutine
	UFUNCTION(BlueprintCallable)
	void CallSubroutine(FString Name);
	//set stance
	UFUNCTION(BlueprintCallable)
	void SetStance(EActionStance InStance);
	//force set state
	UFUNCTION(BlueprintCallable, CallInEditor)
	void JumpToState(FString NewName);
	//gets current state name
	UFUNCTION(BlueprintPure)
	FString GetCurrentStateName() const;
	//check if state can be entered
	UFUNCTION(BlueprintPure)
	bool CheckStateEnabled(EStateType StateType);
	//enable state type
	UFUNCTION(BlueprintCallable)
	void EnableState(EEnableFlags EnableType);
	//disables state type
	UFUNCTION(BlueprintCallable)
	void DisableState(EEnableFlags EnableType);
	//enable all attacks only
	UFUNCTION(BlueprintCallable)
	void EnableAttacks();
	//enables all state types besides tech
	UFUNCTION(BlueprintCallable)
	void EnableAll();
	//disable all state types besides tech
	UFUNCTION(BlueprintCallable)
	void DisableAll();
	//checks raw inputs (after side switching)
	UFUNCTION(BlueprintPure)
	bool CheckInputRaw(EInputFlags Input);
	//checks input condition
	UFUNCTION(BlueprintPure)
	bool CheckInput(const FInputCondition& Input);
	//temporarily adds air jump
	UFUNCTION(BlueprintCallable)
	void AddAirJump(int32 NewAirJump);
	//temporarily adds air dash
	UFUNCTION(BlueprintCallable)
	void AddAirDash(int32 NewAirDash);
	//set air dash timer (set is forward for forward airdashes)
	UFUNCTION(BlueprintCallable)
	void SetAirDashTimer(bool IsForward);
	//set air dash timer (set is forward for forward airdashes)
	UFUNCTION(BlueprintCallable)
	void SetAirDashNoAttackTimer(bool IsForward);
	//toggles default landing action. if true, landing will go to JumpLanding state. if false, define your own landing.
	UFUNCTION(BlueprintCallable)
	void SetDefaultLandingAction(bool Enable);
};

constexpr size_t SizeOfPlayerObject = offsetof(APlayerObject, PlayerSyncEnd) - offsetof(APlayerObject, PlayerSync);