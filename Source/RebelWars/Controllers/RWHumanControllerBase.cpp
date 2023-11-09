#include "Controllers/RWHumanControllerBase.h"

#include "GameFramework/PlayerInput.h"

void ARWHumanControllerBase::BeginPlay()
{
	Super::BeginPlay();

	SetMouseSensitivity(GetMouseSensitivity());
}

float ARWHumanControllerBase::GetMouseSensitivity() const
{
	float Sensitivity = 0.1f;
	GConfig->GetFloat(TEXT("/Script/RebelWars.RWPlayerInput"), TEXT("MouseSensitivity"), Sensitivity, GInputIni);

	return Sensitivity;
}

void ARWHumanControllerBase::SetMouseSensitivity(float NewSensitivity)
{
	if (PlayerInput)
	{
		PlayerInput->SetMouseSensitivity(NewSensitivity);
	}

	GConfig->SetFloat(TEXT("/Script/RebelWars.RWPlayerInput"), TEXT("MouseSensitivity"), NewSensitivity, GInputIni);
}
