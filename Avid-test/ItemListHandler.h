#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <windows.h>
#include <condition_variable>

// TODO
// rewrite result to be map
// so it autosorts

class ItemListHandler
{
private:
	 

    ItemListHandler(const ItemListHandler&);
	ItemListHandler& operator=(const ItemListHandler&);

	// Locks
	mutable std::mutex m_lockResult;
	mutable std::mutex m_lockQueue;
    mutable std::mutex m_lockOperation;
    mutable std::mutex m_lockLogging;
    mutable std::condition_variable m_cvResult;
    mutable std::condition_variable_any m_cvNewItem;

	// Threads
	std::list<std::thread*> m_pActiveThreads;
	
	// State
	bool mAllStop;

	std::string mLogFileName;

    // Some counters
    long m_cElemToProcess;
    long m_cActiveProcessing;
    long m_cNeedsProcessing;

    // I/O queues
    std::vector<const std::wstring> mQueue;
    std::set<std::wstring> mResult;

	std::wofstream mOutLogger;	

    // Processing
	void process();	

public:
	void addItemToProcess(std::wstring sItem); // adds new item to process
	
	std::set<std::wstring> getResults() const; // waits and returns full results
										// note: doesn't cleans previous results
	ItemListHandler(std::string aLogFileName);
	~ItemListHandler();
};
