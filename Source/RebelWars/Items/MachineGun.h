#pragma once

#include "CoreMinimal.h"
#include "Items/Firearm.h"

#include "MachineGun.generated.h"

UCLASS()
class REBELWARS_API AMachineGun : public AFirearm
{
	GENERATED_BODY()

protected:
	virtual void Tick(float DeltaTime) override;
};
