#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subject14Night1Types.h"
#include "Subject14Night1Director.generated.h"

class USoundBase;
class USubject14NightEndPromptWidget;

/**
 * Placed in a level to drive a short Night 1 timeline (~5–8 minutes by default).
 * Fires editor-configured events; safe if optional sounds are unset.
 */
UCLASS(Blueprintable)
class SUBJECT_14_API ASubject14Night1Director : public AActor
{
	GENERATED_BODY()

public:
	ASubject14Night1Director();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

public:
	/** Total nominal length of the night slice (used for a forced end failsafe). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Timing", meta = (ClampMin = "60.0", ClampMax = "1200.0"))
	float NightDurationSeconds = 95.0f;

	/** Scales how fast the internal night clock advances (1 = real-time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Timing", meta = (ClampMin = "0.05", ClampMax = "20.0"))
	float SequenceTimeScale = 1.0f;

	/** Added to the night clock at start (seconds into the sequence). Useful for PIE iteration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Timing|Debug", meta = (ClampMin = "0.0"))
	float DebugSequenceStartOffsetSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Timeline")
	TArray<FSubject14Night1ScheduleEntry> Schedule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Thoughts")
	TArray<FString> ThoughtLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Thoughts", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float ThoughtFadeInSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Thoughts", meta = (ClampMin = "2.0", ClampMax = "30.0"))
	float ThoughtDisplaySeconds = 6.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Thoughts", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float ThoughtFadeOutSeconds = 0.5f;

	/** If set, ambient bed swaps to this when the “ambient shift” event runs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Ambient")
	TObjectPtr<USoundBase> AmbientSoundAfterShift;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch")
	TObjectPtr<USoundBase> GlitchOneShotSound;

	/** Seconds of subtle ambient duck before the peak glitch (0 skips). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float GlitchBuildupSeconds = 0.38f;

	/** Ambient scalar during buildup (still playing, not paused). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float GlitchBuildupAmbientScalar = 0.88f;

	/** Hold time at the harsh peak (pause + dip + flicker). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float GlitchPeakHoldSeconds = 0.28f;

	/** Ambient scalar at peak (while paused). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlitchPeakAmbientScalar = 0.06f;

	/** Total recovery time after peak before returning to full normal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampMin = "0.05", ClampMax = "6.0"))
	float GlitchRecoverSeconds = 0.72f;

	/** Portion of recover time spent at a mid scalar before snapping to full. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float GlitchRecoverStageRatio = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float GlitchRecoverMidAmbientScalar = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampIntMin = "2", ClampIntMax = "48"))
	int32 GlitchPeakFlickerCount = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Glitch", meta = (ClampMin = "0.01", ClampMax = "0.2"))
	float GlitchPeakFlickerStepSeconds = 0.032f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature")
	TObjectPtr<USoundBase> CreatureHintSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature")
	TObjectPtr<USoundBase> CreatureStingSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampMin = "200.0", ClampMax = "8000.0"))
	float CreatureHintDistance = 1650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CreatureHintVolume = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CreatureStingVolume = 0.18f;

	/** Seconds of optional ambient duck / silence before the hint audio (0 disables). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float CreaturePreHintSilenceSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float CreaturePreHintAmbientScalar = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature")
	bool bCreatureHintOnceOnly = true;

	/** If > 0 and CreatureHintAnchor is set, skip the hint when the player is farther than this from the anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampMin = "0.0", ClampMax = "20000.0"))
	float CreatureHintMaxPlayerDistanceFromAnchor = 0.0f;

	/**
	 * If > 0 and CreatureHintAnchor is set, require the player forward (XY) to be within this half-angle (degrees)
	 * toward the anchor. 0 disables the facing check.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampMin = "0.0", ClampMax = "175.0"))
	float CreatureHintFacingConeHalfAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampIntMin = "0", ClampIntMax = "24"))
	int32 CreatureHintFlashlightFlickerCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Creature", meta = (ClampMin = "0.01", ClampMax = "0.2"))
	float CreatureHintFlashlightFlickerStepSeconds = 0.038f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End", meta = (MultiLine = "true"))
	FString EndNightMessage = TEXT("Day breaks.\n\nThe forest is quiet again—for now.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float EndNightFadeSeconds = 2.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End", meta = (ClampMin = "0.0", ClampMax = "8.0"))
	float EndNightCaptionDelaySeconds = 2.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End", meta = (ClampMin = "2.0", ClampMax = "30.0"))
	float EndNightCaptionHoldSeconds = 6.0f;

	/** Ducks ambient bed when the end sequence starts (before optional pause). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EndNightAmbientDuckScalar = 0.22f;

	/** Pauses ambient after the camera fade completes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End")
	bool bEndNightPauseAmbientAfterFade = true;

	// --- End-night return / prompt (post-caption) ---

	/** If true, after the end prompt (or immediately if prompt is off), open ReturnToMenuMapName instead of restarting this level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return")
	bool bReturnToMenuAfterNightEnd = true;

	/** Level to open after Night 1 (e.g. Lvl_MainMenu once created, or Lvl_Dev to reload the test map). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return")
	FName ReturnToMenuMapName = FName(TEXT("Lvl_Dev"));

	/** Wait this many seconds after the end caption overlay has fully finished before showing the prompt (or auto-return if prompt is off). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return", meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float DelayBeforeEndPromptSeconds = 0.5f;

	/** If false, skips the “press any key” UI and immediately performs the same return/restart as a keypress would. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return")
	bool bShowEndPromptAfterCaption = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return", meta = (MultiLine = "false"))
	FString EndPromptText = TEXT("Press any key to continue");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return")
	bool bAllowAnyKeyReturn = true;

	/** Fade-in duration for the end prompt text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return", meta = (ClampMin = "0.1", ClampMax = "6.0"))
	float EndPromptFadeInSeconds = 0.65f;

	/** Non-shipping: auto-restart the current level after DevRestartDelaySeconds, skipping the prompt. Ignored in shipping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return|Debug")
	bool bDevAutoRestartInstead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return|Debug", meta = (ClampMin = "0.0", ClampMax = "120.0"))
	float DevRestartDelaySeconds = 3.0f;

	/** Non-shipping: verbose LogTemp lines for the end-night return phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|End|Return|Debug")
	bool bLogEndStateTransitions = true;

	// --- Optional scene anchors (unset = legacy behavior) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Anchors")
	TObjectPtr<AActor> CreatureHintAnchor;

	/** Designer hint only: validated in editor / non-shipping for nearby note actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Anchors")
	TObjectPtr<AActor> NoteSuggestedAnchor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Anchors")
	TObjectPtr<AActor> GlitchFocusAnchor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Anchors")
	TObjectPtr<AActor> EndNightFocusAnchor;

	/** Advances the internal clock to the next schedule entry and fires it (no-op in shipping). */
	UFUNCTION(BlueprintCallable, Category = "Night1|Debug", meta = (DevelopmentOnly = "true"))
	void DebugSkipToNextScheduleEvent();

