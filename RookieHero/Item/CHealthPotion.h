#pragma once
#include "CItem.h"

// 체력 회복 포션 아이템
class CHealthPotion : public CItem
{
private:
    static const int HEAL_AMOUNT = 50;  // 회복량

public:
    CHealthPotion(string _Name, int _Cnt) : CItem(_Name, _Cnt)
    {
        SetPrice(10);
    }

    // 회복량 조회
    int GetAmount() const;

    // 포션 사용 (체력 회복)
    virtual void Use() override;
};