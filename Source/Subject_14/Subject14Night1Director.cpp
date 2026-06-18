#include "Subject14Night1Director.h"
#include "Subject14FirstPersonCharacter.h"
#include "Subject14NightEndPromptWidget.h"
#include "Subject14NoteActor.h"
#include "Subject14StorySubsystem.h"
#include "Subject14ThoughtOverlayWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14Night1Director)

namespace Subject14Night1MapUtil
{
static bool MapShortNameLikelyExistsOnDisk(const FString& ShortName)
{
	if (ShortName.IsEmpty())
	{
		return false;
	}
	const FString UnderMaps = FString::Printf(TEXT("/Game/Maps/%s"), *ShortName);
	const FString UnderGame = FString::Printf(TEXT("/Game/%s"), *ShortName);
	return FPackageName::DoesPackageExist(UnderMaps) || FPackageName::DoesPackageExist(UnderGame);
}

/** If Desired is missing on disk, prefer Lvl_Dev, then current level name (always reloadable). */
static FName ResolveOpenLevelName(UWorld* const World, const FName Desired)
{
	if (!World || Desired.IsNone())
	{
		return NAME_None;
	}

	const FString DesiredStr = Desired.ToString();
	if (MapShortNameLikelyExistsOnDisk(DesiredStr))
	{
		return Desired;
	}

	if (DesiredStr != TEXT("Lvl_Dev") && MapShortNameLikelyExistsOnDisk(TEXT("Lvl_Dev")))
	{
		return FName(TEXT("Lvl_Dev"));
	}

	FName Fallback = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	if (Fallback.IsNone())
	{
		Fallback = FName(TEXT("Lvl_Dev"));
	}
	return Fallback;
}
} // namespace Subject14Night1MapUtil

ASubject14Night1Director::ASubject14Night1Director()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	NightDurationSeconds = 95.0f;

	Schedule.Add(FSubject14Night1ScheduleEntry{8.0f, ENight1EventType::AmbientShift, 0});
	Schedule.Add(FSubject14Night1ScheduleEntry{22.0f, ENight1EventType::EnvironmentGlitch, 0});
	Schedule.Add(FSubject14Night1ScheduleEntry{36.0f, ENight1EventType::ThoughtText, 0});
	Schedule.Add(FSubject14Night1ScheduleEntry{50.0f, ENight1EventType::ThoughtText, 1});
	Schedule.Add(FSubject14Night1ScheduleEntry{62.0f, ENight1EventType::CreatureHint, 0});
	Schedule.Add(FSubject14Night1ScheduleEntry{74.0f, ENight1EventType::ThoughtText, 2});
	Schedule.Add(FSubject14Night1ScheduleEntry{86.0f, ENight1EventType::EndNight, 0});

	ThoughtLines.Add(TEXT("Something slipped—felt before I could name it."));
	ThoughtLines.Add(TEXT("That roll in the distance was too clean to be weather."));
	ThoughtLines.Add(TEXT("This place holds its breath like it was built for an audience."));

	static ConstructorHelpers::FObjectFinder<USoundWave> AmbAfter(
		TEXT("/Game/Audio/AmbientPressure.AmbientPressure"));
	if (AmbAfter.Succeeded())
	{
		AmbientSoundAfterShift = AmbAfter.Object;
		AmbAfter.Object->bLooping = true;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> GlitchSfx(
		TEXT("/Game/Audio/TapeHissBurst.TapeHissBurst"));
	if (GlitchSfx.Succeeded())
	{
		GlitchOneShotSound = GlitchSfx.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> HintSfx(
		TEXT("/Game/Audio/DistantMetalStress.DistantMetalStress"));
	if (HintSfx.Succeeded())
	{
		CreatureHintSound = HintSfx.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> StingSfx(TEXT("/Game/Audio/DreadHit.DreadHit"));
	if (StingSfx.Succeeded())
	{
		CreatureStingSound = StingSfx.Object;
	}
}

void ASubject14Night1Director::Night1DebugLogEvent(
	const TCHAR* const EventName,
	const float ScheduledFireTimeSeconds,
	const bool bUsedFallback,
	const TCHAR* const FallbackNote) const
{
#if !UE_BUILD_SHIPPING
	const UWorld* const World = GetWorld();
	const float GameT = World ? World->GetTimeSeconds() : -1.0f;
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Subject14 Night1 [%s] seqElapsed=%.2fs scheduled=%.2fs gameT=%.2fs fallback=%d %s"),
		EventName,
		NightElapsed,
		ScheduledFireTimeSeconds,
		GameT,
		bUsedFallback ? 1 : 0,
		FallbackNote);
#endif
}

void ASubject14Night1Director::ValidateNight1Setup() const
{
#if !UE_BUILD_SHIPPING
	if (ThoughtLines.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Subject14 Night1Director: ThoughtLines is empty."));
	}

	bool bHasEndNight = false;
	for (const FSubject14Night1ScheduleEntry& E : Schedule)
	{
		if (E.EventType == ENight1EventType::EndNight)
		{
			bHasEndNight = true;
			break;
		}
	}
	if (!bHasEndNight)
	{
		UE_LOG(LogTemp, Warning, TEXT("Subject14 Night1Director: Schedule has no EndNight event."));
	}

	if (Schedule.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Subject14 Night1Director: Schedule is empty."));
	}

	if (!Cast<ASubject14FirstPersonCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Subject14 Night1Director: Player pawn is not ASubject14FirstPersonCharacter (some events will no-op)."));
	}

	if (!CreatureHintAnchor)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Subject14 Night1Director: CreatureHintAnchor unset — creature hint uses relative placement fallback."));
	}

	if (bReturnToMenuAfterNightEnd && ReturnToMenuMapName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Subject14 Night1Director: bReturnToMenuAfterNightEnd is true but ReturnToMenuMapName is empty — will restart current level at end."));
	}

	if (NoteSuggestedAnchor)
	{
		bool bFoundNearbyNote = false;
		if (const UWorld* const W = GetWorld())
		{
			for (TActorIterator<ASubject14NoteActor> It(W); It; ++It)
			{
				if (It && FVector::Dist(It->GetActorLocation(), NoteSuggestedAnchor->GetActorLocation()) < 750.0f)
				{
					bFoundNearbyNote = true;
					break;
				}
			}
		}
		if (!bFoundNearbyNote)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Subject14 Night1Director: NoteSuggestedAnchor set but no Subject14NoteActor within ~750uu."));
		}
	}
