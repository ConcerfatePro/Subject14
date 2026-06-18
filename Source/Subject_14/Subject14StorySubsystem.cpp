#include "Subject14StorySubsystem.h"

#include "Subject14SaveGame.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AssertionMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14StorySubsystem)

namespace Subject14StoryFlags
{
const FName Night1Started = FName(TEXT("Night1Started"));
const FName Night1Completed = FName(TEXT("Night1Completed"));
const FName Day2Started = FName(TEXT("Day2Started"));
const FName HatchDiscovered = FName(TEXT("HatchDiscovered"));
const FName HatchHasPower = FName(TEXT("HatchHasPower"));
const FName HatchUnlocked = FName(TEXT("HatchUnlocked"));
const FName HatchOpened = FName(TEXT("HatchOpened"));
}

namespace Subject14StorySubsystemPrivate
{
static const TCHAR* PhaseToText(const ESubject14StoryPhase Phase)
{
	switch (Phase)
	{
	case ESubject14StoryPhase::IntroWake:         return TEXT("IntroWake");
	case ESubject14StoryPhase::Night1Active:      return TEXT("Night1Active");
	case ESubject14StoryPhase::Night1Complete:    return TEXT("Night1Complete");
	case ESubject14StoryPhase::Day2Investigation: return TEXT("Day2Investigation");
	case ESubject14StoryPhase::Day3BreachPrep:    return TEXT("Day3BreachPrep");
	case ESubject14StoryPhase::HatchUnlocked:     return TEXT("HatchUnlocked");
	case ESubject14StoryPhase::FirstBreach:       return TEXT("FirstBreach");
	}
	return TEXT("<unknown>");
}
}

// -----------------------------------------------------------------------------
// UGameInstanceSubsystem
// -----------------------------------------------------------------------------

