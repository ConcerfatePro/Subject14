#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Subject14GameMode.generated.h"

UCLASS()
class SUBJECT_14_API ASubject14GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASubject14GameMode();

	virtual void StartPlay() override;
};
