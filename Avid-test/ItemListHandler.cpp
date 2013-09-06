#include "ItemListHandler.h"

ItemListHandler::ItemListHandler(std::map<t_mapKey, t_mapItem>::iterator begin,
                                 std::map<t_mapKey, t_mapItem>::iterator end) : 
    mState(PROCESSING_STATE::NONE),
    m_iEnd(end),
    m_iItemToProcess(begin), 
    m_iLog(begin) {
        init();
}


ItemListHandler::~ItemListHandler(void) {
    deinit();
}

// ItemListHandler::process() 
// should be called to start processing items
// returns true if processing done successfully
// false if error occured;
// Call of this function blocks caller thread
bool ItemListHandler::process() {
    if (mState == PROCESSING_STATE::READY) {
         return true;
    }
    if (mState == PROCESSING_STATE::ERROR_UNKNOWN || mState == PROCESSING_STATE::ERROR_CANT_OPEN_LOG_FILE) {
        return false;
    }

    mState = PROCESSING_STATE::PROCESSING;
    
    unsigned int hw_threads = std::thread::hardware_concurrency();
    
    if (!hw_threads) hw_threads = 1;

    for (int i = 0; i < hw_threads; ++i) {
        m_pActiveThreads.push_back(new std::thread(&ItemListHandler::threadWorker, this));
    }

    for (; m_iLog != m_iEnd; ++m_iLog) {
        t_mapItem* item = &m_iLog->second;
        while (!item->isReady()) {
            std::unique_lock<std::mutex> _ul(m_lockOperation);
            m_cvSomeItemReady.wait_for(_ul, std::chrono::milliseconds(100));
        }

        m_sRes.push_back(item->getStat() + L"\n");
        if (m_ofsLogger.is_open()) {
            m_ofsLogger << item->getStat() + L"\n";
            m_ofsLogger.flush();
        }
    }

    return true;
} 

void ItemListHandler::threadWorker() {
    while (m_iItemToProcess != m_iEnd) {
        m_lockOperation.lock();

        auto i_elementProcessed = m_iItemToProcess;
        ++m_iItemToProcess;
        m_lockOperation.unlock();
        
        t_mapItem* item = &i_elementProcessed->second;
        item->process();
        
        m_cvSomeItemReady.notify_all();
    }
}


std::wstring ItemListHandler::getResult(size_t maxLinesToReturn) const {
    if (mState == PROCESSING_STATE::READY) {
        return L"Some error occured - state is not READY";
    }
    std::wstring tmpRes = L"";
    size_t done = 0;
    for (auto i = m_sRes.begin(); i != m_sRes.end() && done < maxLinesToReturn; ++i, ++done) {
        tmpRes += *i;
    }
    return tmpRes;
}


ItemListHandler::PROCESSING_STATE ItemListHandler::getState() const {
    return mState;
}

bool ItemListHandler::init() {
    if (mState != PROCESSING_STATE::NONE) return false;
    if (m_ofsLogger.is_open()) {
        m_ofsLogger.close();
    }
    
    mState = PROCESSING_STATE::ERROR_CANT_OPEN_LOG_FILE;
    
    if (0 != CreateDirectory(L"C:\\Avid-test\\", NULL) ||
        GetLastError() == ERROR_ALREADY_EXISTS) {
        m_ofsLogger.open(L"C:\\Avid-test\\avid-test.log",
            std::ofstream::out | std::ofstream::trunc);
        if (m_ofsLogger.is_open()) {
            m_ofsLogger << L"-------Begin log file" << std::endl;
            mState = PROCESSING_STATE::QUEUED;
        } 
    }  
    return true;
}

bool ItemListHandler::deinit() {
    if (mState == PROCESSING_STATE::PROCESSING) return false;
    for (auto p_Thread : m_pActiveThreads) {
        p_Thread->join();
        delete p_Thread;
    }

    if (m_ofsLogger.is_open()) {
        m_ofsLogger.close();
    }
    mState = PROCESSING_STATE::NONE;
    return true;
}

bool ItemListHandler::reset() {
    if (mState == PROCESSING_STATE::PROCESSING) return false;
    if (!deinit()) return false;
    return init();
}
