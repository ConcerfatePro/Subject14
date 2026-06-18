#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Subject14InteractableTestActor.generated.h"

class UStaticMeshComponent;

/** Simple cube you can trace and press E on; replace with Blueprint subclasses later. */
UCLASS(Blueprintable)
class SUBJECT_14_API ASubject14InteractableTestActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASubject14InteractableTestActor();

	virtual void Subject14Interact_Implementation(AActor* Instigator) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject14")
	FString PromptText;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subject14")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
