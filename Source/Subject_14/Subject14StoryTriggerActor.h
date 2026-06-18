#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Subject14StoryTypes.h"
#include "Subject14StoryTriggerActor.generated.h"

class UBillboardComponent;
class UBoxComponent;
class USubject14StorySubsystem;

/**
 * Generic designer-placeable trigger that writes into the story subsystem.
 *
 * Fires on:
 *   - Pawn overlap (if bFireOnOverlap is true)
 *   - Explicit interact via IInteractable::Subject14Interact (if bFireOnInteract is true)
 *
 * On fire it will, in order:
 *   1. Validate RequiredStoryFlag / RequiredPhaseAtLeast
 *   2. Set GrantedStoryFlag (if any) to true
 *   3. Advance the story phase if bAdvancePhaseOnFire is true
 *   4. Show a one-line thought via USubject14ThoughtOverlayWidget (if ThoughtLine set)
 *   5. Log an objective / notification string (if ObjectiveLine set) — the Phase 3
 *      objective widget subscribes to the same subsystem / flag events that end up firing.
 *
 * Safe to leave half-configured: each field is independently optional.
 */
UCLASS(Blueprintable)
class SUBJECT_14_API ASubject14StoryTriggerActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASubject14StoryTriggerActor();

	virtual void Subject14Interact_Implementation(AActor* Instigator) override;

	// --- Gating ---

	/** If non-None, the trigger is inert until this flag is set on the story subsystem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Gate")
	FName RequiredStoryFlag = NAME_None;

	/**
	 * If non-None, the trigger does not fire while this flag is present on the story subsystem.
	 * Independent of RequiredStoryFlag (both gates must pass).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Gate")
	FName BlockedByStoryFlag = NAME_None;

	/** Trigger is inert until CurrentPhase >= RequiredPhaseAtLeast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Gate")
	ESubject14StoryPhase RequiredPhaseAtLeast = ESubject14StoryPhase::IntroWake;

	// --- Effects ---

	/** Set on fire (no-op if None). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Effects")
	FName GrantedStoryFlag = NAME_None;

	/** Cleared on fire (no-op if None). Applied AFTER GrantedStoryFlag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Effects")
	FName ClearedStoryFlag = NAME_None;

	/**
	 * If true, the trigger also calls AdvanceToNextStoryBeat() on fire.
	 * Prefer flag-driven beats; enable this only for the rare cue that should
	 * itself be the story-beat gate (e.g. "wake up" cinematic endpoint).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Effects")
	bool bAdvancePhaseOnFire = false;

	/** Optional single thought overlay shown on fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (MultiLine = "true"))
	FString ThoughtLine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float ThoughtHoldSeconds = 5.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (ClampMin = "0.05", ClampMax = "3.0"))
	float ThoughtFadeInSeconds = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (ClampMin = "0.05", ClampMax = "4.0"))
	float ThoughtFadeOutSeconds = 1.0f;

	/**
	 * Passed through to any listening objective widget (Phase 3). Stored as a
	 * story flag of the form "Objective:<text>" so the same persistence path
	 * is reused with no extra save fields.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (MultiLine = "false"))
	FString ObjectiveLine;

	/** Fires at most once per play session. bOneShotPersistent survives save/load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Behavior")
	bool bOneShot = true;

	/**
	 * If true AND bOneShot, GrantedStoryFlag (or the auto-generated one-shot flag) is
	 * used to block re-fire across save/load. If GrantedStoryFlag is None, we use the
	 * actor's FName as the blocker.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Behavior")
	bool bOneShotPersistent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Behavior")
	bool bFireOnOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Behavior")
	bool bFireOnInteract = false;

	/** Only the local player pawn is considered for overlap. Useful for multiplayer-safe no-ops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Behavior")
	bool bOnlyLocalPlayerOverlap = true;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14")
	TObjectPtr<UBoxComponent> TriggerBox;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14")
	TObjectPtr<UBillboardComponent> EditorIcon;
#endif

private:
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	bool EvaluateGating(const USubject14StorySubsystem& Subsystem) const;
	bool EvaluateOneShotBlock(const USubject14StorySubsystem& Subsystem) const;
	FName ResolveOneShotFlagName() const;

	/** Single-player guard: when multiple local players exist, only instigator matching player 0 may advance macro phase. */
	bool ShouldAllowStoryPhaseAdvanceFromInstigator(AActor* Instigator) const;

	void FireNow(AActor* Instigator);

	bool bHasFiredThisSession = false;
};