#endif
}

void ASubject14Night1Director::RebuildActiveScheduleFromSchedule()
{
	ActiveSchedule = Schedule;
	for (FSubject14Night1ScheduleEntry& E : ActiveSchedule)
	{
		const float Jitter = FMath::Max(0.0f, E.TimeSecondsJitter);
		if (Jitter > KINDA_SMALL_NUMBER)
		{
			E.TimeSeconds += FMath::FRandRange(-Jitter, Jitter);
		}
		E.TimeSeconds = FMath::Max(0.02f, E.TimeSeconds);
	}
	SortActiveSchedule();
}

void ASubject14Night1Director::SortActiveSchedule()
{
	ActiveSchedule.Sort([](const FSubject14Night1ScheduleEntry& A, const FSubject14Night1ScheduleEntry& B)
	{
		return A.TimeSeconds < B.TimeSeconds;
	});
}

bool ASubject14Night1Director::ShouldRunNightTimeline()
{
	if (!bAutoStartFromStoryState)
	{
		return true;
	}

	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return true;
	}

	const ESubject14StoryPhase Phase = Subsystem->GetCurrentPhase();

	if (!bOnlyRunDuringNight1Phase)
	{
		if (Phase == ESubject14StoryPhase::IntroWake)
		{
			Subsystem->AdvanceToNextStoryBeat();
			Subsystem->SetStoryFlag(Subject14StoryFlags::Night1Started, true);
			Subsystem->SaveProgressToSlot();
		}
		return true;
	}

	if (Subsystem->HasStoryFlag(Subject14StoryFlags::Night1Completed))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Log, TEXT("Subject14 Night1Director: skipping timeline (Night1Completed flag set)."));
#endif
		return false;
	}

	if (Phase == ESubject14StoryPhase::IntroWake)
	{
		Subsystem->AdvanceToNextStoryBeat();
		Subsystem->SetStoryFlag(Subject14StoryFlags::Night1Started, true);
		Subsystem->SetCurrentObjectiveLine(TEXT("Stay alert. Listen for what doesn't belong."));
		Subsystem->SaveProgressToSlot();
		return true;
	}

	if (Phase == ESubject14StoryPhase::Night1Active)
	{
		if (!Subsystem->HasStoryFlag(Subject14StoryFlags::Night1Started))
		{
			Subsystem->SetStoryFlag(Subject14StoryFlags::Night1Started, true);
			Subsystem->SaveProgressToSlot();
		}
		if (Subsystem->GetCurrentObjectiveLine().IsEmpty())
		{
			Subsystem->SetCurrentObjectiveLine(TEXT("Stay alert. Listen for what doesn't belong."));
		}
		return true;
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Subject14 Night1Director: skipping timeline (story phase %d is past Night 1)."),
		(int32)Phase);
