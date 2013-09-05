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

enum PROCESSING_STATE { NONE, QUEUED, PROCESSING, READY };

typedef std::pair<std::wstring, PROCESSING_STATE> t_mapValue;
typedef std::wstring t_mapKey;
typedef std::pair<t_mapKey, t_mapValue> t_mapItem;

class ItemListHandler
{
private:
    PROCESSING_STATE mState;

    ItemListHandler(const ItemListHandler&);
	ItemListHandler& operator=(const ItemListHandler&);

	// Locks
    mutable std::mutex m_lockOperation;
    
	// Threads
	std::list<std::thread*> m_pActiveThreads;
    
    // I/O
    std::map<t_mapKey, t_mapValue>::iterator m_iEnd;
    std::map<t_mapKey, t_mapValue>::iterator m_iQueueProcesssing;

    // Processing  
    void start();
    void processAndEnd();
	
    void process();	
public:
    mutable std::condition_variable m_cvResult;
    mutable std::condition_variable m_cvProcessingEnds;
    
    ItemListHandler(void);
	~ItemListHandler(void); 
    
    bool addQueueToProcess(std::map<t_mapKey, t_mapValue>::iterator begin,
                           std::map<t_mapKey, t_mapValue>::iterator end); // adds queue to process;
    void processQueue();
    bool isReady() const; 
};
