#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subject14StoryTypes.h"
#include "Subject14SaveGame.generated.h"

/**
 * Flat serialization of the story subsystem state.
 *
 * Kept intentionally small and dumb: this class owns no game logic. The story
 * subsystem copies its runtime state in/out of one of these via SaveGameToSlot /
 * LoadGameFromSlot (USubject14StorySubsystem::SaveProgressToSlot / LoadProgressFromSlot).
 */
UCLASS()
class SUBJECT_14_API USubject14SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Matches USubject14StorySubsystem::CurrentStoryDay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Save")
	int32 CurrentStoryDay = 1;

	/** Matches USubject14StorySubsystem::CurrentStoryNight. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Save")
	int32 CurrentStoryNight = 0;

	/** Matches USubject14StorySubsystem::CurrentPhase. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Save")
	ESubject14StoryPhase CurrentPhase = ESubject14StoryPhase::IntroWake;

	/** Sorted for diff-friendly saves; the subsystem rehydrates into a TSet. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Save")
	TArray<FName> StoryFlags;

	/** Persisted NoteIds that have already been read. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Save")
	TArray<FName> ReadNotes;

	/** Epoch save timestamp (seconds since app launch — OK for basic ordering/debug). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Save")
	double SavedAtAppSeconds = 0.0;

	/** Build / schema tag so we can bump when the format breaks. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Save")
	int32 SaveSchemaVersion = 1;
};