#endif
	return false;
}

void ASubject14Night1Director::CommitNight1StoryProgress()
{
	if (!bCommitStoryProgressOnEndNight)
	{
		return;
	}

	USubject14StorySubsystem* const Subsystem = USubject14StorySubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}

	if (Subsystem->HasStoryFlag(Subject14StoryFlags::Night1Completed)
		&& (uint8)Subsystem->GetCurrentPhase() >= (uint8)ESubject14StoryPhase::Day2Investigation)
	{
		return;
	}

	Subsystem->SetStoryFlag(Subject14StoryFlags::Night1Completed, true);

	const ESubject14StoryPhase Phase = Subsystem->GetCurrentPhase();
	if (Phase == ESubject14StoryPhase::Night1Active)
	{
		Subsystem->AdvanceToNextStoryBeat();
		Subsystem->AdvanceToNextStoryBeat();
	}
	else if (Phase == ESubject14StoryPhase::Night1Complete)
	{
		Subsystem->AdvanceToNextStoryBeat();
	}
	else if ((uint8)Phase < (uint8)ESubject14StoryPhase::Day2Investigation)
	{
		Subsystem->SetPhase(ESubject14StoryPhase::Day2Investigation);
	}

	if (!Subsystem->HasStoryFlag(Subject14StoryFlags::Day2Started))
	{
		Subsystem->SetStoryFlag(Subject14StoryFlags::Day2Started, true);
	}

	Subsystem->SetCurrentObjectiveLine(TEXT("Check the cabin. Something here isn't natural."));
	Subsystem->SaveProgressToSlot();

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Subject14 Night1Director: story advanced to Day 2 investigation and saved."));
#endif
}

void ASubject14Night1Director::BeginPlay()
{
	Super::BeginPlay();

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (!ShouldRunNightTimeline())
	{
		bNightActive = false;
		bNightEnded = true;
		SetActorTickEnabled(false);
		return;
	}

	RebuildActiveScheduleFromSchedule();
	bNightActive = true;
	bNightEnded = false;
	bForcedEndFired = false;
	bCreatureHintConsumed = false;
	NightElapsed = FMath::Max(0.0f, DebugSequenceStartOffsetSeconds);
	NextScheduleIndex = 0;

	ValidateNight1Setup();

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Subject14 Night1Director: started (%d active entries, %.0fs night cap, timeScale=%.2f, startOffset=%.2f)"),
		ActiveSchedule.Num(),
		NightDurationSeconds,
		SequenceTimeScale,
		DebugSequenceStartOffsetSeconds);
#endif

	while (NextScheduleIndex < ActiveSchedule.Num() && NightElapsed >= ActiveSchedule[NextScheduleIndex].TimeSeconds)
	{
		FireEvent(ActiveSchedule[NextScheduleIndex], ActiveSchedule[NextScheduleIndex].TimeSeconds);
		NextScheduleIndex++;
	}
}

void ASubject14Night1Director::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const World = GetWorld())
	{
		ClearGlitchTimers();
		World->GetTimerManager().ClearTimer(AmbientShiftRelaxTimer);
		World->GetTimerManager().ClearTimer(EndCaptionTimer);
		World->GetTimerManager().ClearTimer(EndNightAmbientPauseTimer);
		World->GetTimerManager().ClearTimer(CreatureHintTimer);
		World->GetTimerManager().ClearTimer(EndAwaitPromptTimer);
		World->GetTimerManager().ClearTimer(DevAutoRestartTimer);
	}
	if (EndPromptWidget.IsValid())
	{
		EndPromptWidget->RemoveFromParent();
		EndPromptWidget.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void ASubject14Night1Director::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bNightActive || bNightEnded)
	{
		return;
	}

	if (const APlayerController* const PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (!PC->IsLocalPlayerController())
		{
			return;
		}
	}

	NightElapsed += DeltaSeconds * FMath::Max(0.05f, SequenceTimeScale);
	ProcessTimeline();

	if (!bNightEnded && NightElapsed >= NightDurationSeconds + 5.0f && !bForcedEndFired)
	{
		bForcedEndFired = true;
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Subject14 Night1Director: forced EndNight (timeline past NightDurationSeconds)"));
#endif
		Event_EndNight();
	}
}

