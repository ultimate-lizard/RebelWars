#include "Player/RWPlayerCameraManager.h"

ARWPlayerCameraManager::ARWPlayerCameraManager() :
	Super()
{
	ViewPunchAlpha = 1.0f;
	ViewPunchAngle;
	bFinishedPunch = false;
}

void ARWPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	Super::UpdateCamera(DeltaTime);

	DecayViewPunch(DeltaTime);
}

void ARWPlayerCameraManager::AddViewPunch(FRotator InAngles)
{
	ViewPunchAngle += InAngles;
	
	FMinimalViewInfo POVInfo = GetCameraCachePOV();
	POVInfo.Rotation += InAngles;
	SetCameraCachePOV(POVInfo);
}

FRotator ARWPlayerCameraManager::GetCurrentViewPunchAngle() const
{
	return ViewPunchAngle;
}

void ARWPlayerCameraManager::DecayViewPunch(float DeltaTime)
{
	const float ViewPunchDecaySpeed = 5.0f;

	float Length = ViewPunchAngle.Vector().Size();
	Length -= ViewPunchDecaySpeed * DeltaTime;

	if (Length < 0.0f)
	{
		Length = 0.0f;
	}

	FRotator Old = ViewPunchAngle;
	ViewPunchAngle *= Length;

	FRotator Diff = Old - ViewPunchAngle * DeltaTime;

	FMinimalViewInfo POVInfo = GetCameraCachePOV();
	POVInfo.Rotation -= Diff * 0.05f;
	SetCameraCachePOV(POVInfo);
}
