#pragma once
#include "CItem.h"

// 공격력 증가 포션 아이템
class CAttackBoost : public CItem
{
private:
    static const int BOOST_AMOUNT = 10;  // 공격력 증가량

public:
    CAttackBoost(string _Name, int _Cnt) : CItem(_Name, _Cnt)
    {
        SetPrice(15);
    }

    // 공격력 증가량 조회
    int GetAmount() const;

    // 포션 사용 (공격력 증가)
    virtual void Use() override;
};