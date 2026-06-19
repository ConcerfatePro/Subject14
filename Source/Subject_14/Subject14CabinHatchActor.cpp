#include "Subject14CabinHatchActor.h"

#include "Subject14StorySubsystem.h"
#include "Subject14ThoughtOverlayWidget.h"

#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14CabinHatchActor)

namespace Subject14HatchPrivate
{
static constexpr float ThoughtHold = 5.2f;
static constexpr float ThoughtFadeIn = 0.55f;
static constexpr float ThoughtFadeOut = 1.05f;
}

ASubject14CabinHatchActor::ASubject14CabinHatchActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	InteractProxy = CreateDefaultSubobject<USphereComponent>(TEXT("InteractProxy"));
	InteractProxy->SetupAttachment(RootScene);
	InteractProxy->InitSphereRadius(180.0f);
	InteractProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractProxy->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractProxy->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractProxy->SetHiddenInGame(true);
	InteractProxy->SetVisibility(false);

	RugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RugMesh"));
	RugMesh->SetupAttachment(RootScene);
	RugMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RugMesh->SetCollisionResponseToAllChannels(ECR_Block);
	RugMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(RootScene);

	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(Hinge);
	LidMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	LidMesh->SetCollisionResponseToAllChannels(ECR_Block);
	LidMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	LockIndicatorLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("LockIndicatorLight"));
	LockIndicatorLight->SetupAttachment(LidMesh);
	LockIndicatorLight->SetIntensity(0.0f);
	LockIndicatorLight->SetAttenuationRadius(220.0f);
	LockIndicatorLight->SetOuterConeAngle(44.0f);
	LockIndicatorLight->SetVisibility(false);
	LockIndicatorLight->SetCastShadows(false);

	DiscoveryVolume = CreateDefaultSubobject<USphereComponent>(TEXT("DiscoveryVolume"));
	DiscoveryVolume->SetupAttachment(RootScene);
	DiscoveryVolume->InitSphereRadius(140.0f);
	DiscoveryVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DiscoveryVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DiscoveryVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DiscoveryVolume->SetGenerateOverlapEvents(true);

	HumAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("HumAudio"));
	HumAudio->SetupAttachment(RootScene);
	HumAudio->bAutoActivate = false;

	DescentAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("DescentAnchor"));
	DescentAnchor->SetupAttachment(RootScene);
	DescentAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, -220.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		RugMesh->SetStaticMesh(CubeFinder.Object);
		RugMesh->SetRelativeScale3D(FVector(2.2f, 1.6f, 0.06f));
		LidMesh->SetStaticMesh(CubeFinder.Object);
		LidMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.1f));
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> HumFinder(TEXT("/Game/Audio/MonitorWhineLoop.MonitorWhineLoop"));
	if (HumFinder.Succeeded())
	{
		HumLoopSound = HumFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> DiscoveryFinder(TEXT("/Game/Audio/pipeping.pipeping"));
	if (DiscoveryFinder.Succeeded())
	{
		DiscoverySound = DiscoveryFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> UnlockFinder(TEXT("/Game/Audio/LockedDoor.LockedDoor"));
	if (UnlockFinder.Succeeded())
	{
		UnlockSound = UnlockFinder.Object;
		SealedNoPowerSound = UnlockFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> OpenFinder(TEXT("/Game/Audio/DoorOpen.DoorOpen"));
	if (OpenFinder.Succeeded())
	{
		OpenSound = OpenFinder.Object;
	}

	ThoughtOnProximityCue = TEXT("The floor sounds hollow here.");
	ThoughtOnDiscovery = TEXT("This isn't a cabin.");
	ThoughtOnNoPower = TEXT("No power. The lock won't release.");
	ThoughtOnUnlock = TEXT("Lock released.");
	ThoughtWhenGateBlocked = TEXT("Not yet. Keep searching.");
}

void ASubject14CabinHatchActor::BeginPlay()
{
	Super::BeginPlay();

	if (DiscoveryVolume)
	{
		DiscoveryVolume->OnComponentBeginOverlap.AddDynamic(this, &ASubject14CabinHatchActor::HandleDiscoveryOverlap);
	}

	if (USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this))
	{
		Subsystem->OnStoryFlagChanged.AddUObject(this, &ASubject14CabinHatchActor::HandleStoryFlagChanged);
	}

	RefreshPresentationFromSubsystem();

#if !UE_BUILD_SHIPPING
	if (!LidMesh || !Hinge)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: lid / hinge missing — hatch animation may fail."), *GetName());
	}
#endif
}

