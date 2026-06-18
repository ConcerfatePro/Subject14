#include "Subject14SimpleDoorActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14SimpleDoorActor)

ASubject14SimpleDoorActor::ASubject14SimpleDoorActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	RootComponent = Hinge;

	DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanel"));
	DoorPanel->SetupAttachment(Hinge);
	DoorPanel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorPanel->SetCollisionResponseToAllChannels(ECR_Block);
	DoorPanel->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		DoorPanel->SetStaticMesh(CubeAsset.Object);
	}

	DoorPanel->SetRelativeLocation(FVector(45.0f, 0.0f, 60.0f));
	DoorPanel->SetRelativeScale3D(FVector(0.9f, 0.06f, 1.2f));

	static ConstructorHelpers::FObjectFinder<USoundWave> OpenSfx(TEXT("/Game/Audio/DoorOpen.DoorOpen"));
	if (OpenSfx.Succeeded())
	{
		DoorOpenSound = OpenSfx.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundWave> CloseSfx(TEXT("/Game/Audio/DoorClose.DoorClose"));
	if (CloseSfx.Succeeded())
	{
		DoorCloseSound = CloseSfx.Object;
	}
}

void ASubject14SimpleDoorActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float NewYaw = FMath::FInterpTo(CurrentYawDegrees, TargetYawDegrees, DeltaSeconds, DoorInterpSpeed);
	if (!FMath::IsNearlyEqual(NewYaw, CurrentYawDegrees, 0.05f))
	{
		CurrentYawDegrees = NewYaw;
		Hinge->SetRelativeRotation(FRotator(0.0f, CurrentYawDegrees, 0.0f));
	}
}

void ASubject14SimpleDoorActor::Subject14Interact_Implementation(AActor* Instigator)
{
	bDoorOpen = !bDoorOpen;
	TargetYawDegrees = bDoorOpen ? OpenAngleDegrees : 0.0f;

	if (USoundBase* const Sfx = bDoorOpen ? DoorOpenSound : DoorCloseSound)
	{
		if (UWorld* const World = GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sfx, GetActorLocation());
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			(uint64)((PTRINT)this),
			1.25f,
			FColor(32, 200, 140),
			bDoorOpen ? TEXT("Door: opening") : TEXT("Door: closing"));
	}
}