void USubject14StorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Construction default: a fresh run. The main menu / first level can call
	// StartNewGame() for an explicit reset; LoadProgressFromSlot() replaces this.
	CurrentStoryDay = 1;
	CurrentStoryNight = 0;
	CurrentPhase = ESubject14StoryPhase::IntroWake;
	StoryFlags.Reset();
	ReadNotes.Reset();
	SyncDayNightToPhase();

	UE_LOG(LogTemp, Log,
		TEXT("USubject14StorySubsystem initialized (Day=%d Night=%d Phase=%s)"),
		CurrentStoryDay,
		CurrentStoryNight,
		Subject14StorySubsystemPrivate::PhaseToText(CurrentPhase));

	// --- Console commands ---
	//
	// We resolve the subsystem at invocation time via the provided UWorld so we
	// never cache a dangling `this` between PIE sessions or engine cycles.

	auto ResolveFromWorld = [](UWorld* const World) -> USubject14StorySubsystem*
	{
		if (!World)
		{
			return nullptr;
		}
		const UGameInstance* const GI = World->GetGameInstance();
		return GI ? GI->GetSubsystem<USubject14StorySubsystem>() : nullptr;
	};

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("Subject14.DumpStory"),
		TEXT("Dump the Subject 14 story subsystem state to the log and screen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[ResolveFromWorld](const TArray<FString>& /*Args*/, UWorld* const World)
			{
				if (USubject14StorySubsystem* const S = ResolveFromWorld(World))
				{
					S->DumpStoryToLog();
				}
			})));

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("Subject14.SetStoryPhase"),
		TEXT("Subject14.SetStoryPhase <0..6> — force a phase index."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[ResolveFromWorld](const TArray<FString>& Args, UWorld* const World)
			{
				if (Args.Num() < 1)
				{
					UE_LOG(LogTemp, Warning, TEXT("Subject14.SetStoryPhase: expected 1 argument"));
					return;
				}
				USubject14StorySubsystem* const S = ResolveFromWorld(World);
				if (!S)
				{
					return;
				}
				const int32 Max = (int32)ESubject14StoryPhase::FirstBreach;
				const int32 Index = FCString::Atoi(*Args[0]);
				if (Index < 0 || Index > Max)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("Subject14.SetStoryPhase: %d out of range [0..%d]"), Index, Max);
					return;
				}
				S->SetPhase((ESubject14StoryPhase)Index);
			})));

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("Subject14.SetStoryFlag"),
		TEXT("Subject14.SetStoryFlag <Flag> <0|1>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[ResolveFromWorld](const TArray<FString>& Args, UWorld* const World)
			{
				if (Args.Num() < 2)
				{
					UE_LOG(LogTemp, Warning, TEXT("Subject14.SetStoryFlag: expected <Flag> <0|1>"));
					return;
				}
				USubject14StorySubsystem* const S = ResolveFromWorld(World);
				if (!S)
				{
					return;
				}
				S->SetStoryFlag(FName(*Args[0]), FCString::Atoi(*Args[1]) != 0);
			})));

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("Subject14.ClearStory"),
		TEXT("Reset the in-memory story state (does not delete the save slot)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[ResolveFromWorld](const TArray<FString>& /*Args*/, UWorld* const World)
			{
				if (USubject14StorySubsystem* const S = ResolveFromWorld(World))
				{
					S->StartNewGame();
				}
			})));

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("Subject14.AdvanceStory"),
		TEXT("Call AdvanceToNextStoryBeat() once."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[ResolveFromWorld](const TArray<FString>& /*Args*/, UWorld* const World)
			{
				if (USubject14StorySubsystem* const S = ResolveFromWorld(World))
				{
					S->AdvanceToNextStoryBeat();
				}
			})));

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("Subject14.SaveStory"),
		TEXT("Write the story subsystem state to its configured save slot."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[ResolveFromWorld](const TArray<FString>& /*Args*/, UWorld* const World)
			{
				if (USubject14StorySubsystem* const S = ResolveFromWorld(World))
				{
					S->SaveProgressToSlot();
				}
			})));

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("Subject14.LoadStory"),
		TEXT("Load the story subsystem state from its configured save slot."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[ResolveFromWorld](const TArray<FString>& /*Args*/, UWorld* const World)
			{
				if (USubject14StorySubsystem* const S = ResolveFromWorld(World))
				{
					S->LoadProgressFromSlot();
				}
			})));

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("Subject14.DeleteStory"),
		TEXT("Delete the configured save slot."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[ResolveFromWorld](const TArray<FString>& /*Args*/, UWorld* const World)
			{
				if (USubject14StorySubsystem* const S = ResolveFromWorld(World))
				{
					S->DeleteProgressSlot();
				}
			})));
}

void USubject14StorySubsystem::Deinitialize()
{
	// Console commands auto-unregister via FAutoConsoleCommand's destructor.
	ConsoleCommands.Reset();

	OnStoryPhaseChanged.Clear();
	OnStoryFlagChanged.Clear();
	OnNoteRead.Clear();

	Super::Deinitialize();
}

USubject14StorySubsystem* USubject14StorySubsystem::Get(const UObject* const WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	UGameInstance* const GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GI ? GI->GetSubsystem<USubject14StorySubsystem>() : nullptr;
}

// -----------------------------------------------------------------------------
// Run lifecycle
// -----------------------------------------------------------------------------

void USubject14StorySubsystem::StartNewGame()
{
	const ESubject14StoryPhase OldPhase = CurrentPhase;

	CurrentStoryDay = 1;
	CurrentStoryNight = 0;
	CurrentPhase = ESubject14StoryPhase::IntroWake;
	StoryFlags.Reset();
	ReadNotes.Reset();
	SyncDayNightToPhase();

	UE_LOG(LogTemp, Log, TEXT("USubject14StorySubsystem::StartNewGame — state cleared."));

	InternalBroadcastPhaseChange(OldPhase);
}

