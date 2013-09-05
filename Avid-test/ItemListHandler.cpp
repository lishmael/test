#include "ItemListHandler.h"

ItemListHandler::ItemListHandler(std::map<t_mapKey, t_mapValue>::iterator begin,
                                 std::map<t_mapKey, t_mapValue>::iterator end) : 
    mState(PROCESSING_STATE::QUEUED),
    m_iEnd(end),
    m_iQueueProcesssing(begin), 
    m_iLog(begin) {
}

bool ItemListHandler::reQueue(std::map<t_mapKey, t_mapValue>::iterator begin,
                              std::map<t_mapKey, t_mapValue>::iterator end) {
    if (mState == PROCESSING_STATE::PROCESSING) return false;

    waitTillReady();

    m_iEnd = end;
    m_iQueueProcesssing = begin;
    m_iLog = begin;

    mState = PROCESSING_STATE::QUEUED;

    return true;
}

ItemListHandler::~ItemListHandler(void) 
{
    if (m_ofsLogger.is_open()) {
        m_ofsLogger.close();
    }
}

void ItemListHandler::invoke_processing() {
    while (mState == PROCESSING_STATE::PROCESSING) {
        m_lockOperation.lock();
        
        if (m_iQueueProcesssing == m_iEnd) {
            
            mState = PROCESSING_STATE::READY;
            m_lockOperation.unlock();
            
            m_cvProcessingEnds.notify_all();
            break;
        }

        auto i_elementProcessed = m_iQueueProcesssing;
        ++m_iQueueProcesssing;
        m_lockOperation.unlock();
        
        t_mapValue* item = &i_elementProcessed->second;
        item->stat();
        
        if (item->isReady())
        {
            
        }
        m_cvResult.notify_all();
    }
}

void ItemListHandler::log() {
    if (m_ofsLogger.is_open()) {
        m_ofsLogger.close();
    }
    
    if (0 != CreateDirectory(L"C:\\Avid-test\\", NULL) ||
        GetLastError() == ERROR_ALREADY_EXISTS) {
        m_ofsLogger.open(L"C:\\Avid-test\\avid-test.log",
            std::ofstream::out | std::ofstream::trunc);
        if (m_ofsLogger.is_open()) {
            m_ofsLogger << L"-------Begin log file" << std::endl;
        }
    }

    for (; m_iLog != m_iEnd; ++m_iLog) {
        t_mapValue* item = &m_iLog->second;
        while (!item->isReady()) {
            std::unique_lock<std::mutex> _ul(m_lockOperation);
            m_cvResult.wait_for(_ul, std::chrono::milliseconds(100));
        }

        m_sRes += item->getStat() + L"\n";
        if (m_ofsLogger.is_open()) {
            m_ofsLogger << item->getStat() + L"\n";
            m_ofsLogger.flush();
        }
    }
}

void ItemListHandler::process() {
    if (isReady()) return;
    mState = PROCESSING_STATE::PROCESSING;
    start();
    log();
} 

void ItemListHandler::start() {
    unsigned int hw_threads = std::thread::hardware_concurrency();
    
    if (!hw_threads) hw_threads = 1;

    for (int i = 0; i < hw_threads; ++i) {
        m_pActiveThreads.push_back(new std::thread(&ItemListHandler::invoke_processing, this));
    }
}

void ItemListHandler::waitTillReady() {
    while (mState != PROCESSING_STATE::READY) {
        std::unique_lock<std::mutex> _end(m_lockOperation);
        m_cvProcessingEnds.wait_for(_end, std::chrono::milliseconds(500));
    }
    for (auto p_Thread : m_pActiveThreads) {
        p_Thread->join();
        delete p_Thread;
    }

    if (m_ofsLogger.is_open()) {
        m_ofsLogger.close();
    }
}

bool ItemListHandler::isReady() const
{
    return mState == PROCESSING_STATE::READY;
}

std::wstring ItemListHandler::getResult() {
    return isReady() ? m_sRes : L"";
}

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

void Item::stat() {
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