void ASubject14CabinHatchActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this))
	{
		Subsystem->OnStoryFlagChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ASubject14CabinHatchActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bOpeningInterp || !Hinge)
	{
		return;
	}

	LidInterpAlpha = FMath::FInterpTo(LidInterpAlpha, 1.0f, DeltaSeconds, LidOpenInterpSpeed);
	const float Yaw = FMath::Lerp(0.0f, LidOpenYawDegrees, LidInterpAlpha);
	Hinge->SetRelativeRotation(FRotator(0.0f, Yaw, 0.0f));

	if (LidInterpAlpha >= 0.999f)
	{
		FinishOpening();
	}
}

void ASubject14CabinHatchActor::HandleStoryFlagChanged(const FName Flag, const bool /*bValue*/)
{
	if (Flag == Subject14StoryFlags::HatchDiscovered
		|| Flag == Subject14StoryFlags::HatchHasPower
		|| Flag == Subject14StoryFlags::HatchUnlocked
		|| Flag == Subject14StoryFlags::HatchOpened)
	{
		RefreshPresentationFromSubsystem();
	}
}

void ASubject14CabinHatchActor::HandleDiscoveryOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* const OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	APawn* const Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}
	const APlayerController* const PC = Cast<APlayerController>(Pawn->GetController());
	if (!PC || !PC->IsLocalPlayerController())
	{
		return;
	}

	if (USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this))
	{
		if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchDiscovered))
		{
			return;
		}
	}

	bProximityCue = true;

	if (!ThoughtOnProximityCue.IsEmpty())
	{
		USubject14ThoughtOverlayWidget::ShowThoughtLine(
			this,
			ThoughtOnProximityCue,
			Subject14HatchPrivate::ThoughtHold,
			Subject14HatchPrivate::ThoughtFadeIn,
			Subject14HatchPrivate::ThoughtFadeOut);
	}

	UE_LOG(LogTemp, Log, TEXT("%s: proximity discovery cue (transient)."), *GetName());
}

bool ASubject14CabinHatchActor::EvaluateStoryGate(const USubject14StorySubsystem& Subsystem) const
{
	if (!RequiredStoryFlag.IsNone() && !Subsystem.HasStoryFlag(RequiredStoryFlag))
	{
		return false;
	}
	if (!BlockedByStoryFlag.IsNone() && Subsystem.HasStoryFlag(BlockedByStoryFlag))
	{
		return false;
	}
	return true;
}

bool ASubject14CabinHatchActor::IsInstigatorPrimarySinglePlayerPawn(AActor* const Instigator) const
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

ESubject14CabinHatchState ASubject14CabinHatchActor::GetHatchState() const
{
	const USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return ESubject14CabinHatchState::Concealed;
	}

	if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchOpened))
	{
		return ESubject14CabinHatchState::Open;
	}
	if (bOpeningInterp)
	{
		return ESubject14CabinHatchState::Opening;
	}
	if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchUnlocked))
	{
		return ESubject14CabinHatchState::Unlocked;
	}
	if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchDiscovered) && Subsystem->HasStoryFlag(Subject14StoryFlags::HatchHasPower)
		&& !Subsystem->HasStoryFlag(Subject14StoryFlags::HatchUnlocked))
	{
		return ESubject14CabinHatchState::LockedPowered;
	}
	if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchDiscovered) && !Subsystem->HasStoryFlag(Subject14StoryFlags::HatchHasPower))
	{
		return ESubject14CabinHatchState::LockedNoPower;
	}
	if (!Subsystem->HasStoryFlag(Subject14StoryFlags::HatchDiscovered) && bProximityCue)
	{
		return ESubject14CabinHatchState::Discovered;
	}
	return ESubject14CabinHatchState::Concealed;
}

void ASubject14CabinHatchActor::SetHumActive(const bool bActive)
{
	if (!HumAudio)
	{
		return;
	}
	if (bActive && HumLoopSound)
	{
		HumAudio->SetSound(HumLoopSound);
		HumAudio->Play();
	}
	else
	{
		HumAudio->Stop();
	}
}

void ASubject14CabinHatchActor::RefreshPresentationFromSubsystem()
{
	RefreshPresentation();
}

