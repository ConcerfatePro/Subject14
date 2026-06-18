#include "Subject14NoteActor.h"

#include "Subject14StorySubsystem.h"
#include "Subject14ThoughtOverlayWidget.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14NoteActor)

ASubject14NoteActor::ASubject14NoteActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneAsset.Succeeded())
	{
		Mesh->SetStaticMesh(PlaneAsset.Object);
	}

	Mesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	Mesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));

	NoteTitle = TEXT("Research log (fragment)");
	NoteBody = TEXT(
		"Subject 14 demonstrates stable environmental adaptation. Cabin remains preferred shelter during initial dark-cycle stress periods.");
}

void ASubject14NoteActor::BeginPlay()
{
	Super::BeginPlay();

	if (Mesh && GrayboxMarkerMeshOverride)
	{
		Mesh->SetStaticMesh(GrayboxMarkerMeshOverride);
	}

	WarnDuplicateNoteIdsInLevel();

#if !UE_BUILD_SHIPPING
	if (bRegisterInStorySubsystem && NoteId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s: bRegisterInStorySubsystem is true but NoteId is None — progression will not register."),
			*GetName());
	}
#endif
}

void ASubject14NoteActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DelayedThoughtTimer);
	}
	Super::EndPlay(Reason);
}

bool ASubject14NoteActor::EvaluateStoryGate(const USubject14StorySubsystem& Subsystem) const
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

void ASubject14NoteActor::WarnDuplicateNoteIdsInLevel() const
{
#if !UE_BUILD_SHIPPING
	if (NoteId.IsNone() || !GetWorld())
	{
		return;
	}
	int32 Count = 0;
	for (TActorIterator<ASubject14NoteActor> It(GetWorld()); It; ++It)
	{
		if (It->NoteId == NoteId)
		{
			++Count;
		}
	}
	if (Count > 1)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s: duplicate NoteId '%s' found %d times in level — saves may treat reads ambiguously."),
			*GetName(),
			*NoteId.ToString(),
			Count);
	}
#endif
}

void ASubject14NoteActor::ApplyStoryProgression(USubject14StorySubsystem& Subsystem, const bool bFirstProgressionRead)
{
	if (bRegisterInStorySubsystem && !NoteId.IsNone())
	{
		Subsystem.RegisterReadNote(NoteId);
	}

	if (!GrantedStoryFlag.IsNone() && (!bOnlyFirstReadGrantsProgression || bFirstProgressionRead))
	{
		Subsystem.SetStoryFlag(GrantedStoryFlag, true);
	}

	if (!ObjectiveAfterRead.IsEmpty() && (!bOnlyFirstReadGrantsProgression || bFirstProgressionRead))
	{
		Subsystem.SetCurrentObjectiveLine(ObjectiveAfterRead);
	}

	if (!ThoughtAfterRead.IsEmpty() && (!bOnlyFirstReadGrantsProgression || bFirstProgressionRead))
	{
		if (UWorld* const World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				DelayedThoughtTimer,
				this,
				&ASubject14NoteActor::PlayDelayedThoughtAfterRead,
				FMath::Max(0.0f, ThoughtAfterReadDelaySeconds),
				false);
		}
	}
}

void ASubject14NoteActor::PlayDelayedThoughtAfterRead()
{
	if (!ThoughtAfterRead.IsEmpty())
	{
		USubject14ThoughtOverlayWidget::ShowThoughtLine(
			this,
			ThoughtAfterRead,
			NoteDisplaySeconds * 0.65f,
			NoteFadeInSeconds,
			NoteFadeOutSeconds);
	}
}

void ASubject14NoteActor::Subject14Interact_Implementation(AActor* Instigator)
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now - LastInteractGameTime < InteractionCooldownSeconds)
	{
		return;
	}

	if (bReadOnce && bHasBeenRead)
	{
		return;
	}

	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (Subsystem && !EvaluateStoryGate(*Subsystem))
	{
		if (!ThoughtWhenGateBlocked.IsEmpty())
		{
			USubject14ThoughtOverlayWidget::ShowThoughtLine(
				this,
				ThoughtWhenGateBlocked,
				NoteDisplaySeconds * 0.5f,
				NoteFadeInSeconds,
				NoteFadeOutSeconds);
		}
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Verbose, TEXT("%s: read blocked by story gate."), *GetName());
#endif
		return;
	}

	LastInteractGameTime = Now;
	if (bReadOnce)
	{
		bHasBeenRead = true;
	}

	UE_LOG(LogTemp, Log, TEXT("Subject14 Note [%s]: %s"), *NoteTitle, *NoteBody);

	const FString Combined = FString::Printf(TEXT("%s\n\n%s"), *NoteTitle, *NoteBody);
	USubject14ThoughtOverlayWidget::ShowThoughtLine(
		this,
		Combined,
		NoteDisplaySeconds,
		NoteFadeInSeconds,
		NoteFadeOutSeconds);

	if (Subsystem)
	{
		bool bFirstProgressionRead = !bOnlyFirstReadGrantsProgression;
		if (bOnlyFirstReadGrantsProgression)
		{
			if (bRegisterInStorySubsystem && !NoteId.IsNone())
			{
				bFirstProgressionRead = !Subsystem->HasReadNote(NoteId);
			}
			else
			{
				bFirstProgressionRead = !bProgressionEffectsConsumed;
			}
		}
		ApplyStoryProgression(*Subsystem, bFirstProgressionRead);
		if (bOnlyFirstReadGrantsProgression && !bRegisterInStorySubsystem && bFirstProgressionRead)
		{
			bProgressionEffectsConsumed = true;
		}
	}

	if (GEngine)
	{
		const uint64 KeyBase = (uint64)((PTRINT)this);
		GEngine->AddOnScreenDebugMessage(KeyBase, 5.0f, FColor(200, 200, 205), TEXT("(Note)"));
	}
}
