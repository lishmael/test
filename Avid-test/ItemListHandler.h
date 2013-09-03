#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <unordered_map>
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
	
	// Locks
	std::mutex mResultLock;
	std::mutex mThreadOpLock;
	
	// Threads
	std::thread* m_pCleanerThread;
	std::unordered_map<std::thread::id, std::thread*> m_pActiveThreads;
	
	// State
	std::list<std::thread::id> mThreadDoneIDs;
	int mProcessingActive;
	
	// Result
	std::list<std::wstring> mResult;
		
	// Processing
	void process(const std::wstring& sArg);	
	void threadCleanerLoop();
	
public:
	void addItemToProcess(const std::wstring& sItem); // adds new item to process
	
	std::list<std::wstring> getResults(); // waits and returns full results
										// note: doesn't cleans previous results
	void cleanResults(); // removes contents of mResult
	
	ItemListHandler(void);
	~ItemListHandler(void);
};
