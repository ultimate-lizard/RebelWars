#include "Controllers/HumanPlayerController.h"

#include <Characters/CombatCharacter.h>

void AHumanPlayerController::AddViewPunch(FRotator InAngles)
{
	ViewPunchAngle += InAngles;
	SetControlRotation(GetControlRotation() + ViewPunchAngle);
}

void AHumanPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//float Old = ViewPunchAngle.Pitch;
	//ViewPunchAngle.Pitch -= 10 * DeltaTime;
	//if (ViewPunchAngle.Pitch < 0.0f)
	//{
	//	ViewPunchAngle.Pitch = 0.0f;
	//}

	//float Diff = Old - ViewPunchAngle.Pitch;

	//UE_LOG(LogTemp, Log, TEXT("%f"), Diff);

	//SetControlRotation(GetControlRotation() - FRotator(Diff, 0.0f, 0.0f));

	float Length = ViewPunchAngle.Vector().Size();
	Length -= 5.0f * DeltaTime;

	if (Length < 0.0f)
	{
		Length = 0.0f;
	}

	FRotator Old = ViewPunchAngle;
	ViewPunchAngle *= Length;

	FRotator Diff = Old - ViewPunchAngle;

	SetControlRotation(GetControlRotation() - Diff);
}

void AHumanPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

}
