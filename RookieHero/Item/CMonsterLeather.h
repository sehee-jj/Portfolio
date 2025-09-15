#pragma once
#include "CItem.h"

// 몬스터 가죽 재료 아이템 (판매 전용)
class CMonsterLeather : public CItem
{
public:
    CMonsterLeather(string _Name, int _Cnt) : CItem(_Name, _Cnt)
    {
        SetPrice(5);
    }

    // 사용 불가 아이템 (판매 전용)
    virtual void Use() override;
};