void ASubject14CabinHatchActor::RefreshPresentation()
{
	const USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	const bool bDiscovered = Subsystem && Subsystem->HasStoryFlag(Subject14StoryFlags::HatchDiscovered);
	const bool bPowered = Subsystem && Subsystem->HasStoryFlag(Subject14StoryFlags::HatchHasPower);
	const bool bUnlocked = Subsystem && Subsystem->HasStoryFlag(Subject14StoryFlags::HatchUnlocked);
	const bool bOpened = Subsystem && Subsystem->HasStoryFlag(Subject14StoryFlags::HatchOpened);

	if (RugMesh)
	{
		RugMesh->SetHiddenInGame(bDiscovered);
		RugMesh->SetCollisionEnabled(bDiscovered ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}

	if (LockIndicatorLight)
	{
		LockIndicatorLight->SetIntensity(0.0f);
	}

	if (InteractProxy)
	{
		InteractProxy->SetCollisionEnabled(bOpened ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}

	SetHumActive(bPowered && bDiscovered && !bOpened && !bOpeningInterp);

	if (bOpened && Hinge)
	{
		Hinge->SetRelativeRotation(FRotator(0.0f, LidOpenYawDegrees, 0.0f));
		LidInterpAlpha = 1.0f;
	}
}

void ASubject14CabinHatchActor::TryCommitDiscoveryFromInteract(AActor* const Instigator)
{
	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}

	if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchDiscovered))
	{
		return;
	}

	if (bRequireProximityCueBeforeFormalDiscovery && !bProximityCue)
	{
		if (!ThoughtOnOpenBlocked.IsEmpty())
		{
			USubject14ThoughtOverlayWidget::ShowThoughtLine(
				this,
				ThoughtOnOpenBlocked,
				Subject14HatchPrivate::ThoughtHold,
				Subject14HatchPrivate::ThoughtFadeIn,
				Subject14HatchPrivate::ThoughtFadeOut);
		}
		return;
	}

	Subsystem->SetStoryFlag(Subject14StoryFlags::HatchDiscovered, true);
	bProximityCue = false;

	if (DiscoverySound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DiscoverySound, GetActorLocation());
	}

	if (!ThoughtOnDiscovery.IsEmpty())
	{
		USubject14ThoughtOverlayWidget::ShowThoughtLine(
			this,
			ThoughtOnDiscovery,
			Subject14HatchPrivate::ThoughtHold,
			Subject14HatchPrivate::ThoughtFadeIn,
			Subject14HatchPrivate::ThoughtFadeOut);
	}

	UE_LOG(LogTemp, Log, TEXT("%s: HatchDiscovered committed (%s)."), *GetName(), Instigator ? *Instigator->GetName() : TEXT("<none>"));

	Subsystem->SetCurrentObjectiveLine(TEXT("Restore local power."));
	if (bSaveProgressOnStateChange)
	{
		Subsystem->SaveProgressToSlot();
	}

	RefreshPresentation();
}

void ASubject14CabinHatchActor::TryUnlockFromPowered(AActor* const Instigator)
{
	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}

	if (!RequiredStoryFlagToUnlock.IsNone() && !Subsystem->HasStoryFlag(RequiredStoryFlagToUnlock))
	{
		if (!ThoughtOnPoweredSealed.IsEmpty())
		{
			USubject14ThoughtOverlayWidget::ShowThoughtLine(
				this,
				ThoughtOnPoweredSealed,
				Subject14HatchPrivate::ThoughtHold,
				Subject14HatchPrivate::ThoughtFadeIn,
				Subject14HatchPrivate::ThoughtFadeOut);
		}
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Verbose, TEXT("%s: unlock blocked — missing flag %s"), *GetName(), *RequiredStoryFlagToUnlock.ToString());
#endif
		return;
	}

	if (!bUnlockOnInteractWhenPowered)
	{
		return;
	}

	Subsystem->SetStoryFlag(Subject14StoryFlags::HatchUnlocked, true);

	if (UnlockSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, UnlockSound, GetActorLocation());
	}

	if (!ThoughtOnUnlock.IsEmpty())
	{
		USubject14ThoughtOverlayWidget::ShowThoughtLine(
			this,
			ThoughtOnUnlock,
			Subject14HatchPrivate::ThoughtHold,
			Subject14HatchPrivate::ThoughtFadeIn,
			Subject14HatchPrivate::ThoughtFadeOut);
	}

	if (bAdvanceMacroPhaseToHatchUnlockedOnUnlock && Subsystem->GetCurrentPhase() == ESubject14StoryPhase::Day3BreachPrep)
	{
		Subsystem->SetPhase(ESubject14StoryPhase::HatchUnlocked);
	}

	Subsystem->SetCurrentObjectiveLine(TEXT("Open the hatch."));

	UE_LOG(LogTemp, Log, TEXT("%s: hatch unlocked by %s."), *GetName(), *Instigator->GetName());

	if (bSaveProgressOnStateChange)
	{
		Subsystem->SaveProgressToSlot();
	}

	RefreshPresentation();
}

