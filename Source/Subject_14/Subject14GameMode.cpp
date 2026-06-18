#include "Subject14GameMode.h"
#include "Subject14FirstPersonCharacter.h"

ASubject14GameMode::ASubject14GameMode()
{
	DefaultPawnClass = ASubject14FirstPersonCharacter::StaticClass();
}
