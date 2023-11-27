// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/RWPlayerState.h"

void ARWPlayerState::SetTeam(EAffiliation InTeam)
{
    Team = InTeam;
}

EAffiliation ARWPlayerState::GetTeam() const
{
    return Team;
}