void ASubject14CabinHatchActor::TryBeginOpen(AActor* const Instigator)
{
	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem || Subsystem->HasStoryFlag(Subject14StoryFlags::HatchOpened))
	{
		return;
	}

	if (!Subsystem->HasStoryFlag(Subject14StoryFlags::HatchUnlocked))
	{
		return;
	}

	if (bOpeningInterp)
	{
		return;
	}

	bOpeningInterp = true;
	LidInterpAlpha = 0.0f;
	SetActorTickEnabled(true);

	if (OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
	}

	UE_LOG(LogTemp, Log, TEXT("%s: hatch opening started (%s)."), *GetName(), *Instigator->GetName());
	RefreshPresentation();
}

void ASubject14CabinHatchActor::FinishOpening()
{
	bOpeningInterp = false;
	SetActorTickEnabled(false);

	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}

	if (!Subsystem->HasStoryFlag(Subject14StoryFlags::HatchOpened))
	{
		Subsystem->SetStoryFlag(Subject14StoryFlags::HatchOpened, true);
		Subsystem->SetPhase(ESubject14StoryPhase::FirstBreach);
		Subsystem->SetCurrentObjectiveLine(FString());
		if (bSaveProgressOnStateChange)
		{
			Subsystem->SaveProgressToSlot();
		}
		UE_LOG(LogTemp, Warning, TEXT("%s: FIRST BREACH — HatchOpened + phase FirstBreach."), *GetName());
	}

	RefreshPresentation();
}

void ASubject14CabinHatchActor::Subject14Interact_Implementation(AActor* const Instigator)
{
	if (!Instigator || !IsInstigatorPrimarySinglePlayerPawn(Instigator))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Verbose, TEXT("%s: interact ignored (instigator guard)."), *GetName());
#endif
		return;
	}

	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem || !EvaluateStoryGate(*Subsystem))
	{
		if (Subsystem && !ThoughtWhenGateBlocked.IsEmpty())
		{
			USubject14ThoughtOverlayWidget::ShowThoughtLine(
				this,
				ThoughtWhenGateBlocked,
				Subject14HatchPrivate::ThoughtHold,
				Subject14HatchPrivate::ThoughtFadeIn,
				Subject14HatchPrivate::ThoughtFadeOut);
		}
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Verbose, TEXT("%s: interact blocked by story gate."), *GetName());
#endif
		return;
	}

	const ESubject14CabinHatchState State = GetHatchState();

	switch (State)
	{
	case ESubject14CabinHatchState::Concealed:
	case ESubject14CabinHatchState::Discovered:
		TryCommitDiscoveryFromInteract(Instigator);
		if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchHasPower)
			&& Subsystem->HasStoryFlag(Subject14StoryFlags::HatchDiscovered)
			&& !Subsystem->HasStoryFlag(Subject14StoryFlags::HatchUnlocked))
		{
			TryUnlockFromPowered(Instigator);
		}
		if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchUnlocked))
		{
			TryBeginOpen(Instigator);
		}
		break;
	case ESubject14CabinHatchState::LockedNoPower:
		if (SealedNoPowerSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SealedNoPowerSound, GetActorLocation());
		}
		if (!ThoughtOnNoPower.IsEmpty())
		{
			USubject14ThoughtOverlayWidget::ShowThoughtLine(
				this,
				ThoughtOnNoPower,
				Subject14HatchPrivate::ThoughtHold,
				Subject14HatchPrivate::ThoughtFadeIn,
				Subject14HatchPrivate::ThoughtFadeOut);
		}
		Subsystem->SetCurrentObjectiveLine(TEXT("Restore local power."));
		break;
	case ESubject14CabinHatchState::LockedPowered:
		TryUnlockFromPowered(Instigator);
		if (Subsystem->HasStoryFlag(Subject14StoryFlags::HatchUnlocked))
		{
			TryBeginOpen(Instigator);
		}
		break;
	case ESubject14CabinHatchState::Unlocked:
		TryBeginOpen(Instigator);
		break;
	case ESubject14CabinHatchState::Opening:
		break;
	case ESubject14CabinHatchState::Open:
	default:
		break;
	}
}
