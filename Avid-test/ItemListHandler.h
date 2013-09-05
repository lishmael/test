#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <windows.h>
#include <condition_variable>
#include <chrono>


class ItemListHandler
{
private:
    ItemListHandler(const ItemListHandler&);
	ItemListHandler& operator=(const ItemListHandler&);

	// Locks
    mutable std::mutex m_lockOperation;
    
	// Threads
	std::list<std::thread*> m_pActiveThreads;
    
    // I/O
    std::map<std::wstring, std::pair<std::wstring, PROCESSING_STATE> >* mResult;
    std::map<std::wstring, std::wstring>::iterator m_iQueueProcesssing;

    // Processing
    void PROCESSING_STATE mState;
    
    void start();
    void end();
	void process();	

public:
    enum PROCESSING_STATE = { QUEUED, PROCESSING, READY };

    mutable std::condition_variable m_cvResult;
    mutable std::condition_variable m_cvProcessingEnds;
     
    void addQueueToProcess(std::map<std::wstring, std::wstring>* mQueue); // adds queue to process;
    bool processQueue();
    bool isReady() const; 
	ItemListHandler();
	~ItemListHandler();
};