void ASubject14Night1Director::ProcessTimeline()
{
	while (NextScheduleIndex < ActiveSchedule.Num() && NightElapsed >= ActiveSchedule[NextScheduleIndex].TimeSeconds)
	{
		const float T = ActiveSchedule[NextScheduleIndex].TimeSeconds;
		FireEvent(ActiveSchedule[NextScheduleIndex], T);
		NextScheduleIndex++;
	}
}

void ASubject14Night1Director::FireEvent(const FSubject14Night1ScheduleEntry& Entry, const float SourceTimeSeconds)
{
	const TCHAR* EventLabel = TEXT("Unknown");
	switch (Entry.EventType)
	{
	case ENight1EventType::AmbientShift:
		EventLabel = TEXT("AmbientShift");
		break;
	case ENight1EventType::EnvironmentGlitch:
		EventLabel = TEXT("EnvironmentGlitch");
		break;
	case ENight1EventType::ThoughtText:
		EventLabel = TEXT("ThoughtText");
		break;
	case ENight1EventType::CreatureHint:
		EventLabel = TEXT("CreatureHint");
		break;
	case ENight1EventType::EndNight:
		EventLabel = TEXT("EndNight");
		break;
	default:
		break;
	}

	Night1DebugLogEvent(EventLabel, SourceTimeSeconds, false, TEXT(""));

	switch (Entry.EventType)
	{
	case ENight1EventType::AmbientShift:
		Event_AmbientShift();
		break;
	case ENight1EventType::EnvironmentGlitch:
		Event_EnvironmentGlitch();
		break;
	case ENight1EventType::ThoughtText:
		Event_ThoughtText(Entry.ThoughtLineIndex);
		break;
	case ENight1EventType::CreatureHint:
		Event_CreatureHint();
		break;
	case ENight1EventType::EndNight:
		Event_EndNight();
		break;
	default:
		break;
	}
}

void ASubject14Night1Director::DebugSkipToNextScheduleEvent()
{
#if !UE_BUILD_SHIPPING
	if (!bNightActive || bNightEnded)
	{
		return;
	}
	if (NextScheduleIndex >= ActiveSchedule.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Subject14 Night1Director: DebugSkip — no next schedule entry."));
		return;
	}

	const float T = ActiveSchedule[NextScheduleIndex].TimeSeconds;
	NightElapsed = T + 0.02f;
	UE_LOG(LogTemp, Warning, TEXT("Subject14 Night1Director: DebugSkipToNextScheduleEvent -> seqT=%.2fs"), NightElapsed);
	ProcessTimeline();
#endif
}

void ASubject14Night1Director::Event_AmbientShift()
{
	ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
		UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Char)
	{
#if !UE_BUILD_SHIPPING
		Night1DebugLogEvent(TEXT("AmbientShift"), NightElapsed, true, TEXT("(no Subject14 pawn)"));
#endif
		return;
	}

	if (AmbientSoundAfterShift)
	{
		Char->SetAmbientBedSoundAndRestart(AmbientSoundAfterShift);
		return;
	}

	Char->SetAmbientBedVolumeScalar(0.55f);
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AmbientShiftRelaxTimer);
		World->GetTimerManager().SetTimer(
			AmbientShiftRelaxTimer,
			this,
			&ASubject14Night1Director::AmbientShiftRelax,
			6.0f,
			false);
#if !UE_BUILD_SHIPPING
		Night1DebugLogEvent(TEXT("AmbientShift"), NightElapsed, true, TEXT("(volume dip, no swap asset)"));
#endif
	}
}

void ASubject14Night1Director::AmbientShiftRelax()
{
	if (ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
			UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Char->SetAmbientBedVolumeScalar(1.0f);
	}
}

void ASubject14Night1Director::ClearGlitchTimers()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GlitchBuildupTimer);
		World->GetTimerManager().ClearTimer(GlitchPeakTimer);
		World->GetTimerManager().ClearTimer(GlitchRecoverStageTimer);
		World->GetTimerManager().ClearTimer(GlitchRecoverTimer);
	}
}

