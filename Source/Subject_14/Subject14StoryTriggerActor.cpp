#include "Subject14StoryTriggerActor.h"

#include "Subject14StorySubsystem.h"
#include "Subject14ThoughtOverlayWidget.h"

#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14StoryTriggerActor)

ASubject14StoryTriggerActor::ASubject14StoryTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(128.0f, 128.0f, 128.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	// Allow the interact trace to land even if bFireOnInteract is later toggled on.
	TriggerBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TriggerBox->SetGenerateOverlapEvents(true);

#if WITH_EDITORONLY_DATA
	EditorIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	if (EditorIcon)
	{
		EditorIcon->SetupAttachment(TriggerBox);
		EditorIcon->bHiddenInGame = true;
		EditorIcon->bIsEditorOnly = true;
	}
#endif
}

void ASubject14StoryTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.RemoveDynamic(this, &ASubject14StoryTriggerActor::HandleBeginOverlap);
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ASubject14StoryTriggerActor::HandleBeginOverlap);
	}

#if !UE_BUILD_SHIPPING
	if (!bFireOnOverlap && !bFireOnInteract)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s: both bFireOnOverlap and bFireOnInteract are false — trigger will never fire"),
			*GetName());
	}
	if (GrantedStoryFlag.IsNone() && ClearedStoryFlag.IsNone()
		&& !bAdvancePhaseOnFire && ThoughtLine.IsEmpty() && ObjectiveLine.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s: no effect configured (no flag set, no phase advance, no thought/objective)"),
			*GetName());
	}
#endif
}

void ASubject14StoryTriggerActor::Subject14Interact_Implementation(AActor* const Instigator)
{
	if (!bFireOnInteract)
	{
		return;
	}

	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: interact fired but story subsystem is unavailable"), *GetName());
		return;
	}
	if (!EvaluateGating(*Subsystem) || EvaluateOneShotBlock(*Subsystem))
	{
		return;
	}
	FireNow(Instigator);
}

void ASubject14StoryTriggerActor::HandleBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* const OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!bFireOnOverlap || !OtherActor)
	{
		return;
	}

	APawn* const OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn)
	{
		return;
	}
	if (bOnlyLocalPlayerOverlap)
	{
		const APlayerController* const PC = Cast<APlayerController>(OtherPawn->GetController());
		if (!PC || !PC->IsLocalPlayerController())
		{
			return;
		}
	}

	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}
	if (!EvaluateGating(*Subsystem) || EvaluateOneShotBlock(*Subsystem))
	{
		return;
	}
	FireNow(OtherActor);
}

bool ASubject14StoryTriggerActor::EvaluateGating(const USubject14StorySubsystem& Subsystem) const
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

FName ASubject14StoryTriggerActor::ResolveOneShotFlagName() const
{
	if (!GrantedStoryFlag.IsNone())
	{
		return GrantedStoryFlag;
	}
	// Fall back to actor FName — stable across saves, unique per placement.
	return GetFName();
}

bool ASubject14StoryTriggerActor::ShouldAllowStoryPhaseAdvanceFromInstigator(AActor* const Instigator) const
{
	UWorld* const World = GetWorld();
	if (!World || World->GetNumPlayerControllers() <= 1)
	{
		return true;
	}
	const APlayerController* const PC0 = UGameplayStatics::GetPlayerController(World, 0);
	const APawn* const Pawn0 = PC0 ? PC0->GetPawn() : nullptr;
	return Instigator != nullptr && Cast<APawn>(Instigator) == Pawn0;
}

bool ASubject14StoryTriggerActor::EvaluateOneShotBlock(const USubject14StorySubsystem& Subsystem) const
{
	if (!bOneShot)
	{
		return false;
	}
	if (bHasFiredThisSession)
	{
		return true;
	}
	if (bOneShotPersistent)
	{
		const FName BlockerFlag = ResolveOneShotFlagName();
		if (!BlockerFlag.IsNone() && Subsystem.HasStoryFlag(BlockerFlag))
		{
			return true;
		}
	}
	return false;
}

void ASubject14StoryTriggerActor::FireNow(AActor* const Instigator)
{
	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!ensure(Subsystem))
	{
		return;
	}

	bHasFiredThisSession = true;

	if (!GrantedStoryFlag.IsNone())
	{
		Subsystem->SetStoryFlag(GrantedStoryFlag, true);
	}
	if (!ClearedStoryFlag.IsNone())
	{
		Subsystem->SetStoryFlag(ClearedStoryFlag, false);
	}

	// Persistent one-shot marker (only needed if we didn't already grant one).
	if (bOneShot && bOneShotPersistent && GrantedStoryFlag.IsNone())
	{
		const FName Blocker = ResolveOneShotFlagName();
		if (!Blocker.IsNone())
		{
			Subsystem->SetStoryFlag(Blocker, true);
		}
	}

	if (bAdvancePhaseOnFire)
	{
		if (ShouldAllowStoryPhaseAdvanceFromInstigator(Instigator))
		{
			Subsystem->AdvanceToNextStoryBeat();
		}
#if !UE_BUILD_SHIPPING
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("%s: skipped AdvanceToNextStoryBeat (multi-local guard; instigator=%s)"),
				*GetName(),
				Instigator ? *Instigator->GetName() : TEXT("<none>"));
		}
#endif
	}

	if (!ObjectiveLine.IsEmpty())
	{
		Subsystem->SetCurrentObjectiveLine(ObjectiveLine);
	}

	if (!ThoughtLine.IsEmpty())
	{
		USubject14ThoughtOverlayWidget::ShowThoughtLine(
			this,
			ThoughtLine,
			ThoughtHoldSeconds,
			ThoughtFadeInSeconds,
			ThoughtFadeOutSeconds);
	}

	UE_LOG(LogTemp, Log,
		TEXT("%s fired by %s (GrantedFlag=%s Cleared=%s Advance=%d)"),
		*GetName(),
		Instigator ? *Instigator->GetName() : TEXT("<none>"),
		*GrantedStoryFlag.ToString(),
		*ClearedStoryFlag.ToString(),
		bAdvancePhaseOnFire ? 1 : 0);
}