bool USubject14StorySubsystem::AdvanceToNextStoryBeat()
{
	const ESubject14StoryPhase OldPhase = CurrentPhase;
	const ESubject14StoryPhase NextPhase = GetNextPhase(OldPhase);
	if (NextPhase == OldPhase)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("USubject14StorySubsystem::AdvanceToNextStoryBeat — already at terminal phase %s"),
			Subject14StorySubsystemPrivate::PhaseToText(OldPhase));
		return false;
	}

	CurrentPhase = NextPhase;
	SyncDayNightToPhase();

	UE_LOG(LogTemp, Log,
		TEXT("USubject14StorySubsystem::AdvanceToNextStoryBeat — %s -> %s (Day=%d Night=%d)"),
		Subject14StorySubsystemPrivate::PhaseToText(OldPhase),
		Subject14StorySubsystemPrivate::PhaseToText(NextPhase),
		CurrentStoryDay,
		CurrentStoryNight);

	InternalBroadcastPhaseChange(OldPhase);
	return true;
}

ESubject14StoryPhase USubject14StorySubsystem::GetNextPhase(const ESubject14StoryPhase Phase)
{
	switch (Phase)
	{
	case ESubject14StoryPhase::IntroWake:         return ESubject14StoryPhase::Night1Active;
	case ESubject14StoryPhase::Night1Active:      return ESubject14StoryPhase::Night1Complete;
	case ESubject14StoryPhase::Night1Complete:    return ESubject14StoryPhase::Day2Investigation;
	case ESubject14StoryPhase::Day2Investigation: return ESubject14StoryPhase::Day3BreachPrep;
	case ESubject14StoryPhase::Day3BreachPrep:    return ESubject14StoryPhase::HatchUnlocked;
	case ESubject14StoryPhase::HatchUnlocked:     return ESubject14StoryPhase::FirstBreach;
	case ESubject14StoryPhase::FirstBreach:       return ESubject14StoryPhase::FirstBreach; // terminal
	}
	ensureMsgf(false, TEXT("Unhandled ESubject14StoryPhase %d in GetNextPhase"), (int32)Phase);
	return Phase;
}

// -----------------------------------------------------------------------------
// Flags
// -----------------------------------------------------------------------------

bool USubject14StorySubsystem::HasStoryFlag(const FName Flag) const
{
	return !Flag.IsNone() && StoryFlags.Contains(Flag);
}

void USubject14StorySubsystem::SetCurrentObjectiveLine(const FString& ObjectiveLine)
{
	const FString Trimmed = ObjectiveLine.TrimStartAndEnd();
	for (const FName& Flag : GetAllStoryFlagsSorted())
	{
		const FString S = Flag.ToString();
		if (S.StartsWith(TEXT("Objective:")))
		{
			SetStoryFlag(Flag, false);
		}
	}
	if (Trimmed.IsEmpty())
	{
		return;
	}
	const FName ObjectiveFlag(*FString::Printf(TEXT("Objective:%s"), *Trimmed));
	SetStoryFlag(ObjectiveFlag, true);
}

FString USubject14StorySubsystem::GetCurrentObjectiveLine() const
{
	for (const FName& Flag : GetAllStoryFlagsSorted())
	{
		const FString S = Flag.ToString();
		if (S.StartsWith(TEXT("Objective:")))
		{
			return S.RightChop(10);
		}
	}
	return FString();
}

void USubject14StorySubsystem::SetStoryFlag(const FName Flag, const bool bValue)
{
	if (Flag.IsNone())
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("USubject14StorySubsystem::SetStoryFlag called with None"));
#endif
		return;
	}

	const bool bHad = StoryFlags.Contains(Flag);
	if (bValue && !bHad)
	{
		StoryFlags.Add(Flag);
	}
	else if (!bValue && bHad)
	{
		StoryFlags.Remove(Flag);
	}
	else
	{
		return; // no change
	}

	UE_LOG(LogTemp, Log, TEXT("USubject14StorySubsystem::SetStoryFlag %s = %d"), *Flag.ToString(), bValue ? 1 : 0);
	OnStoryFlagChanged.Broadcast(Flag, bValue);
}