void ASubject14Night1Director::GlitchBeginBuildup()
{
	ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
		UGameplayStatics::GetPlayerPawn(this, 0));
	if (Char)
	{
		Char->SetAmbientBedPaused(false);
		Char->SetAmbientBedVolumeScalar(GlitchBuildupAmbientScalar);
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		GlitchEnterPeak();
		return;
	}

	if (GlitchBuildupSeconds <= KINDA_SMALL_NUMBER)
	{
		GlitchEnterPeak();
		return;
	}

	World->GetTimerManager().SetTimer(
		GlitchBuildupTimer,
		this,
		&ASubject14Night1Director::GlitchEnterPeak,
		GlitchBuildupSeconds,
		false);
}

void ASubject14Night1Director::GlitchEnterPeak()
{
	ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
		UGameplayStatics::GetPlayerPawn(this, 0));
	if (Char)
	{
		Char->SetAmbientBedPaused(true);
		Char->SetAmbientBedVolumeScalar(GlitchPeakAmbientScalar);
		Char->RunFlashlightFlickerRoutine(GlitchPeakFlickerCount, GlitchPeakFlickerStepSeconds);
	}

	if (GlitchOneShotSound)
	{
		const float PeakPitch = FMath::FRandRange(0.94f, 1.04f);
		if (GlitchFocusAnchor)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				GlitchOneShotSound,
				GlitchFocusAnchor->GetActorLocation(),
				0.55f,
				PeakPitch);
#if !UE_BUILD_SHIPPING
			Night1DebugLogEvent(TEXT("GlitchPeak"), NightElapsed, false, TEXT("(3D at GlitchFocusAnchor)"));
#endif
		}
		else
		{
			UGameplayStatics::PlaySound2D(this, GlitchOneShotSound, 0.55f, PeakPitch);
#if !UE_BUILD_SHIPPING
			Night1DebugLogEvent(TEXT("GlitchPeak"), NightElapsed, true, TEXT("(2D glitch SFX, no anchor)"));
#endif
		}
	}

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GlitchPeakTimer);
		World->GetTimerManager().SetTimer(
			GlitchPeakTimer,
			this,
			&ASubject14Night1Director::GlitchLeavePeakStartRecover,
			GlitchPeakHoldSeconds,
			false);
	}
}

void ASubject14Night1Director::GlitchLeavePeakStartRecover()
{
	ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
		UGameplayStatics::GetPlayerPawn(this, 0));
	if (Char)
	{
		Char->SetAmbientBedPaused(false);
		Char->SetAmbientBedVolumeScalar(GlitchRecoverMidAmbientScalar);
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		GlitchRecoverFinal();
		return;
	}

	const float Stage1 = FMath::Clamp(GlitchRecoverSeconds * GlitchRecoverStageRatio, 0.02f, GlitchRecoverSeconds);
	World->GetTimerManager().ClearTimer(GlitchRecoverStageTimer);
	World->GetTimerManager().SetTimer(
		GlitchRecoverStageTimer,
		this,
		&ASubject14Night1Director::GlitchRecoverMidStage,
		Stage1,
		false);
}

void ASubject14Night1Director::GlitchRecoverMidStage()
{
	if (ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
			UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		const float Mid = GlitchRecoverMidAmbientScalar;
		Char->SetAmbientBedVolumeScalar(FMath::Lerp(Mid, 1.0f, 0.55f));
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		GlitchRecoverFinal();
		return;
	}

	const float Stage1 = FMath::Clamp(GlitchRecoverSeconds * GlitchRecoverStageRatio, 0.02f, GlitchRecoverSeconds);
	const float Stage2 = FMath::Max(0.02f, GlitchRecoverSeconds - Stage1);
	World->GetTimerManager().ClearTimer(GlitchRecoverTimer);
	World->GetTimerManager().SetTimer(
		GlitchRecoverTimer,
		this,
		&ASubject14Night1Director::GlitchRecoverFinal,
		Stage2,
		false);
}

void ASubject14Night1Director::GlitchRecoverFinal()
{
	if (ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
			UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Char->SetAmbientBedPaused(false);
		Char->SetAmbientBedVolumeScalar(1.0f);
	}
}

void ASubject14Night1Director::Event_EnvironmentGlitch()
{
	ClearGlitchTimers();

	if (!Cast<ASubject14FirstPersonCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
#if !UE_BUILD_SHIPPING
		Night1DebugLogEvent(
			TEXT("EnvironmentGlitch"),
			NightElapsed,
			true,
			TEXT("(no Subject14 pawn — ambient/flicker skipped where applicable)"));
#endif
	}

	GlitchBeginBuildup();
}

