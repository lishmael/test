#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <string>
#include <algorithm>
#include <fstream>

class ItemListHandler
{
private:
	// 
	class Autolock {
	private:
		std::mutex* mLock;
		Autolock(const Autolock&);
		Autolock& operator=(const Autolock&);
	public:
		Autolock(std::mutex* Mutex);
		~Autolock();
	};
	
    ItemListHandler(const ItemListHandler&);
    ItemListHandler& operator=(const Autolock&);

	// Locks
	std::mutex mResultLock;
	std::mutex mQueueLock;
    std::mutex mThreadOpLock;
    std::mutex mLogFileLock;

	// Threads
	std::list<std::thread*> m_pActiveThreads;
	
	// State
	bool mAllStop;
    int mProcessingActive;
    
    // Input queue
    std::list<std::wstring> mQueue;

	// Result
	std::list<std::wstring> mResult;
	std::wofstream mOutLogger;	
	// Processing
	void process();	
	
public:
	void addItemToProcess(const std::wstring& sItem); // adds new item to process
	
	std::list<std::wstring> getResults(); // waits and returns full results
										// note: doesn't cleans previous results
	void cleanResults(); // removes contents of mResult
	
	ItemListHandler(const std::wstring aLogFileName);
	~ItemListHandler(void);
};
