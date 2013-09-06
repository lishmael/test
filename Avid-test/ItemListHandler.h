#pragma once

#include "Item.h"

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



typedef Item t_mapItem;
typedef std::wstring t_mapKey;
typedef std::pair<t_mapKey, t_mapItem> t_mapElement;

class ItemListHandler
{
public:
    enum PROCESSING_STATE { NONE, QUEUED, PROCESSING, READY, ERROR_CANT_OPEN_LOG_FILE, ERROR_UNKNOWN };
    
    ItemListHandler(std::map<t_mapKey, t_mapItem>::iterator begin,
                    std::map<t_mapKey, t_mapItem>::iterator end);
	~ItemListHandler(void); 


    void process();
    
    std::wstring getResult(size_t maxLinesToReturn);
    PROCESSING_STATE getState() const;
     
    mutable std::condition_variable m_cvSomeItemReady;
    
private:
    PROCESSING_STATE mState;

    ItemListHandler(const ItemListHandler&);
	ItemListHandler& operator=(const ItemListHandler&);

    // Processing  
    void threadWorker();
    bool init();
    bool deinit();
    bool reset();
	
    // Locks
    mutable std::mutex m_lockOperation;
    
	// Threads
	std::list<std::thread*> m_pActiveThreads;
    
    // I/O
    std::map<t_mapKey, t_mapItem>::iterator m_iEnd;
    std::map<t_mapKey, t_mapItem>::iterator m_iItemToProcess;
    std::map<t_mapKey, t_mapItem>::iterator m_iLog;
    std::wofstream m_ofsLogger;
    std::list<std::wstring> m_sRes;
};

