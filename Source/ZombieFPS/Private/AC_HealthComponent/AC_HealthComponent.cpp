// Fill out your copyright notice in the Description page of Project Settings.

#include "AC_HealthComponent/AC_HealthComponent.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UAC_HealthComponent::UAC_HealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	MaxHealth = 100.0f;
	Health = MaxHealth;
	// ...
}


// Called when the game starts
void UAC_HealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		//The Owner Object is now bound to respond to the OnTakeAnyDamage Function.        
		MyOwner->OnTakeAnyDamage.AddDynamic(this, &UAC_HealthComponent::TakeDamage);
	}

	//Set Health Equal to Max Health.           
	Health = MaxHealth;
	
}

void UAC_HealthComponent::TakeDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f)
	{
		return;
		//Destroy();
	}

	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth); 
	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

}


// Called every frame
//void UAC_HealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
//{
//	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
//
//	// ...
//}

