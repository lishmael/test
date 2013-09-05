#include "ItemListHandler.h"

ItemListHandler::ItemListHandler(std::string aLogFileName) : 
    mAllStop(false),
    mLogFileName(aLogFileName),
    m_cElemToProcess(-1),
    m_cActiveProcessing(0),
    m_cNeedsProcessing(0) {
	mOutLogger.open(mLogFileName, std::ofstream::out | std::ofstream::app);

    unsigned int hw_threads = std::thread::hardware_concurrency();
    
    if (!hw_threads) hw_threads = 1;

    for (int i = 0; i < hw_threads; ++i) {
		m_pActiveThreads.push_back(new std::thread(&ItemListHandler::process, this));
    }
}

ItemListHandler::~ItemListHandler() {
    mAllStop = true;
    
    for (auto p_Thread : m_pActiveThreads) {
        p_Thread->join();
        delete p_Thread;
    }

    mOutLogger.close();
}

void ItemListHandler::process() {
    while (!mAllStop) {
        m_lockOperation.lock();
        while (!m_cNeedsProcessing) {
            m_cvNewItem.wait_for(m_lockOperation, std::chrono::milliseconds(500));
            if (mAllStop) return;
        }

        if (mQueue.size() <= m_cElemToProcess) {
                m_lockOperation.unlock();
                continue;
        }

        std::wstring sArg = mQueue[m_cElemToProcess];
        --m_cNeedsProcessing;
        ++m_cElemToProcess;
        ++m_cActiveProcessing;
        
        m_lockOperation.unlock();
    
        std::ifstream inFile(sArg,
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

            HANDLE hFile = CreateFileW(sArg.c_str(),
                                        GENERIC_READ,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);
            
            std::wstring tmp_sRes = L"";
            tmp_sRes += sArg.substr(sArg.find_last_of(L"/\\") + 1) + L" ";
            
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

            std::lock_guard<std::mutex> _resLock(m_lockResult);
            mResult.insert(tmp_sRes);
            if (mOutLogger.is_open()) {
                mOutLogger << tmp_sRes << L"\r\n";    
                mOutLogger.flush();
            }
        }
        std::lock_guard<std::mutex> _opLock(m_lockOperation);
        
        --m_cActiveProcessing;
        m_cvResult.notify_all();
    }
}

void ItemListHandler::addItemToProcess(std::wstring sItem) {
    std::lock(m_lockQueue, m_lockOperation);
    
    mQueue.push_back(sItem);
    ++m_cNeedsProcessing;
    if (m_cElemToProcess < 0) m_cElemToProcess = 0;
    
    m_lockQueue.unlock();
    m_lockOperation.unlock();

    m_cvNewItem.notify_one();
}

std::set<std::wstring> ItemListHandler::getResults() const
{
    std::unique_lock<std::mutex> _lockQ(m_lockQueue);
    while (m_cActiveProcessing || m_cNeedsProcessing) {
        m_cvResult.wait_for(_lockQ, std::chrono::milliseconds(100));
    } 
    std::lock_guard<std::mutex> _lockR(m_lockResult);

    return mResult;
}

