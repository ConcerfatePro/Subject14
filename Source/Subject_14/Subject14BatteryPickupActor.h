#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Subject14BatteryPickupActor.generated.h"

class UStaticMeshComponent;
class USoundBase;

/** World pickup: press E while looking at it to add flashlight charge to the instigating player. */
UCLASS(Blueprintable)
class SUBJECT_14_API ASubject14BatteryPickupActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASubject14BatteryPickupActor();

	virtual void Subject14Interact_Implementation(AActor* Instigator) override;

	/** Charge added to the player's flashlight (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChargeFraction = 0.35f;

	/** If false, interaction does nothing while the player's battery is already full. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14")
	bool bAllowPickupWhenBatteryFull = false;

	/** Remove this actor after a successful pickup (after PickupFxHoldSeconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14")
	bool bDestroyAfterPickup = true;

	/** Time to show the “spent” mesh before destroy/hide. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float PickupFxHoldSeconds = 0.42f;

	/** Defaults to /Game/Audio/BatteryPickup when imported; override per instance if needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14")
	TObjectPtr<USoundBase> PickupSound;

	/** Optional spent shell mesh; defaults to a small engine cube. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14")
	TObjectPtr<UStaticMesh> SpentPickupMesh;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	bool bConsumed = false;

	FTimerHandle PickupCleanupTimerHandle;

	UFUNCTION()
	void HandlePickupCleanupTimer();

	void PlayPickupFeedback();
	void ApplySpentPickupVisual();
};
