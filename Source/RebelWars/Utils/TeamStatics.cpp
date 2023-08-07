#include "Utils/TeamStatics.h"

ETeamAttitude::Type UTeamStatics::SolveAttitudeImpl(FGenericTeamId TeamA, FGenericTeamId TeamB)
{
    TMap<TPair<uint8, uint8>, ETeamAttitude::Type> AttitudeMap
    {
        TPair<TPair<uint8, uint8>, ETeamAttitude::Type>(TPair<uint8, uint8>(1, 2), ETeamAttitude::Hostile),
        TPair<TPair<uint8, uint8>, ETeamAttitude::Type>(TPair<uint8, uint8>(2, 1), ETeamAttitude::Hostile),
    };

    if (ETeamAttitude::Type* Value = AttitudeMap.Find(TPair<uint8, uint8>(TeamA.GetId(), TeamB.GetId())))
    {
        return *Value;
    }

    return ETeamAttitude::Neutral;
}

uint8 UTeamStatics::GetTeamIdFromAffiliation(EAffiliation InAffiliation)
{
    return StaticCast<uint8>(InAffiliation);
}

EAffiliation UTeamStatics::GetTeamAffiliationFromTeamId(uint8 InTeamId)
{
    return StaticCast<EAffiliation>(InTeamId);
}
