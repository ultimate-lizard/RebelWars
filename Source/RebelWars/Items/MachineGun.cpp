#include "Items/MachineGun.h"

void AMachineGun::Tick(float DeltaTime)
{
	if (FirearmState == EFirearmState::Firing)
	{
		if (IsReadyForNextShot())
		{
			PrimaryFire();
			PlayPrimaryShotEffects();
		}
	}
}
