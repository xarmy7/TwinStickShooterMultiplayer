// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "TwinStickShooterPawn.generated.h"

USTRUCT()
struct FSavedMove
{
	GENERATED_USTRUCT_BODY();

	float deltaTime = 0.f;
	FVector moveDirection = FVector();
	FVector pawnPosition = FVector();
};

UCLASS(Blueprintable)
class ATwinStickShooterPawn : public APawn
{
	GENERATED_BODY()

		/* The mesh component */
		UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* ShipMeshComponent;

	/** The camera */
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* CameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

public:
	ATwinStickShooterPawn();
	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
		FString PlayerName;

	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
		int Score = 0;

	/** Offset from the ships location to spawn projectiles */
	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
		FVector GunOffset;

	/* How fast the weapon will fire */
	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
		float FireRate;

	/* The speed our ship moves around the level */
	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
		float MoveSpeed;

	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
		int MaxHealthPoints = 10;

	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
		float ReviveDelay = 2.0f;

	/** Sound to play each time we fire */
	UPROPERTY(Category = Audio, EditAnywhere, BlueprintReadWrite)
		class USoundBase* FireSound;

	// Begin Actor Interface
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End Actor Interface

	/* Handler for the fire timer expiry */
	void ShotTimerExpired();

	// Static names for axis bindings
	static const FName MoveForwardBinding;
	static const FName MoveRightBinding;
	static const FName FireForwardBinding;
	static const FName FireRightBinding;

private:

	/* Flag to control firing  */
	uint32 bCanFire : 1;

	/* Flag to set death state */
	uint32 bIsAlive : 1;

	/** Handle for efficient management of ShotTimerExpired timer */
	FTimerHandle TimerHandle_ShotTimerExpired;

	FTimerHandle TimerHandle_Revive;

	UPROPERTY()
		float ForwardValue;
	UPROPERTY()
		float RightValue;

	UPROPERTY()
		FVector MoveDirection;

	int CurrentHealthPoints;

	/* Fire a shot in the specified direction */
	void FireShot(FVector FireDirection);
	void FireShotServer(FVector FireDirection);

	/* Compute Pawn movement */
	void ComputeMove(FVector moveDir, float DeltaSeconds);
	void ComputeMoveServer(FVector moveDirection, FVector pawnPosition, float DeltaSeconds);

	/* Revive Pawn */
	void Revive();

	float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	void UpdatePawnRPC(FVector moveDir, FVector shootDir, FString const& myPlayerName, float DeltaSecond);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void	PlayerDie();

	UFUNCTION(Server, Reliable, WithValidation)
	void	ServerUpdateInput(FVector moveDir, FVector actorDir, FVector shootDir, FString const& myPlayerName, float deltaSeconds);

	void	ServerUpdateInput_Implementation(FVector moveDir, FVector actorDir, FVector shootDir, FString const& myPlayerName, float deltaSeconds);
	bool	ServerUpdateInput_Validate(FVector moveDir, FVector actorDir, FVector shootDir, FString const& myPlayerName, float deltaSeconds);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void	FireShotServerUpdate(FVector FireDirection);
	void	FireShotServerUpdate_Implementation(FVector FireDirection);
	bool	FireShotServerUpdate_Validate(FVector FireDirection);

	UFUNCTION(Client, Reliable, WithValidation)
	void	ClientUpdateInput(FVector pawnPosition);

	void	ClientUpdateInput_Implementation(FVector pawnPosition);
	bool	ClientUpdateInput_Validate(FVector pawnPosition);


	void UpdateServer();

	bool bUpdatedMoves = false;
	TArray<FSavedMove> savedMoves;
	FSavedMove CorrectMove;

public:
	/** Returns ShipMeshComponent subobject **/
	FORCEINLINE class UStaticMeshComponent* GetShipMeshComponent() const { return ShipMeshComponent; }
	/** Returns CameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE int GetHealth() const { return CurrentHealthPoints; }
};