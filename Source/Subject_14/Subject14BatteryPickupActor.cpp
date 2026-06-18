#include "Subject14BatteryPickupActor.h"
#include "Subject14FirstPersonCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14BatteryPickupActor)

ASubject14BatteryPickupActor::ASubject14BatteryPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderAsset.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderAsset.Object);
	}

	Mesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.18f));

	static ConstructorHelpers::FObjectFinder<USoundWave> PickupAsset(
		TEXT("/Game/Audio/BatteryPickup.BatteryPickup"));
	if (PickupAsset.Succeeded())
	{
		PickupSound = PickupAsset.Object;
	}
}

void ASubject14BatteryPickupActor::BeginPlay()
{
	Super::BeginPlay();

	// FObjectFinder is ctor-only; use LoadObject outside the constructor.
	if (!SpentPickupMesh)
	{
		SpentPickupMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}
}

void ASubject14BatteryPickupActor::Subject14Interact_Implementation(AActor* Instigator)
{
	if (bConsumed)
	{
		return;
	}

	ASubject14FirstPersonCharacter* const Player = Cast<ASubject14FirstPersonCharacter>(Instigator);
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("Subject14BatteryPickup: instigator is not Subject14FirstPersonCharacter"));
		return;
	}

	if (!bAllowPickupWhenBatteryFull && Player->GetFlashlightBatteryFraction() >= 1.0f - KINDA_SMALL_NUMBER)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				(uint64)((PTRINT)this) + 1u,
				1.5f,
				FColor::Orange,
				TEXT("Flashlight already full."));
		}
		return;
	}

	Player->AddFlashlightBattery(ChargeFraction);
	bConsumed = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			(uint64)((PTRINT)this),
			2.0f,
			FColor::Cyan,
			FString::Printf(TEXT("Picked up battery (+%d%%)"), FMath::RoundToInt(ChargeFraction * 100.0f)));
	}

	PlayPickupFeedback();
	ApplySpentPickupVisual();

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PickupCleanupTimerHandle,
			this,
			&ASubject14BatteryPickupActor::HandlePickupCleanupTimer,
			FMath::Clamp(PickupFxHoldSeconds, 0.05f, 5.0f),
			false);
	}
}

void ASubject14BatteryPickupActor::PlayPickupFeedback()
{
	if (PickupSound && GetWorld())
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}
}

void ASubject14BatteryPickupActor::ApplySpentPickupVisual()
{
	if (!Mesh)
	{
		return;
	}

	if (SpentPickupMesh)
	{
		Mesh->SetStaticMesh(SpentPickupMesh);
	}

	Mesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.06f));
	Mesh->SetWorldRotation(FRotator(12.0f, GetActorRotation().Yaw + 17.0f, 4.0f));
}

void ASubject14BatteryPickupActor::HandlePickupCleanupTimer()
{
	if (bDestroyAfterPickup)
	{
		Destroy();
		return;
	}

	Mesh->SetVisibility(false);
}
