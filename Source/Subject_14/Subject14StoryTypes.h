#pragma once

#include "CoreMinimal.h"
#include "Subject14StoryTypes.generated.h"

/**
 * Top-level story phase for the Night 1 -> Day 3 breach vertical slice.
 *
 * Progression is strictly linear: we never move backward, and every phase has
 * a well-defined successor used by USubject14StorySubsystem::AdvanceToNextStoryBeat.
 */
UENUM(BlueprintType)
enum class ESubject14StoryPhase : uint8
{
	/** Pre-gameplay / fresh save. Player has not yet woken up in the cabin. */
	IntroWake UMETA(DisplayName = "Intro / wake"),

	/** Night 1 is running (Subject14Night1Director driving the timeline). */
	Night1Active UMETA(DisplayName = "Night 1 active"),

	/** Night 1 director fired EndNight; player is returning to day space. */
	Night1Complete UMETA(DisplayName = "Night 1 complete"),

	/** Day 2: investigation beats — notes, environment cues, small unease. */
	Day2Investigation UMETA(DisplayName = "Day 2 investigation"),

	/** Day 3 setup: breaker / tools / rug removal before the hatch can open. */
	Day3BreachPrep UMETA(DisplayName = "Day 3 breach prep"),

	/** Hatch has been powered + unlocked, but not yet opened. */
	HatchUnlocked UMETA(DisplayName = "Hatch unlocked"),

	/** Player opened the hatch for the first time — illusion has cracked. */
	FirstBreach UMETA(DisplayName = "First breach")
};
