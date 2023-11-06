#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "AITacticsBase.generated.h"

class ACombatAIController;

UENUM()
enum class EReaction : uint8
{
	Reaction_None,
	Reaction_LowHealth,
	Reaction_HasTarget,
	Reaction_Reload,
};

USTRUCT()
struct FReaction
{
	GENERATED_BODY()

	EReaction ReactionType;
	int32 SkillRequired = 0;
	int32 AggressionRequired = 0;
};

UCLASS()
class REBELWARS_API UAITacticsBase : public UObject
{
	GENERATED_BODY()
	
public:
	UAITacticsBase() {};
	UAITacticsBase(ACombatAIController* InAIController)
	{
		AIController = InAIController;
	}

	virtual bool CanExecute() const { return false; }

	virtual void Execute() {}

	void SetAIController(ACombatAIController* InAIController)
	{
		AIController = InAIController;
	}

protected:
	ACombatAIController* AIController;
};
