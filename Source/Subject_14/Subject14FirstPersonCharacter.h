#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Subject14FlashlightBatteryWidget.h"
#include "Subject14FirstPersonCharacter.generated.h"

class UAudioComponent;
class UCameraComponent;
class USpotLightComponent;
class USoundBase;

UCLASS()
class SUBJECT_14_API ASubject14FirstPersonCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASubject14FirstPersonCharacter();

	/** Add charge for battery pickups / chargers (0 = none, 1 = full). */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Flashlight")
	void AddFlashlightBattery(float Delta01);

	UFUNCTION(BlueprintPure, Category = "Subject14|Flashlight")
	float GetFlashlightBatteryFraction() const { return FlashlightBattery01; }

	/** Multiplies designer AmbientBedVolume (scripted dips / recovery). */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Audio")
	void SetAmbientBedVolumeScalar(float Scalar);

	UFUNCTION(BlueprintCallable, Category = "Subject14|Audio")
	void SetAmbientBedPaused(bool bPaused);

	/** Replaces the looping bed asset and restarts it if the bed was enabled. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Audio")
	void SetAmbientBedSoundAndRestart(USoundBase* NewBedSound);

	/** Rapid visibility toggles on the flashlight beam (glitch / presence beats). */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Night")
	void RunFlashlightFlickerRoutine(int32 ToggleCount = 8, float StepSeconds = 0.045f);

	/** Blocks movement, look, interact, flashlight — used by Night 1 ending. */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Night")
	void SetNightSequenceInputBlocked(bool bBlocked);

	UFUNCTION(BlueprintPure, Category = "Subject14|Night")
	bool IsNightSequenceInputBlocked() const { return bNightSequenceInputBlocked; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

	void TryInteract();
	void ToggleFlashlight();
	void JumpPressed();
	void JumpReleased();

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Interaction")
	bool bShowInteractPrompt = true;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Story")
	bool bShowObjectiveHud = true;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14", meta = (ClampMin = "50.0", ClampMax = "800.0"))
	float InteractDistance = 384.0f;

	/** If false, flashlight starts off (good once you add a pickup flow). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight")
	bool bFlashlightStartsOn = true;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight", meta = (ClampMin = "100.0", ClampMax = "200000.0"))
	float FlashlightIntensityLumens = 12000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight", meta = (ClampMin = "5.0", ClampMax = "85.0"))
	float FlashlightOuterConeDegrees = 48.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight", meta = (ClampMin = "1.0", ClampMax = "80.0"))
	float FlashlightInnerConeDegrees = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight", meta = (ClampMin = "100.0", ClampMax = "200000.0"))
	float FlashlightAttenuationRadius = 6000.0f;

	/** Seconds the light can stay on from 100% to 0% (at default intensity curve). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight", meta = (ClampMin = "5.0", ClampMax = "3600.0"))
	float FlashlightBatteryDrainSecondsForFullDeplete = 120.0f;

	/** Starting charge when the level begins (0–1). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlashlightStartingBatteryFraction = 1.0f;

	/** If true, F toggles the beam but charge never drops (good for greybox). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight")
	bool bFlashlightInfiniteBattery = false;

	/** Legacy on-screen text (only if no UMG bar is active). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight")
	bool bShowFlashlightBatteryDebugHud = false;

	/** Screen-space battery bar (native widget; assign a BP subclass to reskin). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight")
	bool bUseBatteryUmWidget = true;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Flashlight")
	TSubclassOf<USubject14FlashlightBatteryWidget> FlashlightHudWidgetClass;

	/** Footsteps: velocity-based cadence while grounded (assign FootstepSound to hear anything). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	bool bEnableFootsteps = true;

	/**
	 * If non-empty, each footstep picks one sound at random (recommended with multiple one-shots).
	 * If empty, FootstepSound is used instead.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	TArray<TObjectPtr<USoundBase>> FootstepSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	TObjectPtr<USoundBase> FootstepSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio", meta = (ClampMin = "0.15", ClampMax = "1.5"))
	float FootstepBaseIntervalSeconds = 0.38f;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio", meta = (ClampMin = "0.08", ClampMax = "1.2"))
	float FootstepMinIntervalSeconds = 0.26f;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float FootstepVolumeMultiplier = 0.45f;

	/** Looped bed on the local player (assign AmbientBedSound). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	bool bEnableAmbientBed = true;

	/** Use a Sound Wave with “Looping” enabled, or a Sound Cue that loops. */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	TObjectPtr<USoundBase> AmbientBedSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AmbientBedVolume = 0.28f;

	/** 2D click when toggling the flashlight (local player only). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	TObjectPtr<USoundBase> FlashlightOnSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	TObjectPtr<USoundBase> FlashlightOffSound;

	/** Sparse warning while the light is on and charge is very low. */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	TObjectPtr<USoundBase> LowBatterySound;

private:
	UPROPERTY(VisibleAnywhere, Category = "Subject14")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, Category = "Subject14")
	TObjectPtr<USpotLightComponent> Flashlight;

	UPROPERTY(VisibleAnywhere, Category = "Subject14|Audio")
	TObjectPtr<UAudioComponent> AmbientBedAudio;

	bool bFlashlightOn = false;
	float FlashlightBattery01 = 1.0f;
	float BatteryHudAccumSec = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<USubject14FlashlightBatteryWidget> BatteryHudWidget;

	void MaybeUpdateBatteryHud(float DeltaSeconds);
	void ScheduleBatteryHudInit();
	void CreateBatteryHud();
	void DestroyBatteryHud();

	FTimerHandle BatteryHudRetryTimer;
	int32 BatteryHudRetryAttempts = 0;

	float FootstepAccumulator = 0.0f;

	/** After a low-battery chirp, cleared until charge rises enough to allow another. */
	bool bLowBatteryChirpArmed = true;

	void UpdateFootsteps(float DeltaSeconds);
	void MaybePlayLowBatteryWarning();
	void TryStartAmbientBed();
	void StopAmbientBed();
	void UpdateInteractPrompt();
	void UpdateObjectiveHud();

	void RefreshAmbientBedVolume();
	void FlickerTick();

	float AmbientBedVolumeScalar = 1.0f;
	bool bNightSequenceInputBlocked = false;

	FTimerHandle FlickerTimerHandle;
	int32 FlickerStepsRemaining = 0;
	bool bFlickerRestoreVisibility = false;
};
