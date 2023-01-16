// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "TwinStickShooterPawn.h"
#include "TwinStickShooter.h"
#include "TwinStickShooterProjectile.h"
#include "TimerManager.h"
#include <Runtime/Engine/Classes/Engine/Engine.h>

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5, FColor::White,text)

const FName ATwinStickShooterPawn::MoveForwardBinding("MoveForward");
const FName ATwinStickShooterPawn::MoveRightBinding("MoveRight");
const FName ATwinStickShooterPawn::FireForwardBinding("FireForward");
const FName ATwinStickShooterPawn::FireRightBinding("FireRight");


ATwinStickShooterPawn::ATwinStickShooterPawn()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShipMesh(TEXT("/Game/TwinStick/Meshes/TwinStickUFO.TwinStickUFO"));
	// Create the mesh component
	ShipMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMeshComponent;
	ShipMeshComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	ShipMeshComponent->SetStaticMesh(ShipMesh.Object);

	// Cache our sound effect
	static ConstructorHelpers::FObjectFinder<USoundBase> FireAudio(TEXT("/Game/TwinStick/Audio/TwinStickFire.TwinStickFire"));
	FireSound = FireAudio.Object;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetAbsolute(false, true, false); // Don't want arm to rotate when ship does
	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->SetRelativeRotation(FRotator(-80.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;	// Camera does not rotate relative to arm

	// Movement
	MoveSpeed = 1000.0f;
	// Weapon
	GunOffset = FVector(90.f, 0.f, 0.f);
	FireRate = 0.1f;
	bCanFire = true;
	bIsAlive = true;

	CurrentHealthPoints = MaxHealthPoints;

	SetReplicates(true);
}

void ATwinStickShooterPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	// set up gameplay key bindings
	PlayerInputComponent->BindAxis(MoveForwardBinding);
	PlayerInputComponent->BindAxis(MoveRightBinding);
	PlayerInputComponent->BindAxis(FireForwardBinding);
	PlayerInputComponent->BindAxis(FireRightBinding);
}

void ATwinStickShooterPawn::Tick(float DeltaSeconds)
{
	if (!bIsAlive)
		return;

	// Find movement direction
	ForwardValue = GetInputAxisValue(MoveForwardBinding);
	RightValue = GetInputAxisValue(MoveRightBinding);


	// Clamp max size so that (X=1, Y=1) doesn't cause faster movement in diagonal directions
	MoveDirection = FVector(ForwardValue, RightValue, 0.f).GetClampedToMaxSize(1.0f);

	// Create fire direction vector
	const float FireForwardValue = GetInputAxisValue(FireForwardBinding);
	const float FireRightValue = GetInputAxisValue(FireRightBinding);
	const FVector FireDirection = FVector(FireForwardValue, FireRightValue, 0.f);

	//if (GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
	UpdatePawnRPC(MoveDirection, FireDirection, PlayerName, DeltaSeconds);
}

void ATwinStickShooterPawn::ComputeMove(FVector moveDir, float DeltaSeconds)
{
	if (ForwardValue == 0.0f && RightValue == 0.0f)
		return;

	// Calculate  movement
	const FVector Movement = MoveDirection * MoveSpeed * DeltaSeconds;

	// If non-zero size, move this actor
	if (Movement.SizeSquared() > 0.0f)
	{
		const FRotator NewRotation = Movement.Rotation();
		FHitResult Hit(1.f);
		RootComponent->MoveComponent(Movement, NewRotation, true, &Hit);

		if (Hit.IsValidBlockingHit())
		{
			const FVector Normal2D = Hit.Normal.GetSafeNormal2D();
			const FVector Deflection = FVector::VectorPlaneProject(Movement, Normal2D) * (1.f - Hit.Time);
			RootComponent->MoveComponent(Deflection, NewRotation, true);
		}
	}
}

void ATwinStickShooterPawn::ComputeMoveServer(FVector moveDirection, FVector pawnPosition, float DeltaSeconds)
{
	// Calculate  movement
	const FVector Movement = moveDirection * MoveSpeed * DeltaSeconds;

	// If non-zero size, move this actor
	if (Movement.SizeSquared() > 0.0f)
	{
		const FRotator NewRotation = Movement.Rotation();
		FHitResult Hit(1.f);
		RootComponent->MoveComponent(Movement, NewRotation, true, &Hit);

		if (Hit.IsValidBlockingHit())
		{
			const FVector Normal2D = Hit.Normal.GetSafeNormal2D();
			const FVector Deflection = FVector::VectorPlaneProject(Movement, Normal2D) * (1.f - Hit.Time);
			RootComponent->MoveComponent(Deflection, NewRotation, true);
		}
	}

	if (FVector::Dist(GetActorLocation(), pawnPosition) > 0.1f)
	{
		print("pawnPos: " + pawnPosition.ToString());
		print("GetActor: " + GetActorLocation().ToString());
		ClientUpdateInput(GetActorLocation());
	}
}