TArray<FName> USubject14StorySubsystem::GetAllStoryFlagsSorted() const
{
	TArray<FName> Out = StoryFlags.Array();
	Out.Sort(FNameLexicalLess());
	return Out;
}

// -----------------------------------------------------------------------------
// Notes
// -----------------------------------------------------------------------------

void USubject14StorySubsystem::RegisterReadNote(const FName NoteId)
{
	if (NoteId.IsNone())
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("USubject14StorySubsystem::RegisterReadNote called with None"));
#endif
		return;
	}
	bool bAlreadyIn = false;
	ReadNotes.Add(NoteId, &bAlreadyIn);
	if (bAlreadyIn)
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("USubject14StorySubsystem::RegisterReadNote '%s'"), *NoteId.ToString());
	OnNoteRead.Broadcast(NoteId);
}

bool USubject14StorySubsystem::HasReadNote(const FName NoteId) const
{
	return !NoteId.IsNone() && ReadNotes.Contains(NoteId);
}

TArray<FName> USubject14StorySubsystem::GetAllReadNotesSorted() const
{
	TArray<FName> Out = ReadNotes.Array();
	Out.Sort(FNameLexicalLess());
	return Out;
}

// -----------------------------------------------------------------------------
// Day / night / phase accessors
// -----------------------------------------------------------------------------

void USubject14StorySubsystem::SetPhase(const ESubject14StoryPhase NewPhase)
{
	if (NewPhase == CurrentPhase)
	{
		return;
	}
	const ESubject14StoryPhase OldPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	SyncDayNightToPhase();

	UE_LOG(LogTemp, Log,
		TEXT("USubject14StorySubsystem::SetPhase %s -> %s"),
		Subject14StorySubsystemPrivate::PhaseToText(OldPhase),
		Subject14StorySubsystemPrivate::PhaseToText(NewPhase));

	InternalBroadcastPhaseChange(OldPhase);
}

void USubject14StorySubsystem::SetDayAndNight(const int32 NewDay, const int32 NewNight)
{
	CurrentStoryDay = FMath::Max(1, NewDay);
	CurrentStoryNight = FMath::Max(0, NewNight);
	UE_LOG(LogTemp, Log,
		TEXT("USubject14StorySubsystem::SetDayAndNight Day=%d Night=%d"),
		CurrentStoryDay, CurrentStoryNight);
}

void USubject14StorySubsystem::InternalBroadcastPhaseChange(const ESubject14StoryPhase OldPhase)
{
	OnStoryPhaseChanged.Broadcast(OldPhase, CurrentPhase);
}

void USubject14StorySubsystem::SyncDayNightToPhase()
{
	switch (CurrentPhase)
	{
	case ESubject14StoryPhase::IntroWake:
		CurrentStoryDay = 1;
		CurrentStoryNight = 0;
		break;
	case ESubject14StoryPhase::Night1Active:
	case ESubject14StoryPhase::Night1Complete:
		CurrentStoryDay = FMath::Max(CurrentStoryDay, 1);
		CurrentStoryNight = FMath::Max(CurrentStoryNight, 1);
		break;
	case ESubject14StoryPhase::Day2Investigation:
		CurrentStoryDay = FMath::Max(CurrentStoryDay, 2);
		CurrentStoryNight = FMath::Max(CurrentStoryNight, 1);
		break;
	case ESubject14StoryPhase::Day3BreachPrep:
	case ESubject14StoryPhase::HatchUnlocked:
	case ESubject14StoryPhase::FirstBreach:
		CurrentStoryDay = FMath::Max(CurrentStoryDay, 3);
		CurrentStoryNight = FMath::Max(CurrentStoryNight, 1);
		break;
	default:
		break;
	}
}

