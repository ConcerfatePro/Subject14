#include "Subject14GameMode.h"
#include "Subject14FirstPersonCharacter.h"
#include "Subject14StorySubsystem.h"

ASubject14GameMode::ASubject14GameMode()
{
	DefaultPawnClass = ASubject14FirstPersonCharacter::StaticClass();
}

void ASubject14GameMode::StartPlay()
{
	Super::StartPlay();

	if (USubject14StorySubsystem* const Story = USubject14StorySubsystem::Get(this))
	{
		if (Story->DoesProgressSlotExist())
		{
			Story->LoadProgressFromSlot();
		}
	}
}
