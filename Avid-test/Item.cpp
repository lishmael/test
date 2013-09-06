#include "Item.h"

Item::Item(std::wstring wsFullPath) :
    m_wsFullPath(wsFullPath),
    m_wsFileName(L""),
    m_wsCreationDate(L""),
    m_wsSize(L""),
    m_wsChecksum(L""),
    mState(Item::ITEM_STATE::RAW) {
}

Item::~Item() {
}

bool Item::isReady() const {
    return mState == Item::ITEM_STATE::READY;
}

Item::ITEM_STATE Item::getState() const {
    return mState;
}

void Item::process() {
    if (isReady()) return;
    mState = ITEM_STATE::PROCESSED;

    std::ifstream inFile(m_wsFullPath.c_str(),
                         std::ifstream::in | std::ifstream::binary);
    if (inFile.is_open()) {
        unsigned long long lSum = 0, lSize = 0;
        char c = 0;
        for (; inFile; inFile.read(&c, sizeof(char))) {
            if (inFile.good()) {
                lSum += (unsigned long)c;
                ++lSize;
            }
        }
        inFile.close();

        m_wsChecksum = std::to_wstring(lSum);
            
        m_wsFileName = 
            m_wsFullPath.substr(m_wsFullPath.find_last_of(L"/\\") + 1);
        
        HANDLE hFile = CreateFile(m_wsFullPath.c_str(),
                                        GENERIC_READ,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);
            

        if (hFile != INVALID_HANDLE_VALUE) {
            FILETIME tmp_fileTime, tmp_localFileTime;
            SYSTEMTIME tmp_sysTime;
            GetFileTime(hFile, &tmp_fileTime, NULL, NULL);
            FileTimeToLocalFileTime(&tmp_fileTime,  &tmp_localFileTime);
            FileTimeToSystemTime(&tmp_localFileTime, &tmp_sysTime);
            
            m_wsCreationDate += 
                std::to_wstring(tmp_sysTime.wDay) +
                L"." + std::to_wstring(tmp_sysTime.wMonth) + 
                L"." + std::to_wstring(tmp_sysTime.wYear) + 
                L" " + std::to_wstring(tmp_sysTime.wHour) + 
                L":" + (tmp_sysTime.wMinute >= 10 ? L"" : L"0") +
                std::to_wstring(tmp_sysTime.wMinute);
            CloseHandle(hFile);
        }

            
        std::wstring sizes[] = {L"B", L"KiB ", L"MiB ", L"GiB "};
        long muls[] = { 1, 1024, 1048576, 1073742824 };
            
        for (int i = 3; i >= 0; --i) {
            if (lSize / muls[i] != 0) {
                unsigned int tmp_iSizePart = lSize / muls[i];
                lSize = (unsigned long long)lSize % muls[i];
                m_wsSize += std::to_wstring(tmp_iSizePart) + sizes[i];
            }
        }
        mState = ITEM_STATE::READY;
    } else {
        mState = ITEM_STATE::BAD;
    }
}

std::wstring Item::getStat() const {
    wchar_t sep[] = L"; ";
    return isReady() ? 
            (m_wsFileName + sep + 
                L"Size: " +  m_wsSize + sep +
                L"Created: " + m_wsCreationDate + sep + 
                L"Checksum: " + m_wsChecksum
            ) : L"";
}

std::wstring Item::getFullPath() const  {
    return m_wsFullPath;
}
