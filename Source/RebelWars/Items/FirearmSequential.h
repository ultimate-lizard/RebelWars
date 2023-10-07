#pragma once

#include "CoreMinimal.h"
#include "Items/Firearm.h"

#include "FirearmSequential.generated.h"

/* An automatic or semi-automatic weapon that has sequential reload, such as shotguns or bolt action rifles */
UCLASS()
class REBELWARS_API AFirearmSequential : public AFirearm
{
	GENERATED_BODY()
	
public:
	virtual void Reload(bool bFromReplication = false) override;

protected:

	virtual void StartSequentialReload();
	virtual void EndSequentialReload();
	virtual void LoadRound();
	virtual void OnRoundLoaded();
};
