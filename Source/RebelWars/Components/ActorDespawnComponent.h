#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "ActorDespawnComponent.generated.h"

class AActor;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REBELWARS_API UActorDespawnComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UActorDespawnComponent();

	void AddToDespawnQueue(AActor* ActorToAdd);
	void SetQueueLimit(uint32 NewLimit);
	void FreeDespawnSlot(AActor* ActorToFree);

private:
	static TArray<AActor*> DespawnQueue;
	int32 DespawnLimit;
};