void ASubject14Night1Director::Event_ThoughtText(const int32 LineIndex)
{
	if (!ThoughtLines.IsValidIndex(LineIndex))
	{
#if !UE_BUILD_SHIPPING
		Night1DebugLogEvent(TEXT("ThoughtText"), NightElapsed, true, TEXT("(invalid ThoughtLines index)"));
#endif
		return;
	}

	USubject14ThoughtOverlayWidget::ShowThoughtLine(
		this,
		ThoughtLines[LineIndex],
		ThoughtDisplaySeconds,
		ThoughtFadeInSeconds,
		ThoughtFadeOutSeconds);
}

void ASubject14Night1Director::ScheduleCreatureHintPlayback()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CreatureHintTimer);
		World->GetTimerManager().SetTimer(
			CreatureHintTimer,
			this,
			&ASubject14Night1Director::PlayCreatureHintAudioAndEffects,
			FMath::Max(0.0f, CreaturePreHintSilenceSeconds),
			false);
	}
	else
	{
		PlayCreatureHintAudioAndEffects();
	}
}

void ASubject14Night1Director::Event_CreatureHint()
{
	if (bCreatureHintOnceOnly && bCreatureHintConsumed)
	{
#if !UE_BUILD_SHIPPING
		Night1DebugLogEvent(TEXT("CreatureHint"), NightElapsed, true, TEXT("(skipped, already consumed)"));
#endif
		return;
	}

	APawn* const Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
#if !UE_BUILD_SHIPPING
		Night1DebugLogEvent(TEXT("CreatureHint"), NightElapsed, true, TEXT("(no pawn)"));
#endif
		return;
	}

	const FVector AnchorLoc = CreatureHintAnchor ? CreatureHintAnchor->GetActorLocation() : FVector::ZeroVector;
	if (CreatureHintAnchor)
	{
		if (CreatureHintMaxPlayerDistanceFromAnchor > KINDA_SMALL_NUMBER)
		{
			const float D = FVector::Dist(Pawn->GetActorLocation(), AnchorLoc);
			if (D > CreatureHintMaxPlayerDistanceFromAnchor)
			{
#if !UE_BUILD_SHIPPING
				Night1DebugLogEvent(
					TEXT("CreatureHint"),
					NightElapsed,
					true,
					TEXT("(player beyond CreatureHintMaxPlayerDistanceFromAnchor)"));
#endif
				if (bCreatureHintOnceOnly)
				{
					bCreatureHintConsumed = true;
				}
				return;
			}
		}

		if (CreatureHintFacingConeHalfAngleDegrees > KINDA_SMALL_NUMBER)
		{
			const FVector ToAnchor = (AnchorLoc - Pawn->GetActorLocation()).GetSafeNormal2D();
			const FVector Fwd = Pawn->GetActorForwardVector().GetSafeNormal2D();
			const float CosNeed = FMath::Cos(FMath::DegreesToRadians(CreatureHintFacingConeHalfAngleDegrees));
			if (ToAnchor.IsNearlyZero() || FVector::DotProduct(Fwd, ToAnchor) < CosNeed)
			{
#if !UE_BUILD_SHIPPING
				Night1DebugLogEvent(TEXT("CreatureHint"), NightElapsed, true, TEXT("(facing cone check failed)"));
#endif
				if (bCreatureHintOnceOnly)
				{
					bCreatureHintConsumed = true;
				}
				return;
			}
		}
	}

	if (bCreatureHintOnceOnly)
	{
		bCreatureHintConsumed = true;
	}

	ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(Pawn);
	if (CreaturePreHintSilenceSeconds > KINDA_SMALL_NUMBER && Char)
	{
		Char->SetAmbientBedVolumeScalar(CreaturePreHintAmbientScalar);
		ScheduleCreatureHintPlayback();
		return;
	}

	PlayCreatureHintAudioAndEffects();
}

void ASubject14Night1Director::PlayCreatureHintAudioAndEffects()
{
	if (ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
			UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Char->SetAmbientBedVolumeScalar(1.0f);
	}

	APawn* const Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}

	bool bUsedAnchor = false;
	FVector EmitLocation = FVector::ZeroVector;
	if (CreatureHintAnchor)
	{
		EmitLocation = CreatureHintAnchor->GetActorLocation();
		bUsedAnchor = true;
	}
	else
	{
		EmitLocation = Pawn->GetActorLocation() - Pawn->GetActorForwardVector() * CreatureHintDistance
			+ FVector(0.0f, 0.0f, 90.0f);
	}

	if (CreatureHintSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			CreatureHintSound,
			EmitLocation,
			CreatureHintVolume,
			FMath::FRandRange(0.92f, 1.05f));
	}

	if (CreatureStingSound)
	{
		UGameplayStatics::PlaySound2D(this, CreatureStingSound, CreatureStingVolume);
	}

	if (ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(Pawn))
	{
		Char->RunFlashlightFlickerRoutine(CreatureHintFlashlightFlickerCount, CreatureHintFlashlightFlickerStepSeconds);
	}