ESubject14StoryPhase USubject14StorySubsystem::SanitizeLoadedPhase(const uint8 RawValue, bool& bOutWasInvalid)
{
	bOutWasInvalid = false;
	const uint8 Max = (uint8)ESubject14StoryPhase::FirstBreach;
	if (RawValue > Max)
	{
		bOutWasInvalid = true;
		return ESubject14StoryPhase::IntroWake;
	}
	return (ESubject14StoryPhase)RawValue;
}

// -----------------------------------------------------------------------------
// Persistence
// -----------------------------------------------------------------------------

void USubject14StorySubsystem::ApplySnapshotFromSaveGame(const USubject14SaveGame& Snapshot)
{
	const ESubject14StoryPhase OldPhase = CurrentPhase;

	CurrentStoryDay = FMath::Max(1, Snapshot.CurrentStoryDay);
	CurrentStoryNight = FMath::Max(0, Snapshot.CurrentStoryNight);

	bool bBadPhase = false;
	CurrentPhase = SanitizeLoadedPhase((uint8)Snapshot.CurrentPhase, bBadPhase);
#if !UE_BUILD_SHIPPING
	if (bBadPhase)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("USubject14StorySubsystem::ApplySnapshotFromSaveGame — invalid phase in save (raw=%d), clamped to %s"),
			(int32)Snapshot.CurrentPhase,
			Subject14StorySubsystemPrivate::PhaseToText(CurrentPhase));
	}
#endif
	SyncDayNightToPhase();

	StoryFlags.Reset();
	StoryFlags.Reserve(Snapshot.StoryFlags.Num());
	for (const FName& Flag : Snapshot.StoryFlags)
	{
		if (!Flag.IsNone())
		{
			StoryFlags.Add(Flag);
		}
	}

	ReadNotes.Reset();
	ReadNotes.Reserve(Snapshot.ReadNotes.Num());
	for (const FName& NoteId : Snapshot.ReadNotes)
	{
		if (!NoteId.IsNone())
		{
			ReadNotes.Add(NoteId);
		}
	}

	// Broadcast phase first so UI listeners can latch onto the new phase before
	// flag deltas rain down. For flags/notes we don't try to diff — listeners
	// that care about "on load" treat the phase change as their refresh signal.
	InternalBroadcastPhaseChange(OldPhase);
}

void USubject14StorySubsystem::WriteSnapshotToSaveGame(USubject14SaveGame& OutSnapshot) const
{
	OutSnapshot.CurrentStoryDay = CurrentStoryDay;
	OutSnapshot.CurrentStoryNight = CurrentStoryNight;
	OutSnapshot.CurrentPhase = CurrentPhase;
	OutSnapshot.StoryFlags = GetAllStoryFlagsSorted();
	OutSnapshot.ReadNotes = GetAllReadNotesSorted();
	OutSnapshot.SavedAtAppSeconds = FPlatformTime::Seconds();
	OutSnapshot.SaveSchemaVersion = 1;
}

bool USubject14StorySubsystem::SaveProgressToSlot()
{
	if (SaveSlotName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("USubject14StorySubsystem::SaveProgressToSlot — empty SaveSlotName."));
		return false;
	}

	USubject14SaveGame* const Snapshot = Cast<USubject14SaveGame>(
		UGameplayStatics::CreateSaveGameObject(USubject14SaveGame::StaticClass()));
	if (!ensureMsgf(Snapshot, TEXT("Failed to create USubject14SaveGame")))
	{
		return false;
	}

	WriteSnapshotToSaveGame(*Snapshot);

	const bool bOk = UGameplayStatics::SaveGameToSlot(Snapshot, SaveSlotName, SaveUserIndex);
	if (!bOk)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("USubject14StorySubsystem::SaveProgressToSlot — SaveGameToSlot failed (slot='%s' user=%d)"),
			*SaveSlotName, SaveUserIndex);
		return false;
	}

	UE_LOG(LogTemp, Log,
		TEXT("USubject14StorySubsystem::SaveProgressToSlot — wrote slot='%s' (flags=%d notes=%d phase=%s)"),
		*SaveSlotName,
		Snapshot->StoryFlags.Num(),
		Snapshot->ReadNotes.Num(),
		Subject14StorySubsystemPrivate::PhaseToText(Snapshot->CurrentPhase));
	return true;
}

