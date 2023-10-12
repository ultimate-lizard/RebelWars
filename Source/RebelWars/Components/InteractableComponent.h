#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "InteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, AActor*, Initiator);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REBELWARS_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractableComponent();

	void Interact(AActor* Initiator);

	UPROPERTY(BlueprintAssignable)
	FOnInteract OnInteract;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowPrompt;

protected:
	virtual void BeginPlay() override;
};