#if !UE_BUILD_SHIPPING
	Night1DebugLogEvent(
		TEXT("CreatureHintAudio"),
		NightElapsed,
		!bUsedAnchor,
		bUsedAnchor ? TEXT("(3D at anchor)") : TEXT("(relative behind-player emit)"));
#endif
}

void ASubject14Night1Director::ShowEndNightCaption()
{
	EndNightSequenceLog(TEXT("end caption overlay started"));
	USubject14ThoughtOverlayWidget::ShowThoughtLine(
		this,
		EndNightMessage,
		EndNightCaptionHoldSeconds,
		ThoughtFadeInSeconds,
		ThoughtFadeOutSeconds);
}

void ASubject14Night1Director::PauseAmbientAfterEndFade()
{
	if (ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
			UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (bEndNightPauseAmbientAfterFade)
		{
			Char->SetAmbientBedPaused(true);
		}
		Char->SetAmbientBedVolumeScalar(0.0f);
	}
}

void ASubject14Night1Director::Event_EndNight()
{
	if (bNightEnded)
	{
		return;
	}

	CommitNight1StoryProgress();

	bNightEnded = true;
	bNightActive = false;
	SetActorTickEnabled(false);
	bEndReturnTriggered = false;

	EndNightSequenceLog(TEXT("entered end fade (pawn night-lock, controller input off, camera fade)"));

#if !UE_BUILD_SHIPPING
	if (EndNightFocusAnchor)
	{
		UE_LOG(LogTemp, Log, TEXT("Subject14 Night1Director: EndNightFocusAnchor at %s"), *EndNightFocusAnchor->GetActorLocation().ToString());
	}
#endif

	APlayerController* const PC = UGameplayStatics::GetPlayerController(this, 0);
	if (ASubject14FirstPersonCharacter* const Char = Cast<ASubject14FirstPersonCharacter>(
			UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Char->SetNightSequenceInputBlocked(true);
		Char->SetAmbientBedVolumeScalar(EndNightAmbientDuckScalar);
	}

	if (PC)
	{
		PC->DisableInput(PC);
	}

	if (APlayerCameraManager* const PCM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		PCM->StartCameraFade(0.0f, 1.0f, EndNightFadeSeconds, FLinearColor::Black, true, true);
	}

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EndNightAmbientPauseTimer);
		World->GetTimerManager().SetTimer(
			EndNightAmbientPauseTimer,
			this,
			&ASubject14Night1Director::PauseAmbientAfterEndFade,
			EndNightFadeSeconds,
			false);

		World->GetTimerManager().ClearTimer(EndCaptionTimer);
		World->GetTimerManager().SetTimer(
			EndCaptionTimer,
			this,
			&ASubject14Night1Director::ShowEndNightCaption,
			EndNightCaptionDelaySeconds,
			false);

		ScheduleEndNightReturnPhase();
	}
}

void ASubject14Night1Director::EndNightSequenceLog(const TCHAR* const Message) const
{
#if !UE_BUILD_SHIPPING
	if (!bLogEndStateTransitions)
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("Subject14 Night1 EndPhase: %s"), Message);
#else
	(void)Message;
#endif
}

void ASubject14Night1Director::ScheduleEndNightReturnPhase()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EndAwaitPromptTimer);
	World->GetTimerManager().ClearTimer(DevAutoRestartTimer);

	const float CaptionLifetime = ThoughtFadeInSeconds + EndNightCaptionHoldSeconds + ThoughtFadeOutSeconds;
	const float PromptDelay = FMath::Max(0.0f, EndNightCaptionDelaySeconds + CaptionLifetime + DelayBeforeEndPromptSeconds);

#if !UE_BUILD_SHIPPING
	if (bLogEndStateTransitions)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Subject14 Night1 EndPhase: scheduled post-caption return phase in %.1fs (caption delay %.2fs + caption ~%.2fs + extra %.2fs)"),
			PromptDelay,
			EndNightCaptionDelaySeconds,
			CaptionLifetime,
			DelayBeforeEndPromptSeconds);
	}
#endif

