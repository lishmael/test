#pragma once

#include <string>
#include <windows.h>

class Item { 
public:
    enum ITEM_STATE { RAW, PROCESSED, READY, BAD };
    
    Item(std::wstring wsFullPath);
    ~Item();

    void process();

    bool isReady() const;
    ITEM_STATE getState() const;
    std::wstring getStat() const;
    std::wstring getFullPath() const;

private:
    std::wstring m_wsFullPath;
    std::wstring m_wsFileName;
    std::wstring m_wsChecksum;
    std::wstring m_wsSize;
    std::wstring m_wsCreationDate;
    ITEM_STATE mState;
};
