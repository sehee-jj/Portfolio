#include "pch.h"
#include "CHealthPotion.h"
#include "CPlayer.h"

int CHealthPotion::GetAmount() const
{
    return HEAL_AMOUNT;
}

void CHealthPotion::Use()
{
    ReduceCnt();
    CPlayer::GetInst()->Heal(HEAL_AMOUNT);
}
