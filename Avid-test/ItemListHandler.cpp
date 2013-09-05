#include "ItemListHandler.h"

ItemListHandler::ItemListHandler() : mState(QUEUED) {
}

ItemListHandler::~ItemListHandler() {
}

void ItemListHandler::process() {
    while (mState == PROCESSING_STATE::PROCESSING) {
        m_lockOperation.lock();
        if (m_iQueueProcesssing == mResult->end()) {
            mState = PROCESSING_STATE::READY;
            m_lockOperation.unlock();
            m_cvProcessingEnds.notify_all();
            break;
        }

        auto i_elementProcessed = m_iQueueProcesssing;
        ++m_iQueueProcesssing;
        m_lockOperation.unlock();
        i_elementProcessed.second = std::pair<std::wstring, PROCESSING_STATE>("", PROCESSING_STATE::PROCESSING);
        std::ifstream inFile((*i_elementProcessed).first,
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

            HANDLE hFile = CreateFileW((*i_elementProcessed).first.c_str(),
                                        GENERIC_READ,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);
            
            std::wstring tmp_sRes = L"";
            tmp_sRes += (*i_elementProcessed).first.substr((*i_elementProcessed).first.find_last_of(L"/\\") + 1) + L" ";
            
            if (hFile != INVALID_HANDLE_VALUE) {
                FILETIME tmp_fileTime, tmp_localFileTime;
                SYSTEMTIME tmp_sysTime;
                GetFileTime(hFile, &tmp_fileTime, NULL, NULL);
                FileTimeToLocalFileTime(&tmp_fileTime,  &tmp_localFileTime);
                FileTimeToSystemTime(&tmp_localFileTime, &tmp_sysTime);
            
                tmp_sRes += 
                    L"Created: " + std::to_wstring(tmp_sysTime.wDay) +
                    L"." + std::to_wstring(tmp_sysTime.wMonth) + 
                    L"." + std::to_wstring(tmp_sysTime.wYear) + 
                    L" " + std::to_wstring(tmp_sysTime.wHour) + 
                    L":" + (tmp_sysTime.wMinute >= 10 ? L"" : L"0") +
                    std::to_wstring(tmp_sysTime.wMinute) + 
                    L"; ";
                CloseHandle(hFile);
            }

            tmp_sRes += L"Size: ";
            std::wstring sizes[] = {L"B", L"KiB", L"MiB", L"GiB"};
            long muls[] = { 1, 1024, 1048576, 1073742824 };
            
            for (int i = 3; i >= 0; --i) {
                if (lSize / muls[i] != 0) {
                    unsigned int tmp_iSizePart = lSize / muls[i];
                    lSize = (unsigned long long)lSize % muls[i];
                    tmp_sRes += std::to_wstring(tmp_iSizePart) + sizes[i] + L" ";
                }
            }
            tmp_sRes += L"; ";

            *i_elementProcessed.second = std::pair<std::wstring, ItemListHandler::PROCESSING_STATE>(tmp_sRes, PROCESSING_STATE::READY);
        }
        
        m_cvResult.notify_all();
    }
}

bool ItemListHandler::addQueueToProcess(std::map<std::wstring, std::wstring>* queue) {
    if (mState == PROCESSING_STATE::PROCESSING) return false;
    
    mResult = queue;
    m_iQueueProcesssing = mResult->begin();
    return true;
}

void ItemListHandler::processQueue() {
    mState = PROCESSING_STATE::PROCESSING;
    start();
} 

void ItemListHandler::start() {
    unsigned int hw_threads = std::thread::hardware_concurrency();
    
    if (!hw_threads) hw_threads = 1;

    for (int i = 0; i < hw_threads; ++i) {
		m_pActiveThreads.push_back(new std::thread(&ItemListHandler::process, this));
    }

    processAndEnd(); 
}

void ItemListHandler::processAndEnd() {
    std::unique_lock<std::mutex> _end(m_lockOperation);
    m_cvProcessingEnds.wait(_end);

    mState = PROCESSING_STATE::READY;

    for (auto p_Thread : m_pActiveThreads) {
        p_Thread->join();
        delete p_Thread;
    }
}

bool ItemListHandler::isReady() const
{
    return mState == PROCESSING_STATE::READY;
}

