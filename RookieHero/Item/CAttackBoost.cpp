#include "pch.h"
#include "CAttackBoost.h"
#include "CPlayer.h"

int CAttackBoost::GetAmount() const
{
    return BOOST_AMOUNT;
}

void CAttackBoost::Use()
{
    ReduceCnt();
    CPlayer::GetInst()->IncreaseDamage(BOOST_AMOUNT);
}