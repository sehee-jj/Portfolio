#include "pch.h"
#include "CItem.h"

// 아이템 정보 조회
string CItem::GetName() const
{
    return Name;
}

int CItem::GetPrice() const
{
    return Price;
}

int CItem::GetCnt() const
{
    return Cnt;
}

// 아이템 속성 설정
void CItem::SetPrice(int _Price)
{
    Price = _Price;
}

void CItem::SetCnt(int _Cnt)
{
    Cnt = _Cnt;
}

// 수량 관리
void CItem::ReduceCnt()
{
    if (Cnt > 0) {
        Cnt--;
    }
}

void CItem::IncreaseCnt()
{
    Cnt++;
}
