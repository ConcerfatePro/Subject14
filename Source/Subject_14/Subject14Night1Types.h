#pragma once

#include "CoreMinimal.h"
#include "Subject14Night1Types.generated.h"

UENUM(BlueprintType)
enum class ENight1EventType : uint8
{
	AmbientShift UMETA(DisplayName = "Ambient shift"),
	EnvironmentGlitch UMETA(DisplayName = "Environment glitch"),
	ThoughtText UMETA(DisplayName = "Thought text"),
	CreatureHint UMETA(DisplayName = "Creature hint"),
	EndNight UMETA(DisplayName = "End night")
};

/** One row in the Night 1 timeline (sorted by TimeSeconds at runtime). */
USTRUCT(BlueprintType)
struct FSubject14Night1ScheduleEntry
{
	GENERATED_BODY()

	/** Seconds from night start when this event fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1")
	float TimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1")
	ENight1EventType EventType = ENight1EventType::ThoughtText;

	/**
	 * For ThoughtText: index into the director's ThoughtLines array.
	 * Other event types ignore this unless extended later.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1")
	int32 ThoughtLineIndex = 0;

	/** Random offset applied once at night start (±seconds). 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1", meta = (ClampMin = "0.0"))
	float TimeSecondsJitter = 0.0f;
};
