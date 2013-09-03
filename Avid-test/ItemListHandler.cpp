#include "ItemListHandler.h"

ItemListHandler::ItemListHandler(void) : mProcessingActive(0) {
	m_pCleanerThread = new std::thread(&ItemListHandler::threadCleanerLoop, this);
}

ItemListHandler::~ItemListHandler(void) {
	while (mProcessingActive) {}
	
	mProcessingActive = -1;

	mResult.clear();

	m_pCleanerThread->join();
	delete m_pCleanerThread;
}

void ItemListHandler::threadCleanerLoop() {
	while (mProcessingActive >= 0 && true) {
		if (!mThreadDoneIDs.empty()) {
			Autolock _lock(&mThreadOpLock);
			
			m_pActiveThreads[mThreadDoneIDs.front()]->join();
			delete m_pActiveThreads[mThreadDoneIDs.front()];
			m_pActiveThreads.erase(mThreadDoneIDs.front());

			mThreadDoneIDs.pop_front();
		}
	}
}


void ItemListHandler::process(const std::wstring& sArg) {
	mThreadOpLock.lock();
		++mProcessingActive;
	mThreadOpLock.unlock();

	std::ifstream inFile(sArg.c_str(), 
						 std::ifstream::in | std::ifstream::binary);
	if (inFile.is_open()) {
		unsigned long lSum = 0, lSize = 0;
		char* c = new char[1];
		for (; inFile; inFile.read(c, sizeof(char))) {
			if (inFile.good()) {
				lSum += (long)*c;
				++lSize;
			}
		}
		inFile.close();

		std::wstring sTmpResult = sArg + L", size: " + std::to_wstring(lSize) 
			+ L"b, sum: " + std::to_wstring(lSum);
	
		Autolock _resLock(&mResultLock);
		mResult.push_back(sTmpResult);
	}
	
	Autolock _procLock(&mThreadOpLock);
	--mProcessingActive;
	mThreadDoneIDs.push_back(std::this_thread::get_id());
}

void ItemListHandler::addItemToProcess(const std::wstring& sItem)
{
	std::thread* pThread = new std::thread(&ItemListHandler::process, this, sItem);
	
	Autolock _lock(&mThreadOpLock);
	
	m_pActiveThreads.insert(
		std::pair<std::thread::id,std::thread*>(pThread->get_id(), pThread));
}

std::list<std::wstring> ItemListHandler::getResults()
{
	while (mProcessingActive) {}
	
	Autolock _lock(&mResultLock);
	mResult.sort();
	return mResult;
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