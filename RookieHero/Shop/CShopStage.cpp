#include "pch.h"
#include "CShopStage.h"
#include "CStage.h"
#include "CStageManager.h"
#include "CKeyManager.h"
#include "CVillageStage.h"
#include "CItem.h"
#include "CShopManager.h"
#include "CStateStage.h"
#include <Windows.h>

void CShopStage::StageInit()
{
    bCallRender = true;
    CurPlayer_Mode = BUYSELL_MODE::BUY;
    CurrItem = 0;
    ColorOfNotify = 7;
    Notification = "";

    // 상점 및 플레이어 정보 초기화
    shopPotionCnt = CShopManager::GetInst()->GetItemCnt(ITEM_TYPE::HEALTH_POTION);
    shopBoostCnt = CShopManager::GetInst()->GetItemCnt(ITEM_TYPE::ATTACK_BOOST);
    PlayerPotionCnt = CPlayer::GetInst()->GetItemCnt(ITEM_TYPE::HEALTH_POTION);
    PlayerBoostCnt = CPlayer::GetInst()->GetItemCnt(ITEM_TYPE::ATTACK_BOOST);
    PlayerMonLeatherCnt = CPlayer::GetInst()->GetItemCnt(ITEM_TYPE::MONSTER_LEATHER);
    PlayerGold = CPlayer::GetInst()->GetGold();

    ItemList = CShopManager::GetInst()->GetItemList();
}

void CShopStage::StageTick()
{
    if (bCallRender)
    {
        StageRender();
        bCallRender = false;
    }

    // 키 입력 처리
    if (CKeyManager::GetInst()->GetKeyState(KEY_TYPE::UP) == KEY_STATE::TAP)
    {
        CurrItem = (CurrItem - 1 + ItemList.size()) % ItemList.size();
        bCallRender = true;
    }
    else if (CKeyManager::GetInst()->GetKeyState(KEY_TYPE::DOWN) == KEY_STATE::TAP)
    {
        CurrItem = (CurrItem + 1) % ItemList.size();
        bCallRender = true;
    }
    else if (CKeyManager::GetInst()->GetKeyState(KEY_TYPE::LEFT) == KEY_STATE::TAP ||
        CKeyManager::GetInst()->GetKeyState(KEY_TYPE::RIGHT) == KEY_STATE::TAP)
    {
        ChangeBUYSELL();
        bCallRender = true;
    }
    else if (CKeyManager::GetInst()->GetKeyState(KEY_TYPE::SPACE) == KEY_STATE::TAP)
    {
        // 구매/판매 실행
        if (CurPlayer_Mode == BUYSELL_MODE::BUY)
        {
            ITEM_TYPE currentItemType = static_cast<ITEM_TYPE>(CurrItem);
            Notification = CPlayer::GetInst()->BuyItem(currentItemType);
            ColorOfNotify = Notification.empty() ? 2 : 4; // 성공시 초록, 실패시 빨강
        }
        else
        {
            ITEM_TYPE currentItemType = static_cast<ITEM_TYPE>(CurrItem);
            Notification = CPlayer::GetInst()->SellItem(currentItemType);
            ColorOfNotify = Notification.empty() ? 2 : 4;
        }

        // 정보 업데이트
        shopPotionCnt = CShopManager::GetInst()->GetItemCnt(ITEM_TYPE::HEALTH_POTION);
        shopBoostCnt = CShopManager::GetInst()->GetItemCnt(ITEM_TYPE::ATTACK_BOOST);
        PlayerPotionCnt = CPlayer::GetInst()->GetItemCnt(ITEM_TYPE::HEALTH_POTION);
        PlayerBoostCnt = CPlayer::GetInst()->GetItemCnt(ITEM_TYPE::ATTACK_BOOST);
        PlayerMonLeatherCnt = CPlayer::GetInst()->GetItemCnt(ITEM_TYPE::MONSTER_LEATHER);
        PlayerGold = CPlayer::GetInst()->GetGold();

        bCallRender = true;
    }
    else if (CKeyManager::GetInst()->GetKeyState(KEY_TYPE::ESC) == KEY_STATE::TAP)
    {
        // 마을로 돌아가기
        CStageManager::GetInst()->ChangeStage(new CVillageStage);
    }
}

void CShopStage::StageRender()
{
    // ASCII Art 기반 상점 UI 렌더링
    // ... 복잡한 UI 렌더링 코드 생략 ...

    CurrItemRender(); // 현재 선택된 아이템 표시

    // 구매/판매 모드에 따른 렌더링
    if (CurPlayer_Mode == BUYSELL_MODE::BUY)
    {
        BuyRenderPotion();
        BuyRenderBooster();
    }
    else
    {
        SellRenderPotion();
        SellRenderBooster();
        SellRenderMonLeather();
    }
}

// 구매/판매 모드 변경
void CShopStage::ChangeBUYSELL()
{
    CurPlayer_Mode = (CurPlayer_Mode == BUYSELL_MODE::BUY) ? BUYSELL_MODE::SELL : BUYSELL_MODE::BUY;
    CurrItem = 0;
    Notification = "";
    ColorOfNotify = 7;
}

// 유틸리티 함수들
void CShopStage::setConsoleColor(WORD color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void CShopStage::SpaceMaker(string& _str, int _max)
{
    int start = _str.size();
    for (int i = start; i < _max; i++)
    {
        _str += " ";
    }
}

// ... 각 아이템별 렌더링 함수들 구현 생략 ...