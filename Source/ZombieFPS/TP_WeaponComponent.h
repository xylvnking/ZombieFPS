// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Components/TimelineComponent.h"
#include "TP_WeaponComponent.generated.h"

class AZombieFPSCharacter;
class UNiagaraSystem;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIEFPS_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AZombieFPSProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction;



	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void AttachWeapon(AZombieFPSCharacter* TargetCharacter);

	/** Make the weapon Fire a Projectile (renamed from default Fire() */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireProjectile();



	// do stuff here


protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	



private:
	/** The Character holding this weapon*/
	AZombieFPSCharacter* Character;




public: // everything below this is what I added

	/*
	The DefaultMappingContext contains the Look input action, so we need to declare it here to bind it and access the values
	which are used to cancel the recoil timeline reversing.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Make the weapon Fire a HitScan */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireHitScan();

	/* Used to get the input values to cancel recoil timeline reversing */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void GetLookData(const FInputActionValue& Value);

	// does this need to be public?
	FTimeline RecoilTimeline;

protected: 

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	// Used to cancel recoil
	FVector2D MostRecentLookValuesFromWeaponInputComponent;

	void OnStartFire();
	void OnStopFire();

	FTimerHandle AutomaticFireHandle;
	int32 CurrentAmmo;
	int32 DefaultAmmo = 30;

	FRotator RotationBeforeFiring;

	UFUNCTION()
		void StartHorizontalRecoil(float Value);

	UFUNCTION()
		void StartVerticalRecoil(float Value);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Advanbced Recoil | Curves")
		class UCurveFloat* VerticalCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Advanbced Recoil | Curves")
		class UCurveFloat* HorizontalCurve;

	// particle + vfx stuff

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AdvancedRecoil | FX")
		class UMaterialInterface* HitDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AdvancedRecoil | FX")
		UNiagaraSystem* HitParticleFx;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AdvancedRecoil | FX")
		float CoefStrength;

public:
	

};
