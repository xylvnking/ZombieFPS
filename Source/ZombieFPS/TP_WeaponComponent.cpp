// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "ZombieFPSCharacter.h"
#include "ZombieFPSProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	CurrentAmmo = 9999;

	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	//RotationBeforeFiring = FRotator(0.0, 0.0, 0.0);
}

void UTP_WeaponComponent::AttachWeapon(AZombieFPSCharacter* TargetCharacter)
{
	Character = TargetCharacter;
	if (Character == nullptr)
	{
		return;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// switch bHasRifle so the animation blueprint can switch to another animation set
	Character->SetHasRifle(true);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			//EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::FireProjectile);

			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &UTP_WeaponComponent::OnStartFire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &UTP_WeaponComponent::OnStopFire);

			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::GetLookData);
		}
	}
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
			Subsystem->RemoveMappingContext(DefaultMappingContext);
		}
	}
}

void UTP_WeaponComponent::BeginPlay()
{
	// set up recoil curve timeline
	// this code I need to dive into



	FOnTimelineFloat XRecoilCurve;
	FOnTimelineFloat YRecoilCurve;

	XRecoilCurve.BindUFunction(this, FName("StartHorizontalRecoil"));
	YRecoilCurve.BindUFunction(this, FName("StartVerticalRecoil"));

	if (!HorizontalCurve || !VerticalCurve) { return; }

	RecoilTimeline.AddInterpFloat(HorizontalCurve, XRecoilCurve);
	RecoilTimeline.AddInterpFloat(VerticalCurve, YRecoilCurve);
}



void UTP_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, LEVELTICK_All, ThisTickFunction);

	if (RecoilTimeline.IsPlaying())
	{
		RecoilTimeline.TickTimeline(DeltaTime);
		UE_LOG(LogTemp, Warning, TEXT("recoil playing"));
	}

	if (RecoilTimeline.IsReversing())
	{
		if (FMath::Abs(MostRecentLookValuesFromWeaponInputComponent.X) > 0.08 || FMath::Abs(MostRecentLookValuesFromWeaponInputComponent.Y) > 0.08)
		{
			RecoilTimeline.Stop();
			return;
		}

		RecoilTimeline.TickTimeline(DeltaTime);

		UE_LOG(LogTemp, Warning, TEXT("recoil is reversing"));

		// this check wasn't required before piecing the project back together. It makes sense, but I don't get why I didn't need it before.
		if (Character == nullptr || Character->GetController() == nullptr) { return; }
		FRotator NewRotation = UKismetMathLibrary::RInterpTo(Character->GetControlRotation(), RotationBeforeFiring, DeltaTime, 2.0f);
		Character->Controller->ClientSetRotation(NewRotation);
	}

	UE_LOG(LogTemp, Warning, TEXT("component is ticking"));
	
}

void UTP_WeaponComponent::OnStartFire()
{
	if (CurrentAmmo <= 0) {
		RecoilTimeline.Stop();
		return;
	}
	RecoilTimeline.PlayFromStart();
	FireHitScan();
	GetWorld()->GetTimerManager().SetTimer(AutomaticFireHandle, this, &UTP_WeaponComponent::FireHitScan, 0.1, true);
}

void UTP_WeaponComponent::OnStopFire()
{
	if (CurrentAmmo <= 0) {
		RecoilTimeline.Stop();
		return;
	}
	RecoilTimeline.ReverseFromEnd();
	GetWorld()->GetTimerManager().ClearTimer(AutomaticFireHandle);
}

void UTP_WeaponComponent::FireProjectile()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			// Spawn the projectile at the muzzle
			World->SpawnActor<AZombieFPSProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}

	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}

	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void UTP_WeaponComponent::FireHitScan()
{
	if (Character == nullptr || Character->GetController() == nullptr) { return; }
	UWorld* const World = GetWorld();
	if (CurrentAmmo > 0 && World != nullptr)
	{
		RotationBeforeFiring = Character->GetControlRotation();
		CurrentAmmo--;
		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		FHitResult Hit;
		FVector CameraLocation;
		FRotator CameraRotation;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
		FCollisionQueryParams Params;
		Params.AddIgnoredComponent(this);
		FVector TraceStart = CameraLocation;
		FVector TraceEnd = TraceStart + CameraRotation.Vector() * 10000;
		bool bHasHitSomething = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

		if (bHasHitSomething)
		{
			UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitParticleFx, Hit.Location, Hit.ImpactNormal.Rotation(), FVector(0.2f), true, true);
			NiagaraComp->SetNiagaraVariableFloat(FString("StrengthCoef"), CoefStrength);

			if (!HitDecalMaterial) { return; }
			UGameplayStatics::SpawnDecalAtLocation(World, HitDecalMaterial, FVector(15.0f), Hit.Location, Hit.ImpactNormal.Rotation(), 10.0f);
		}
	}
	else
	{
		OnStopFire();
	}
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

}

void UTP_WeaponComponent::GetLookData(const FInputActionValue& Value)
{
	//UE_LOG(LogTemp, Warning, TEXT("getting look data"));FVector2D LookAxisVector = Value.Get<FVector2D>();
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	MostRecentLookValuesFromWeaponInputComponent = FVector2D(LookAxisVector.X, LookAxisVector.Y);
}




void UTP_WeaponComponent::StartHorizontalRecoil(float Value)
{
	if (RecoilTimeline.IsReversing()) { return; }
	Character->AddControllerYawInput(Value);
}

void UTP_WeaponComponent::StartVerticalRecoil(float Value)
{
	if (RecoilTimeline.IsReversing()) { return; }
	Character->AddControllerPitchInput(Value);
}

