#pragma once

// 상점 관리 시스템
class CShopManager : public CSingleton<CShopManager>
{
private:
    int ShopCoin;           // 상점 보유 골드
    int ItemCnt;            // 아이템 기본 재고량
    float ResalePercent;    // 재판매 비율
    unordered_map<ITEM_TYPE, class CItem*> Stuff;  // 상점 재고

public:
    CShopManager();

    // 상점 초기화
    void CShopManagerInit();

    // 재고 조회
    int GetItemCnt(ITEM_TYPE);
    unordered_map<ITEM_TYPE, string> GetItemList();

    // 거래 처리
    int BuyItem(ITEM_TYPE);     // 플레이어로부터 구매 (상점이 사는 것)
    bool SellItem(ITEM_TYPE);   // 플레이어에게 판매 (상점이 파는 것)
};