#include "Subject14InteractableTestActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14InteractableTestActor)

ASubject14InteractableTestActor::ASubject14InteractableTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		Mesh->SetStaticMesh(CubeAsset.Object);
	}

	Mesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.5f));
	PromptText = TEXT("Subject 14 test interactable");
}

void ASubject14InteractableTestActor::Subject14Interact_Implementation(AActor* Instigator)
{
	UE_LOG(LogTemp, Log, TEXT("Subject14Interact: %s"), *PromptText);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			(uint64)((PTRINT)this),
			2.0f,
			FColor::Green,
			FString::Printf(TEXT("Interact: %s"), *PromptText));
	}
}
