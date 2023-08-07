#pragma once

#include "CoreMinimal.h"
#include "Items/Firearm.h"

#include "Pistol.generated.h"

UCLASS()
class REBELWARS_API APistol : public AFirearm
{
	GENERATED_BODY()
	
public:
	virtual void PrimaryFire() override;
};
