// CPlayer 클래스 중 담당 기능 구현 부분
// 전체 캐릭터 시스템에서 인벤토리와 상점 거래 기능을 담당

#include "pch.h"
#include "CPlayer.h"
#include "CItem.h"
#include "CHealthPotion.h"
#include "CAttackBoost.h"
#include "CMonsterLeather.h"
#include "CShopManager.h"

CPlayer::CPlayer()
{
    // ... 다른 초기화 코드 생략 ...

    Gold = 10;
    Inventory.insert({ ITEM_TYPE::HEALTH_POTION, new CHealthPotion("Health Potion", 0) });
    Inventory.insert({ ITEM_TYPE::ATTACK_BOOST, new CAttackBoost("Attack Boost", 0) });
    Inventory.insert({ ITEM_TYPE::MONSTER_LEATHER, new CMonsterLeather("Monster Leather", 0) });
}

// ... 다른 함수들 생략 ...

// 골드 관리
bool CPlayer::CanPayGold(int Price)
{
    return Gold >= Price;
}

void CPlayer::PayGold(int Price)
{
    Gold -= Price;
}

void CPlayer::ReceiveGold(int Price)
{
    Gold += Price;
}

// 인벤토리 조회
int CPlayer::GetItemCnt(ITEM_TYPE Item_t)
{
    if (Inventory.find(Item_t) == Inventory.end())
    {
        return -1;  // 아이템이 존재하지 않음
    }
    return Inventory[Item_t]->GetCnt();
}

unordered_map<ITEM_TYPE, string> CPlayer::GetItemList()
{
    unordered_map<ITEM_TYPE, string> list;
    for (auto it : Inventory)
    {
        list.insert({ it.first, it.second->GetName() });
    }
    return list;
}

string CPlayer::GetItemName(ITEM_TYPE _itemType)
{
    return Inventory[_itemType]->GetName();
}

// 아이템 사용
void CPlayer::UseItem(ITEM_TYPE Item_t)
{
    Inventory[Item_t]->Use();
}

void CPlayer::AddItem(ITEM_TYPE Item_t)
{
    Inventory[Item_t]->IncreaseCnt();
}

// 상점 거래
string CPlayer::BuyItem(ITEM_TYPE Item_t)
{
    int ItemPrice = Inventory[Item_t]->GetPrice();
    if (Gold - ItemPrice < 0)
    {
        return "You don't have enough gold !";
    }

    bool SellResult = CShopManager::GetInst()->SellItem(Item_t);
    if (!SellResult)
    {
        return "That item is out of stock !";
    }

    AddItem(Item_t);
    PayGold(ItemPrice);
    return "";
}

string CPlayer::SellItem(ITEM_TYPE Item_t)
{
    CItem* Item = Inventory[Item_t];
    if (Item->GetCnt() == 0)
    {
        return "You don't have that item !";
    }

    int SalePrice = CShopManager::GetInst()->BuyItem(Item_t);
    if (SalePrice == 0)
    {
        return "You can't sell any more !";
    }

    Item->ReduceCnt();
    ReceiveGold(SalePrice);
    return "";
}