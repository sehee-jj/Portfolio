#pragma once
#include "CStage.h"
#include "CShopManager.h"
#include "CPlayer.h"

// 상점 스테이지 (구매/판매 UI)
class CShopStage : public CStage
{
public:
    enum BUYSELL_MODE
    {
        BUY,
        SELL
    };

private:
    bool bCallRender;
    BUYSELL_MODE CurPlayer_Mode;

    // 상점 재고 정보
    int shopPotionCnt;
    int shopBoostCnt;
    string zeroBUYPotionCnt;
    string zeroBUYBoostCnt;

    // 플레이어 인벤토리 정보
    int PlayerPotionCnt;
    int PlayerBoostCnt;
    int PlayerMonLeatherCnt;
    int PlayerGold;
    string zeroSELLPotionCnt;
    string zeroSELLBoostCnt;
    string zeroSELLMonLeatherCnt;
    string PlayerGoldforPrint;

    // UI 관련
    string Notification;
    unordered_map<ITEM_TYPE, string> ItemList;
    int CurrItem;
    int ColorOfNotify;

public:
    virtual void StageInit() override;
    virtual void StageTick() override;
    virtual void StageRender() override;

private:
    // UI 유틸리티
    void setConsoleColor(WORD color);
    void SpaceMaker(string& _str, int _max);

    // 모드 변경
    void ChangeBUYSELL();
    void CurrItemRender();

    // 아이템별 렌더링
    void BuyRenderPotion();
    void BuyRenderBooster();
    void SellRenderPotion();
    void SellRenderBooster();
    void SellRenderMonLeather();
};