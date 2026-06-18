#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subject14StoryTypes.h"
#include "Subject14StorySubsystem.generated.h"

class USubject14SaveGame;

/** Broadcast when CurrentPhase changes (old -> new). Fires on load too. */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FSubject14StoryPhaseChanged,
	ESubject14StoryPhase /*OldPhase*/,
	ESubject14StoryPhase /*NewPhase*/);

/** Broadcast whenever a flag is set or cleared. Fires on load too. */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FSubject14StoryFlagChanged,
	FName /*Flag*/,
	bool /*bValue*/);

/** Broadcast whenever a NoteId transitions from unread -> read (dedup'd). */
DECLARE_MULTICAST_DELEGATE_OneParam(
	FSubject14NoteRead,
	FName /*NoteId*/);

/** Stable flag names shared between triggers / actors / director. */
namespace Subject14StoryFlags
{
	// Day 1 / Night 1
	extern SUBJECT_14_API const FName Night1Started;
	extern SUBJECT_14_API const FName Night1Completed;

	// Day 2 investigation
	extern SUBJECT_14_API const FName Day2Started;

	// Day 3 breach path
	extern SUBJECT_14_API const FName HatchDiscovered;   // rug moved / lid seen
	extern SUBJECT_14_API const FName HatchHasPower;     // breaker online
	extern SUBJECT_14_API const FName HatchUnlocked;     // lock released
	extern SUBJECT_14_API const FName HatchOpened;       // lid opened = first breach
}

/**
 * Persistent run-wide progression state for Subject 14.
 *
 * Lives on the GameInstance so it survives `OpenLevel` transitions between
 * Lvl_MainMenu, Lvl_Dev, and any future night maps. This is the single source
 * of truth; the save game, triggers, notes, and the night director all talk
 * to it through the API below.
 *
 * Not thread-safe. All calls expected on game thread.
 */
UCLASS(BlueprintType)
class SUBJECT_14_API USubject14StorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- UGameInstanceSubsystem ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Convenience getter — returns nullptr if WorldContextObject has no GameInstance. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Subject14 Story Subsystem"))
	static USubject14StorySubsystem* Get(const UObject* WorldContextObject);

	// --- Run lifecycle ---

	/**
	 * Resets to a clean pre-game state (phase = IntroWake, day 1/night 0, no flags,
	 * no read notes). Does NOT delete the save slot — call DeleteProgressSlot for that.
	 */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story")
	void StartNewGame();

	/** Moves the story to the next canonical beat. Returns true if the phase changed. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story")
	bool AdvanceToNextStoryBeat();

	// --- Flags ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story")
	bool HasStoryFlag(FName Flag) const;

	UFUNCTION(BlueprintCallable, Category = "Subject14|Story")
	void SetStoryFlag(FName Flag, bool bValue);

	/**
	 * Sets the single "Objective:<text>" story flag (same encoding as story triggers).
	 * Clears any existing Objective:* flags first so only one objective line persists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story")
	void SetCurrentObjectiveLine(const FString& ObjectiveLine);

	/** Returns the text after the Objective: prefix, or empty if none is set. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story")
	FString GetCurrentObjectiveLine() const;

	/** Read-only copy for debug UIs / savegame; returns sorted for determinism. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story")
	TArray<FName> GetAllStoryFlagsSorted() const;

	// --- Notes ---

	/** Idempotent: registering the same NoteId twice only fires the delegate once. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story")
	void RegisterReadNote(FName NoteId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story")
	bool HasReadNote(FName NoteId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story")
	TArray<FName> GetAllReadNotesSorted() const;

	// --- Day / night / phase accessors ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story")
	int32 GetCurrentStoryDay() const { return CurrentStoryDay; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story")
	int32 GetCurrentStoryNight() const { return CurrentStoryNight; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story")
	ESubject14StoryPhase GetCurrentPhase() const { return CurrentPhase; }

	/** Forces a specific phase and broadcasts the change. Normally prefer AdvanceToNextStoryBeat. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story")
	void SetPhase(ESubject14StoryPhase NewPhase);

	/** Designer / debug override: bump the day/night counters explicitly. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story")
	void SetDayAndNight(int32 NewDay, int32 NewNight);

	// --- Persistence ---

	/** Writes current state to the configured save slot. Returns false on IO/serialization failure. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story|Save")
	bool SaveProgressToSlot();

	/** Rehydrates from the configured save slot. Returns false if slot missing or corrupt. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story|Save")
	bool LoadProgressFromSlot();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subject14|Story|Save")
	bool DoesProgressSlotExist() const;

	UFUNCTION(BlueprintCallable, Category = "Subject14|Story|Save")
	bool DeleteProgressSlot();

	/** Overridden by cheat / automation paths. Stable default is below. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Save")
	FString SaveSlotName = TEXT("Subject14_Story");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Save")
	int32 SaveUserIndex = 0;

	// --- Delegates (native only; fine for C++ consumers like the director) ---
	FSubject14StoryPhaseChanged OnStoryPhaseChanged;
	FSubject14StoryFlagChanged OnStoryFlagChanged;
	FSubject14NoteRead OnNoteRead;

	// --- Debug / dump ---

	/** Dumps day/night/phase/flags/read-notes to the log + on-screen debug. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Story|Debug")
	void DumpStoryToLog() const;

	// Registered as FAutoConsoleCommandWithWorldAndArgs in Initialize(), NOT as
	// UFUNCTION(Exec). UFUNCTION(Exec) only dispatches on PlayerController /
	// CheatManager / GameInstance / etc., never on UGameInstanceSubsystem, so
	// we'd silently lose the commands. Registered names:
	//
	//   Subject14.DumpStory
	//   Subject14.SetStoryPhase <0..6>
	//   Subject14.SetStoryFlag  <Flag> <0|1>
	//   Subject14.ClearStory
	//   Subject14.AdvanceStory
	//   Subject14.SaveStory
	//   Subject14.LoadStory
	//   Subject14.DeleteStory
	//
	// They resolve the per-world GameInstance's subsystem at invocation time so
	// PIE + cooked both work.

protected:
	/** Canonical mapping used by AdvanceToNextStoryBeat. */
	static ESubject14StoryPhase GetNextPhase(ESubject14StoryPhase Phase);

	void ApplySnapshotFromSaveGame(const USubject14SaveGame& Snapshot);
	void WriteSnapshotToSaveGame(USubject14SaveGame& OutSnapshot) const;

	void InternalBroadcastPhaseChange(ESubject14StoryPhase OldPhase);

	/** Keeps CurrentStoryDay / CurrentStoryNight aligned with CurrentPhase (single source of truth). */
	void SyncDayNightToPhase();

	/** Clamps arbitrary uint8 / disk values to a valid story phase. */
	static ESubject14StoryPhase SanitizeLoadedPhase(uint8 RawValue, bool& bOutWasInvalid);

private:
	UPROPERTY(Transient)
	int32 CurrentStoryDay = 1;

	UPROPERTY(Transient)
	int32 CurrentStoryNight = 0;

	UPROPERTY(Transient)
	ESubject14StoryPhase CurrentPhase = ESubject14StoryPhase::IntroWake;

	/** Not a UPROPERTY: FName TSet is GC-safe on its own and we don't need reflection here. */
	TSet<FName> StoryFlags;

	TSet<FName> ReadNotes;

	// Console-command registrations held for the subsystem's lifetime.
	TArray<TUniquePtr<FAutoConsoleCommandWithWorldAndArgs>> ConsoleCommands;
};
