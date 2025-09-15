#include "pch.h"
#include "CShopManager.h"
#include "CHealthPotion.h"
#include "CAttackBoost.h"
#include "CMonsterLeather.h"
#include "CPlayer.h"

CShopManager::CShopManager()
{
    CShopManagerInit();
}

void CShopManager::CShopManagerInit()
{
    ShopCoin = 300;
    ResalePercent = 0.6f;
    ItemCnt = 5;

    // 상점에서 판매하는 아이템들 초기화
    if (Stuff.empty())
    {
        Stuff.insert({ ITEM_TYPE::HEALTH_POTION, new CHealthPotion("Health Potion", ItemCnt) });
        Stuff.insert({ ITEM_TYPE::ATTACK_BOOST, new CAttackBoost("Attack Boost", ItemCnt) });
    }

    // 재고 리셋
    for (auto item : Stuff)
    {
        item.second->SetCnt(ItemCnt);
    }
}

// 재고 조회
int CShopManager::GetItemCnt(ITEM_TYPE Item_t)
{
    if (Stuff.find(Item_t) == Stuff.end())
    {
        return -1;  // 상점에서 취급하지 않는 아이템
    }
    return Stuff[Item_t]->GetCnt();
}

unordered_map<ITEM_TYPE, string> CShopManager::GetItemList()
{
    unordered_map<ITEM_TYPE, string> list;
    for (auto it : Stuff)
    {
        list.insert({ it.first, it.second->GetName() });
    }
    return list;
}

// 플레이어로부터 아이템 구매 (상점이 사는 것)
int CShopManager::BuyItem(ITEM_TYPE Item_t)
{
    CItem* Item;
    int SalePrice;
    bool isSale = true;

    // 상점에서 판매하지 않는 아이템 (몬스터 가죽 등)
    if (Stuff.find(Item_t) == Stuff.end())
    {
        switch (Item_t)
        {
        case ITEM_TYPE::MONSTER_LEATHER:
            Item = new CMonsterLeather("Monster Leather", 0);
            isSale = false;
            break;
        default:
            return 0;  // 취급하지 않는 아이템
        }
        SalePrice = Item->GetPrice();
        delete Item;
    }
    // 상점에서 판매하는 아이템 (재판매 가격으로)
    else
    {
        Item = Stuff[Item_t];
        SalePrice = round(Item->GetPrice() * ResalePercent);
    }

    if (ShopCoin - SalePrice < 0)
    {
        return 0;  // 구매 불가
    }

    ShopCoin -= SalePrice;

    if (isSale)
    {
        Item->IncreaseCnt();
    }

    return SalePrice;
}

// 플레이어에게 아이템 판매 (상점이 파는 것)
bool CShopManager::SellItem(ITEM_TYPE Item_t)
{
    CItem* Item = Stuff[Item_t];
    if (Item->GetCnt() == 0)
    {
        return false;  // 재고 부족
    }

    ShopCoin += Item->GetPrice();
    Item->ReduceCnt();

    return true;
}