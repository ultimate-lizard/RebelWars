#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Despawnable.generated.h"

UINTERFACE(MinimalAPI)
class UDespawnable : public UInterface
{
	GENERATED_BODY()
};

class REBELWARS_API IDespawnable
{
	GENERATED_BODY()

public:

	UFUNCTION()
	virtual bool CanDespawn() const = 0;
};
