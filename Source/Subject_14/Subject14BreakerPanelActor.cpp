#include "Subject14BreakerPanelActor.h"

#include "Subject14CabinHatchActor.h"
#include "Subject14StorySubsystem.h"
#include "Subject14ThoughtOverlayWidget.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14BreakerPanelActor)

namespace Subject14BreakerPrivate
{
static constexpr float ThoughtHold = 5.0f;
static constexpr float ThoughtFadeIn = 0.5f;
static constexpr float ThoughtFadeOut = 0.95f;
}

ASubject14BreakerPanelActor::ASubject14BreakerPanelActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PanelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PanelMesh"));
	RootComponent = PanelMesh;
	PanelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PanelMesh->SetCollisionResponseToAllChannels(ECR_Block);
	PanelMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		PanelMesh->SetStaticMesh(CubeFinder.Object);
		PanelMesh->SetRelativeScale3D(FVector(0.35f, 0.25f, 0.55f));
	}
}

void ASubject14BreakerPanelActor::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	if (bWarnIfNoLinkedHatches && LinkedHatches.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s: no LinkedHatches — HatchHasPower will still be set globally; place hatch references for local mesh refresh."),
			*GetName());
	}
#endif
}

bool ASubject14BreakerPanelActor::EvaluateGate(const USubject14StorySubsystem& Subsystem) const
{
	if (!RequiredStoryFlag.IsNone() && !Subsystem.HasStoryFlag(RequiredStoryFlag))
	{
		return false;
	}
	if (!BlockedByStoryFlag.IsNone() && Subsystem.HasStoryFlag(BlockedByStoryFlag))
	{
		return false;
	}
	if ((uint8)Subsystem.GetCurrentPhase() < (uint8)RequiredPhaseAtLeast)
	{
		return false;
	}
	return true;
}

bool ASubject14BreakerPanelActor::IsInstigatorPrimarySinglePlayerPawn(AActor* const Instigator) const
{
	UWorld* const World = GetWorld();
	if (!World || World->GetNumPlayerControllers() <= 1)
	{
		return true;
	}
	const APlayerController* const PC0 = UGameplayStatics::GetPlayerController(World, 0);
	const APawn* const P0 = PC0 ? PC0->GetPawn() : nullptr;
	return Instigator != nullptr && Cast<APawn>(Instigator) == P0;
}

bool ASubject14BreakerPanelActor::IsPermanentlyConsumed(const USubject14StorySubsystem& Subsystem) const
{
	if (!ConsumedStoryFlag.IsNone() && Subsystem.HasStoryFlag(ConsumedStoryFlag))
	{
		return true;
	}
	return bLocalConsumed;
}

void ASubject14BreakerPanelActor::ApplyPower(AActor* const Instigator)
{
	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}

	Subsystem->SetStoryFlag(Subject14StoryFlags::HatchHasPower, true);

	if (!GrantedStoryFlag.IsNone())
	{
		Subsystem->SetStoryFlag(GrantedStoryFlag, true);
	}

	if (!ConsumedStoryFlag.IsNone())
	{
		Subsystem->SetStoryFlag(ConsumedStoryFlag, true);
	}
	else
	{
		bLocalConsumed = true;
	}

	for (ASubject14CabinHatchActor* Hatch : LinkedHatches)
	{
		if (Hatch)
		{
			Hatch->RefreshPresentationFromSubsystem();
		}
	}

	if (SwitchThrowSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwitchThrowSound, GetActorLocation());
	}
	if (PowerOnHumSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PowerOnHumSound, GetActorLocation());
	}

	if (!ThoughtOnUse.IsEmpty())
	{
		USubject14ThoughtOverlayWidget::ShowThoughtLine(
			this,
			ThoughtOnUse,
			Subject14BreakerPrivate::ThoughtHold,
			Subject14BreakerPrivate::ThoughtFadeIn,
			Subject14BreakerPrivate::ThoughtFadeOut);
	}

	if (!ObjectiveAfterUse.IsEmpty())
	{
		Subsystem->SetCurrentObjectiveLine(ObjectiveAfterUse);
	}

	if (bSaveImmediatelyAfterUse)
	{
		Subsystem->SaveProgressToSlot();
	}

	UE_LOG(LogTemp, Log,
		TEXT("%s: facility power routed to hatch line (%s)."),
		*GetName(),
		Instigator ? *Instigator->GetName() : TEXT("<none>"));
}

void ASubject14BreakerPanelActor::Subject14Interact_Implementation(AActor* const Instigator)
{
	if (!Instigator || !IsInstigatorPrimarySinglePlayerPawn(Instigator))
	{
		return;
	}

	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}

	if (bOneShot && IsPermanentlyConsumed(*Subsystem))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Verbose, TEXT("%s: interact ignored (one-shot consumed)."), *GetName());
#endif
		return;
	}

	if (!EvaluateGate(*Subsystem))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Verbose, TEXT("%s: interact blocked by gate."), *GetName());
#endif
		return;
	}

	ApplyPower(Instigator);
}
