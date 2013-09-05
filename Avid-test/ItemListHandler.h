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

class Item { 
public:
    enum ITEM_STATE { RAW, PROCESSED, READY, BAD };
    
    Item(std::wstring wsFullPath);
    ~Item();

    void stat();

    bool isReady() const;
    ITEM_STATE getState() const;
    std::wstring getStat() const;
    std::wstring getFullPath() const;

private:
    std::wstring m_wsFullPath;
    std::wstring m_wsFileName;
    std::wstring m_wsChecksum;
    std::wstring m_wsSize;
    std::wstring m_wsCreationDate;
    ITEM_STATE mState;
};

typedef Item t_mapItem;
typedef std::wstring t_mapKey;
typedef std::pair<t_mapKey, t_mapItem> t_mapElement;

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
    std::map<t_mapKey, t_mapItem>::iterator m_iEnd;
    std::map<t_mapKey, t_mapItem>::iterator m_iQueueProcesssing;
    std::map<t_mapKey, t_mapItem>::iterator m_iLog;
    std::wofstream m_ofsLogger;
    std::wstring m_sRes;

    // Processing  
    void start();
    void invoke_processing();
    void log();
public:
    mutable std::condition_variable m_cvResult;
    mutable std::condition_variable m_cvProcessingEnds;
    
    ItemListHandler(std::map<t_mapKey, t_mapItem>::iterator begin,
                    std::map<t_mapKey, t_mapItem>::iterator end);
	~ItemListHandler(void); 
    
    bool reQueue(std::map<t_mapKey, t_mapItem>::iterator begin,
                 std::map<t_mapKey, t_mapItem>::iterator end);
    
    std::wstring getResult();
    
    void process();
    void waitTillReady();
    bool isReady() const; 
};

