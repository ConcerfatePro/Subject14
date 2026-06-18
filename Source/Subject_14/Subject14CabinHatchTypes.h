#pragma once

#include "CoreMinimal.h"
#include "Subject14CabinHatchTypes.generated.h"

/**
 * Cabin hatch presentation / interaction bucket.
 *
 * Discovered vs LockedNoPower: both use subsystem flags, except "Discovered" can
 * represent a soft cue (rug disturbed / wrongness sensed) via transient actor
 * state before HatchDiscovered is committed — see ASubject14CabinHatchActor.
 */
UENUM(BlueprintType)
enum class ESubject14CabinHatchState : uint8
{
	Concealed UMETA(DisplayName = "Concealed"),
	/** Soft cue — rug disturbed / wrongness before formal discovery flag (transient session state). */
	Discovered UMETA(DisplayName = "Discovered (cue)"),
	/** Hatch is known; facility line to hatch is dead. */
	LockedNoPower UMETA(DisplayName = "Locked — no power"),
	/** Power online; physical lock engaged. */
	LockedPowered UMETA(DisplayName = "Locked — powered"),
	/** Lock released; ready to crank open. */
	Unlocked UMETA(DisplayName = "Unlocked"),
	Opening UMETA(DisplayName = "Opening"),
	Open UMETA(DisplayName = "Open (breached)")
};
