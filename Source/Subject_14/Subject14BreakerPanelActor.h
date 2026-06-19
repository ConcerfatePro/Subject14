#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Subject14StoryTypes.h"
#include "Subject14BreakerPanelActor.generated.h"

class ASubject14CabinHatchActor;
class USoundBase;
class UStaticMeshComponent;
class USubject14StorySubsystem;

/**
 * Facility breaker — restores hatch utility power (story flag HatchHasPower).
 * Does not advance macro phase by default; hatch unlock / breach remain separate beats.
 */
UCLASS(Blueprintable)
class SUBJECT_14_API ASubject14BreakerPanelActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASubject14BreakerPanelActor();

	virtual void BeginPlay() override;
	virtual void Subject14Interact_Implementation(AActor* Instigator) override;

protected:
	bool EvaluateGate(const USubject14StorySubsystem& Subsystem) const;
	bool IsInstigatorPrimarySinglePlayerPawn(AActor* Instigator) const;
	bool IsPermanentlyConsumed(const USubject14StorySubsystem& Subsystem) const;
	void ApplyPower(AActor* Instigator);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14|Breaker")
	TObjectPtr<UStaticMeshComponent> PanelMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Gate")
	FName RequiredStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Gate", meta = (ToolTip = "If set, panel is inert while this flag is true on the story subsystem."))
	FName BlockedByStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Gate")
	ESubject14StoryPhase RequiredPhaseAtLeast = ESubject14StoryPhase::IntroWake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Behavior")
	bool bOneShot = true;

	/** If not None, one-shot completion is tracked via this flag (set true on use). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Behavior")
	FName ConsumedStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Effects")
	FName GrantedStoryFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Effects")
	bool bSaveImmediatelyAfterUse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Hatches")
	TArray<TObjectPtr<ASubject14CabinHatchActor>> LinkedHatches;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Breaker|Audio")
	TObjectPtr<USoundBase> SwitchThrowSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Breaker|Audio")
	TObjectPtr<USoundBase> PowerOnHumSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Feedback", meta = (MultiLine = "true"))
	FString ThoughtOnUse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Feedback", meta = (MultiLine = "false"))
	FString ObjectiveAfterUse;

	/** Non-shipping: warns when the linked hatch list is empty (power flag still applies globally). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14|Breaker|Debug")
	bool bWarnIfNoLinkedHatches = true;

private:
	bool bLocalConsumed = false;
};
