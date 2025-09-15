#pragma once

// 게임 아이템의 기본 인터페이스
class CItem
{
private:
    string Name;
    int Cnt;
    int Price;

public:
    CItem(string _Name, int _Cnt) : Name(_Name), Cnt(_Cnt), Price(0) {}
    virtual ~CItem() = default;

    // 아이템 정보 조회
    string GetName() const;
    int GetPrice() const;
    int GetCnt() const;

    // 아이템 속성 설정
    void SetPrice(int _Price);
    void SetCnt(int _Cnt);

    // 수량 관리
    void ReduceCnt();
    void IncreaseCnt();

    // 아이템 사용 효과
    virtual void Use() = 0;
};