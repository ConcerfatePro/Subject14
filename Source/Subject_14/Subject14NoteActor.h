#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Subject14StoryTypes.h"
#include "Subject14NoteActor.generated.h"

class UStaticMeshComponent;
class USubject14StorySubsystem;

/** Readable note: E shows title + body (log + on-screen). Optional story subsystem integration. */
UCLASS(Blueprintable)
class SUBJECT_14_API ASubject14NoteActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASubject14NoteActor();

	virtual void Subject14Interact_Implementation(AActor* Instigator) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Content")
	FString NoteTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Content", meta = (MultiLine = "true"))
	FString NoteBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Presentation", meta = (ClampMin = "2.0", ClampMax = "45.0"))
	float NoteDisplaySeconds = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Presentation", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float NoteFadeInSeconds = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Presentation", meta = (ClampMin = "0.05", ClampMax = "8.0"))
	float NoteFadeOutSeconds = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Interaction")
	bool bReadOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Interaction", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float InteractionCooldownSeconds = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Graybox")
	TObjectPtr<UStaticMesh> GrayboxMarkerMeshOverride;

	// --- Story subsystem (optional) ---

	/** Stable id for save + duplicate detection; required when bRegisterInStorySubsystem is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story")
	FName NoteId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story")
	bool bRegisterInStorySubsystem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story")
	FName GrantedStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story")
	FName RequiredStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story", meta = (ToolTip = "If set, the note cannot be read while this flag is present on the story subsystem."))
	FName BlockedByStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story")
	ESubject14StoryPhase RequiredPhaseAtLeast = ESubject14StoryPhase::IntroWake;

	/**
	 * When true, GrantedStoryFlag / objective / delayed thought only apply on the first successful read
	 * for this NoteId (tracked via subsystem HasReadNote before RegisterReadNote).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story")
	bool bOnlyFirstReadGrantsProgression = true;

	/** Optional second beat after the main note overlay (timer; does not replace the first read). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (MultiLine = "true"))
	FString ThoughtAfterRead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float ThoughtAfterReadDelaySeconds = 3.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (MultiLine = "false"))
	FString ObjectiveAfterRead;

	/** Shown when story gates block reading (no main note body). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Story|Feedback", meta = (MultiLine = "true"))
	FString ThoughtWhenGateBlocked;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	bool EvaluateStoryGate(const USubject14StorySubsystem& Subsystem) const;
	void ApplyStoryProgression(USubject14StorySubsystem& Subsystem, bool bFirstProgressionRead);
	void WarnDuplicateNoteIdsInLevel() const;

	UFUNCTION()
	void PlayDelayedThoughtAfterRead();

	float LastInteractGameTime = -100000.0f;
	bool bHasBeenRead = false;

	FTimerHandle DelayedThoughtTimer;

	/** When not using NoteId registration, prevents repeating Granted/Objective when bOnlyFirstReadGrantsProgression is true. */
	bool bProgressionEffectsConsumed = false;
};
