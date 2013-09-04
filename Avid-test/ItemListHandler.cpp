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
        if (!mQueueLock.try_lock()) {
             continue;
        }
            if (!mThreadOpLock.try_lock() || !mQueue.size()) {
                mQueueLock.unlock();
                continue;
            }
            
                ++mProcessingActive;
                const std::wstring& sArg = mQueue.front();
                mQueue.pop_front();

            mThreadOpLock.unlock();
    
        mQueueLock.unlock();
    
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
                Autolock _logLock(&mOutFileLock);
                mOutLogger << sTmpResult << std::endl;
            } 
            
            Autolock _resLock(&mResultLock);
            mResult.push_back(sTmpResult);
        }
        Autolock _lock(&mThreadOpLock);
        --mProcessingActive;
    }
}

void ItemListHandler::addItemToProcess(const std::wstring& sItem)
{
	Autolock _lock(&mQueueLock);
    mQueue.push_back(sItem);	
}

std::list<std::wstring> ItemListHandler::getResults()
{
    while (true) {
        if (!mProcessingActive && mQueueLock.try_lock() && 
            mThreadOpLock.try_lock && mResultLock.try_lock()) {
            break;
        }
    }
	
    auto res = mResult;
    res.sort();
    
    mQueueLock.unlock();
    mThreadOpLock.unlock();
    mResultLock.unlock();

	return res;
}

void ItemListHandler::cleanResults() {
	Autolock _lock(&mResultLock);
	mResult.clear();
}

ItemListHandler::Autolock::Autolock(std::mutex* Mutex) {
	mLock = Mutex;
	mLock->lock();
}

ItemListHandler::Autolock::~Autolock() {
	mLock->unlock();
}
