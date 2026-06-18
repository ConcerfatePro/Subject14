#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Subject14SimpleDoorActor.generated.h"

class USceneComponent;
class USoundBase;
class UStaticMeshComponent;

/** Hinged door: E toggles open/closed (interp on yaw). */
UCLASS(Blueprintable)
class SUBJECT_14_API ASubject14SimpleDoorActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASubject14SimpleDoorActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Subject14Interact_Implementation(AActor* Instigator) override;

	/** Yaw applied to the door panel when fully open (degrees, typically negative). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14", meta = (ClampMin = "-120.0", ClampMax = "120.0"))
	float OpenAngleDegrees = -88.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14", meta = (ClampMin = "0.5", ClampMax = "20.0"))
	float DoorInterpSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	TObjectPtr<USoundBase> DoorOpenSound;

	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Audio")
	TObjectPtr<USoundBase> DoorCloseSound;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14")
	TObjectPtr<UStaticMeshComponent> DoorPanel;

private:
	float CurrentYawDegrees = 0.0f;
	float TargetYawDegrees = 0.0f;
	bool bDoorOpen = false;
};
