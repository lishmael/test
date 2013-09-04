#include "ItemListHandler.h"

ItemListHandler::ItemListHandler(std::wstring aLogFileName) : mAllStop(false), mLogFileName(aLogFileName) {
    mOutLogger.open(mLogFileName, std::ofstream::out | std::ofstream::append);

    unsigned int hw_threads = std::thread::hardware_concurrency();
    
    if (!hw_threads) hw_threads = 1;

    for (int i = 0; i < hw_threads; ++i) {
        m_pActiveThreads.insert(new std::thread(&ItemListHandler::process, this));

}

ItemListHandler::~ItemListHandler(void) {
    mAllStop = true;
    
    for (auto p_Thread : m_pActiveThreads) {
        p_Thread->join();
        delete p_Thread();
    }
}

void ItemListHandler::process() {
    while (!mAllStop) {
        if (!m_lockQueue.try_lock()) {
             continue;
        }
            if (!m_lockOperation.try_lock() || !mQueue.size()) {
                m_lockQueue.unlock();
                continue;
            }
            
                ++mProcessingActive;
                const std::wstring& sArg = mQueue.front();
                mQueue.pop_front();

            m_lockOperation.unlock();
    
        m_lockQueue.unlock();
    
        std::ifstream inFile(sArg.c_str(), 
                             std::ifstream::in | std::ifstream::binary);
        if (inFile.is_open()) {
            unsigned long lSum = 0, lSize = 0;
            char c;
            for (; inFile; inFile.read(&c, sizeof(char))) {
                if (inFile.good()) {
                    lSum += (unsigned long)c;
                    ++lSize;
                }
            }
            inFile.close();

            std::wstring sTmpResult = sArg + L", size: " + std::to_wstring(lSize) 
                + L"b, sum: " + std::to_wstring(lSum);

            {
                std::lock_guard<std::mutex> _logLock(m_lockLoggingk);
                mOutLogger << sTmpResult << std::endl;
            } 
            
            std::lock_guard<std::mutex> _resLock(m_lockResult);
            mResult.push_back(sTmpResult);
        }
        std::lock_guard<std::mutex> _lock(m_lockOperation);
        --mProcessingActive;
    }
}

void ItemListHandler::addItemToProcess(const std::wstring& sItem)
{
	std::lock_guard<std::mutex> _lock(m_lockQueue);
    mQueue.push_back(sItem);	
}

std::list<std::wstring> ItemListHandler::getResults()
{
    while (true) {
        if (!mProcessingActive && m_lockQueue.try_lock() && 
            m_lockOperation.try_lock && m_lockResult.try_lock()) {
            break;
        }
    }
	
    auto res = mResult;
    res.sort();
    
    m_lockQueue.unlock();
    m_lockOperation.unlock();
    m_lockResult.unlock();

	return res;
}

void ItemListHandler::cleanResults() {
	std::lock_guard<std::mutex> _lock(m_lockResult);
	mResult.clear();
}

