#pragma once

#include <string>
#include <fstream>
#include <windows.h>

class Item {
public:
    enum ITEM_STATE { RAW, PROCESSED, READY, BAD };

    Item(std::wstring wsFullPath);
    ~Item();

    void process(); // process file and collect stats

    bool isReady() const;
    ITEM_STATE getState() const;
    std::wstring getStat() const; // get file stats
    std::wstring getFullPath() const;

private:
    std::wstring m_wsFullPath;
    std::wstring m_wsFileName;
    std::wstring m_wsChecksum;
    std::wstring m_wsSize;
    std::wstring m_wsCreationDate;
    ITEM_STATE mState;
};