#if !UE_BUILD_SHIPPING
	const bool bDevRestart = bDevAutoRestartInstead;
#else
	const bool bDevRestart = false;
#endif

	if (bDevRestart)
	{
		EndNightSequenceLog(TEXT("dev auto-restart armed (skips prompt; races caption if delay is short)"));
		World->GetTimerManager().SetTimer(
			DevAutoRestartTimer,
			this,
			&ASubject14Night1Director::OnDevAutoRestartFire,
			FMath::Max(0.05f, DevRestartDelaySeconds),
			false);
		return;
	}

	World->GetTimerManager().SetTimer(
		EndAwaitPromptTimer,
		this,
		&ASubject14Night1Director::BeginEndPromptPhase,
		PromptDelay,
		false);
}

void ASubject14Night1Director::OnDevAutoRestartFire()
{
#if !UE_BUILD_SHIPPING
	EndNightSequenceLog(TEXT("dev auto-restart timer fired"));
	ExecuteNightEndReturn();
#endif
}

void ASubject14Night1Director::BeginEndPromptPhase()
{
	EndNightSequenceLog(TEXT("end caption timeline complete (fade+hold+fadeout window elapsed)"));

	if (bEndReturnTriggered)
	{
		return;
	}

	EndNightSequenceLog(TEXT("entered post-caption return phase"));

	if (!bShowEndPromptAfterCaption)
	{
		EndNightSequenceLog(TEXT("end prompt disabled — immediate return/restart"));
		ExecuteNightEndReturn();
		return;
	}

	APlayerController* const PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->EnableInput(PC);
	}

	USubject14NightEndPromptWidget* const Prompt = USubject14NightEndPromptWidget::ShowPrompt(
		this,
		this,
		EndPromptText,
		EndPromptFadeInSeconds,
		bAllowAnyKeyReturn);
	EndPromptWidget = Prompt;

	if (Prompt)
	{
		EndNightSequenceLog(TEXT("end prompt shown (awaiting confirmation)"));
	}
	else
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Subject14 Night1Director: end prompt widget failed to create — restarting level."));
#endif
		ExecuteNightEndReturn();
	}
}

void ASubject14Night1Director::HandleEndNightPromptCommitted()
{
	if (bEndReturnTriggered)
	{
		return;
	}

	EndNightSequenceLog(TEXT("return action triggered (prompt)"));

	if (EndPromptWidget.IsValid())
	{
		EndPromptWidget->RemoveFromParent();
		EndPromptWidget.Reset();
	}

	ExecuteNightEndReturn();
}

void ASubject14Night1Director::ExecuteNightEndReturn()
{
	if (bEndReturnTriggered)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	bEndReturnTriggered = true;

	if (EndPromptWidget.IsValid())
	{
		EndPromptWidget->RemoveFromParent();
		EndPromptWidget.Reset();
	}

	const bool bHasMenuMap = !ReturnToMenuMapName.IsNone();
	const bool bOpenMenu = bReturnToMenuAfterNightEnd && bHasMenuMap;

	if (bReturnToMenuAfterNightEnd && !bHasMenuMap)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Subject14 Night1Director: bReturnToMenuAfterNightEnd but ReturnToMenuMapName is empty — falling back to level restart."));
#endif
	}

	if (bOpenMenu)
	{
		const FName Resolved = Subject14Night1MapUtil::ResolveOpenLevelName(World, ReturnToMenuMapName);
#if !UE_BUILD_SHIPPING
		if (Resolved != ReturnToMenuMapName)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Subject14 Night1Director: map '%s' not found on disk — opening '%s' instead. Run subject14_create_main_menu_map.py or set ReturnToMenuMapName to an existing map (e.g. Lvl_Dev)."),
				*ReturnToMenuMapName.ToString(),
				*Resolved.ToString());
		}
		else if (bLogEndStateTransitions)
		{
			UE_LOG(LogTemp, Log, TEXT("Subject14 Night1 EndPhase: opening map: %s"), *Resolved.ToString());
		}
#endif
		UGameplayStatics::OpenLevel(World, Resolved);
		return;
	}

	const FString ShortName = UGameplayStatics::GetCurrentLevelName(World, true);
#if !UE_BUILD_SHIPPING
	if (bLogEndStateTransitions)
	{
		UE_LOG(LogTemp, Log, TEXT("Subject14 Night1 EndPhase: restarting / reopening current level: %s"), *ShortName);
	}
#endif
	UGameplayStatics::OpenLevel(World, FName(*ShortName));
}