	/** Invoked by USubject14NightEndPromptWidget when the player accepts the end prompt (debounced). */
	void HandleEndNightPromptCommitted();

	/** When true, BeginPlay consults the story subsystem before starting the night timeline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Story")
	bool bAutoStartFromStoryState = true;

	/** When true with bAutoStartFromStoryState, the director only runs during IntroWake / Night1Active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Story")
	bool bOnlyRunDuringNight1Phase = true;

	/** When true, EndNight advances story to Day 2 investigation and saves progress before reloading. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night1|Story")
	bool bCommitStoryProgressOnEndNight = true;

private:
	bool ShouldRunNightTimeline();
	void CommitNight1StoryProgress();

	float NightElapsed = 0.0f;
	int32 NextScheduleIndex = 0;
	bool bNightActive = false;
	bool bNightEnded = false;
	bool bForcedEndFired = false;
	bool bCreatureHintConsumed = false;

	TArray<FSubject14Night1ScheduleEntry> ActiveSchedule;

	FTimerHandle GlitchBuildupTimer;
	FTimerHandle GlitchPeakTimer;
	FTimerHandle GlitchRecoverStageTimer;
	FTimerHandle GlitchRecoverTimer;
	FTimerHandle AmbientShiftRelaxTimer;
	FTimerHandle EndCaptionTimer;
	FTimerHandle EndNightAmbientPauseTimer;
	FTimerHandle CreatureHintTimer;
	FTimerHandle EndAwaitPromptTimer;
	FTimerHandle DevAutoRestartTimer;

	bool bEndReturnTriggered = false;
	TWeakObjectPtr<USubject14NightEndPromptWidget> EndPromptWidget;

	void SortActiveSchedule();
	void RebuildActiveScheduleFromSchedule();
	void ProcessTimeline();
	void FireEvent(const FSubject14Night1ScheduleEntry& Entry, const float SourceTimeSeconds);

	void Event_AmbientShift();
	void Event_EnvironmentGlitch();
	void Event_ThoughtText(int32 LineIndex);
	void Event_CreatureHint();
	void Event_EndNight();

	void ClearGlitchTimers();
	void GlitchBeginBuildup();
	void GlitchEnterPeak();
	void GlitchLeavePeakStartRecover();
	void GlitchRecoverMidStage();
	void GlitchRecoverFinal();

	void AmbientShiftRelax();
	void ShowEndNightCaption();
	void PauseAmbientAfterEndFade();

	void ScheduleCreatureHintPlayback();
	void PlayCreatureHintAudioAndEffects();

	void ValidateNight1Setup() const;
	void Night1DebugLogEvent(
		const TCHAR* EventName,
		float ScheduledFireTimeSeconds,
		bool bUsedFallback,
		const TCHAR* FallbackNote) const;

	void EndNightSequenceLog(const TCHAR* Message) const;
	void ScheduleEndNightReturnPhase();
	void BeginEndPromptPhase();
	void OnDevAutoRestartFire();
	void ExecuteNightEndReturn();
};
