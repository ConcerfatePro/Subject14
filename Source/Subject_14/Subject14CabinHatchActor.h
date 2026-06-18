#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Subject14CabinHatchTypes.h"
#include "Subject14CabinHatchActor.generated.h"

class UAudioComponent;
class USoundBase;
class USceneComponent;
class USphereComponent;
class USpotLightComponent;
class UStaticMeshComponent;
class USubject14StorySubsystem;

/**
 * Cabin hatch — first structural crack in the forest simulation.
 * Subsystem flags (Subject14StoryFlags) are authoritative; local fields are presentation only.
 */
UCLASS(Blueprintable)
class SUBJECT_14_API ASubject14CabinHatchActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASubject14CabinHatchActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Subject14Interact_Implementation(AActor* Instigator) override;

	UFUNCTION(BlueprintPure, Category = "Subject14|Hatch")
	ESubject14CabinHatchState GetHatchState() const;

	/** Called by breaker panel after it sets HatchHasPower on the subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Hatch")
	void RefreshPresentationFromSubsystem();

protected:
	UFUNCTION()
	void HandleStoryFlagChanged(FName Flag, bool bValue);

	UFUNCTION()
	void HandleDiscoveryOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void RefreshPresentation();
	void TryCommitDiscoveryFromInteract(AActor* Instigator);
	void TryUnlockFromPowered(AActor* Instigator);
	void TryBeginOpen(AActor* Instigator);
	void FinishOpening();

	bool EvaluateStoryGate(const USubject14StorySubsystem& Subsystem) const;
	bool IsInstigatorPrimarySinglePlayerPawn(AActor* Instigator) const;

public:
	// --- Visuals ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Hatch")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Hatch")
	TObjectPtr<UStaticMeshComponent> RugMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Hatch")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Hatch")
	TObjectPtr<UStaticMeshComponent> LidMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Hatch")
	TObjectPtr<USpotLightComponent> LockIndicatorLight;

	/** Optional overlap cue before formal discovery (sets transient Discovered state). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Hatch")
	TObjectPtr<USphereComponent> DiscoveryVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Presentation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LockLightIntensityPowered = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Presentation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LockLightIntensityUnlocked = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Door", meta = (ClampMin = "-175.0", ClampMax = "175.0"))
	float LidOpenYawDegrees = -95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Door", meta = (ClampMin = "0.5", ClampMax = "25.0"))
	float LidOpenInterpSpeed = 6.0f;

	/** Future handoff: teleport / streaming anchor below the cabin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Descent")
	TObjectPtr<USceneComponent> DescentAnchor;

	// --- Audio ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Hatch|Audio")
	TObjectPtr<UAudioComponent> HumAudio;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Hatch|Audio")
	TObjectPtr<USoundBase> HumLoopSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Hatch|Audio")
	TObjectPtr<USoundBase> DiscoverySound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Hatch|Audio")
	TObjectPtr<USoundBase> UnlockSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Hatch|Audio")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Hatch|Audio")
	TObjectPtr<USoundBase> SealedNoPowerSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Hatch|Audio")
	TObjectPtr<USoundBase> SealedPoweredSound;

	// --- Story / gating ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Gate")
	FName RequiredStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Gate", meta = (ToolTip = "If set, hatch ignores local player while this flag is true."))
	FName BlockedByStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Gate")
	bool bRequireProximityCueBeforeFormalDiscovery = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Unlock")
	bool bUnlockOnInteractWhenPowered = true;

	/** If not None, HatchUnlocked is only granted when this flag is already true (e.g. key). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Unlock")
	FName RequiredStoryFlagToUnlock = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Unlock")
	bool bAdvanceMacroPhaseToHatchUnlockedOnUnlock = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Feedback", meta = (MultiLine = "true"))
	FString ThoughtOnProximityCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Feedback", meta = (MultiLine = "true"))
	FString ThoughtOnDiscovery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Feedback", meta = (MultiLine = "true"))
	FString ThoughtOnNoPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Feedback", meta = (MultiLine = "true"))
	FString ThoughtOnPoweredSealed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Feedback", meta = (MultiLine = "true"))
	FString ThoughtOnOpenBlocked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Feedback", meta = (MultiLine = "true"))
	FString ThoughtWhenGateBlocked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Hatch|Story")
	bool bSaveProgressOnStateChange = true;

private:
	bool bProximityCue = false;
	bool bOpeningInterp = false;
	float LidInterpAlpha = 0.0f;

	void SetHumActive(bool bActive);
};
