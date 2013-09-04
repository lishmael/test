#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <string>
#include <algorithm>
#include <fstream>

// TODO
// rewrite result to be map
// so it autosorts

class ItemListHandler
{
private:
	// 

    ItemListHandler(const ItemListHandler&);
    ItemListHandler& operator=(const Autolock&);

	// Locks
	std::mutex m_lockResult;
	std::mutex m_lockQueue;
    std::mutex m_lockOperation;
    std::mutex m_lockLogging;

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
