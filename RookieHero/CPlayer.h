#pragma once
// CPlayer 클래스 중 담당 기능 구현 부분
// 전체 캐릭터 시스템에서 인벤토리와 상점 거래 기능을 담당

// ... 다른 include, 전역 정의들 생략 ...

class CPlayer : public CSingleton<CPlayer>
{
    SINGLE(CPlayer)

private:
    // ... 다른 멤버 변수들 생략 ...

    int Gold;
    unordered_map<ITEM_TYPE, class CItem*> Inventory;

    // ... 다른 멤버 변수들 생략 ...

public:
    // ... 다른 함수들 생략 ...

    // 골드 관련
    int GetGold() { return Gold; }
    bool CanPayGold(int Price);
    void PayGold(int Price);
    void ReceiveGold(int Price);

    // 인벤토리 관련
    int GetItemCnt(ITEM_TYPE);
    unordered_map<ITEM_TYPE, string> GetItemList();
    string GetItemName(ITEM_TYPE);
    void UseItem(ITEM_TYPE);
    void AddItem(ITEM_TYPE);

    // 상점 거래
    string BuyItem(ITEM_TYPE);
    string SellItem(ITEM_TYPE);

    // ... 다른 함수들 생략 ...
};
