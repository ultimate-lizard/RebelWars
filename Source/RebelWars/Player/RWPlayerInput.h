// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerInput.h"
#include "RWPlayerInput.generated.h"

/**
 * 
 */
UCLASS()
class REBELWARS_API URWPlayerInput : public UPlayerInput
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Config)
	float MouseSensitivity;
};