void ATwinStickShooterPawn::FireShot(FVector FireDirection)
{
	// If we it's ok to fire again
	if (bCanFire == true)
	{
		// If we are pressing fire stick in a direction
		if (FireDirection.SizeSquared() > 0.0f)
		{
			const FRotator FireRotation = FireDirection.Rotation();
			// Spawn projectile at an offset from this pawn
			const FVector SpawnLocation = GetActorLocation() + FireRotation.RotateVector(GunOffset);

			UWorld* const World = GetWorld();
			if (World != NULL)
			{
				// spawn the projectile
				ATwinStickShooterProjectile* NewProjectile = World->SpawnActor<ATwinStickShooterProjectile>(SpawnLocation, FireRotation);
				NewProjectile->SetPlayerOwner(this);
			}

			bCanFire = false;
			World->GetTimerManager().SetTimer(TimerHandle_ShotTimerExpired, this, &ATwinStickShooterPawn::ShotTimerExpired, FireRate);

			bCanFire = false;
		}
	}
}

void ATwinStickShooterPawn::FireShotServer(FVector FireDirection)
{
	FireShotServerUpdate(FireDirection);
}

void ATwinStickShooterPawn::ShotTimerExpired()
{
	bCanFire = true;
}

void ATwinStickShooterPawn::PlayerDie_Implementation()
{
	print("Player is dead");
	bIsAlive = false;
	//bCanFire = false;
	CurrentHealthPoints = 0;
	ShipMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//SetHidden(true);
	ShipMeshComponent->SetVisibility(false);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_Revive, this, &ATwinStickShooterPawn::Revive, ReviveDelay, false);
}

bool ATwinStickShooterPawn::PlayerDie_Validate()
{
	return true;
}

void ATwinStickShooterPawn::Revive()
{
	print("Reviving Player !");
	bIsAlive = true;
	//bCanFire = true;
	CurrentHealthPoints = MaxHealthPoints;
	ShipMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ShipMeshComponent->SetVisibility(true);
}

float ATwinStickShooterPawn::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.f && bIsAlive)
	{
		CurrentHealthPoints -= ActualDamage;

		if (CurrentHealthPoints <= 0)
		{
			PlayerDie();
		}
	}
	return ActualDamage;
}

void ATwinStickShooterPawn::UpdateServer()
{
	if (!bUpdatedMoves)
		return;

	for (int32 i = 0; i < savedMoves.Num(); i++)
	{
		const FSavedMove& move = savedMoves[i];

		ComputeMove(move.moveDirection, move.deltaTime);
	}
}

void ATwinStickShooterPawn::UpdatePawnRPC(FVector moveDir, FVector shootDir, FString const& myPlayerName, float DeltaSeconds)
{
	UpdateServer();

	ComputeMove(moveDir, DeltaSeconds);

	if (GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
		FireShot(shootDir);
	else
		FireShotServerUpdate(shootDir);

	if (GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
		ServerUpdateInput(MoveDirection, GetActorLocation(), shootDir, myPlayerName, DeltaSeconds);
}

void ATwinStickShooterPawn::FireShotServerUpdate_Implementation(FVector shootDir)
{
	if (GetLocalRole() != ENetRole::ROLE_AutonomousProxy)
		FireShot(shootDir);
}

bool ATwinStickShooterPawn::FireShotServerUpdate_Validate(FVector shootDir)
{
	return true;
}

void ATwinStickShooterPawn::ServerUpdateInput_Implementation(FVector moveDir, FVector actorDir, FVector shootDir, FString const& myPlayerName, float deltaSeconds)
{
	ComputeMoveServer(moveDir, actorDir, deltaSeconds);
	FireShotServer(shootDir);
}

bool ATwinStickShooterPawn::ServerUpdateInput_Validate(FVector moveDir, FVector actorDir, FVector shootDir, FString const& myPlayerName, float deltaSeconds)
{
	return true;
}

void ATwinStickShooterPawn::ClientUpdateInput_Implementation(FVector pawnPosition)
{
	// RPC executee chez le client : snap de position
	SetActorLocation(pawnPosition);
	bUpdatedMoves = true;
}

bool ATwinStickShooterPawn::ClientUpdateInput_Validate(FVector pawnPosition)
{
	return true;
}