bool USubject14StorySubsystem::LoadProgressFromSlot()
{
	if (!DoesProgressSlotExist())
	{
		UE_LOG(LogTemp, Log,
			TEXT("USubject14StorySubsystem::LoadProgressFromSlot — no slot '%s' (user=%d)"),
			*SaveSlotName, SaveUserIndex);
		return false;
	}

	USaveGame* const Loaded = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex);
	USubject14SaveGame* const Snapshot = Cast<USubject14SaveGame>(Loaded);
	if (!Snapshot)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("USubject14StorySubsystem::LoadProgressFromSlot — LoadGameFromSlot returned null or wrong class for '%s'"),
			*SaveSlotName);
		return false;
	}

	ApplySnapshotFromSaveGame(*Snapshot);
	UE_LOG(LogTemp, Log,
		TEXT("USubject14StorySubsystem::LoadProgressFromSlot — loaded '%s' (flags=%d notes=%d phase=%s)"),
		*SaveSlotName,
		Snapshot->StoryFlags.Num(),
		Snapshot->ReadNotes.Num(),
		Subject14StorySubsystemPrivate::PhaseToText(Snapshot->CurrentPhase));
	return true;
}

bool USubject14StorySubsystem::DoesProgressSlotExist() const
{
	return !SaveSlotName.IsEmpty()
		&& UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

bool USubject14StorySubsystem::DeleteProgressSlot()
{
	if (!DoesProgressSlotExist())
	{
		return false;
	}
	const bool bOk = UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	UE_LOG(LogTemp, Log,
		TEXT("USubject14StorySubsystem::DeleteProgressSlot('%s', %d) -> %d"),
		*SaveSlotName, SaveUserIndex, bOk ? 1 : 0);
	return bOk;
}

// -----------------------------------------------------------------------------
// Debug dump (driven by the Subject14.DumpStory console command)
// -----------------------------------------------------------------------------

void USubject14StorySubsystem::DumpStoryToLog() const
{
	const TArray<FName> FlagsSorted = GetAllStoryFlagsSorted();
	const TArray<FName> NotesSorted = GetAllReadNotesSorted();

	UE_LOG(LogTemp, Display,
		TEXT("[Subject14Story] Day=%d Night=%d Phase=%s"),
		CurrentStoryDay,
		CurrentStoryNight,
		Subject14StorySubsystemPrivate::PhaseToText(CurrentPhase));

	UE_LOG(LogTemp, Display, TEXT("[Subject14Story] Flags (%d):"), FlagsSorted.Num());
	for (const FName& Flag : FlagsSorted)
	{
		UE_LOG(LogTemp, Display, TEXT("  + %s"), *Flag.ToString());
	}

	UE_LOG(LogTemp, Display, TEXT("[Subject14Story] Read notes (%d):"), NotesSorted.Num());
	for (const FName& NoteId : NotesSorted)
	{
		UE_LOG(LogTemp, Display, TEXT("  + %s"), *NoteId.ToString());
	}

	if (GEngine)
	{
		const FString Summary = FString::Printf(
			TEXT("Subject14 Story: Day %d Night %d Phase %s | Flags %d | Notes %d"),
			CurrentStoryDay,
			CurrentStoryNight,
			Subject14StorySubsystemPrivate::PhaseToText(CurrentPhase),
			FlagsSorted.Num(),
			NotesSorted.Num());
		GEngine->AddOnScreenDebugMessage((uint64)0x5314500, 8.0f, FColor::Cyan, Summary);
	}
}
