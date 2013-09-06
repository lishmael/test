#include "FileContextMenuExt.h"
#include "resource.h"

#include <strsafe.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")


extern HINSTANCE g_hInst;
extern long g_cDllRef;

#define IDM_DISPLAY             0  // The command's identifier offset

FileContextMenuExt::FileContextMenuExt(void) : m_cRef(1), 
    m_pszMenuText(L"&Avid-test"),
    m_pszVerb("Avid-test"),
    m_pwszVerb(L"Avid-test"),
    m_pszVerbCanonicalName("Avid-test"),
    m_pwszVerbCanonicalName(L"Avid-test"),
    m_pszVerbHelpText("Avid-test"),
    m_hMenuBmp(NULL)
{
    InterlockedIncrement(&g_cDllRef);
}

FileContextMenuExt::~FileContextMenuExt(void)
{
    if (m_hMenuBmp)
    {
        DeleteObject(m_hMenuBmp);
        m_hMenuBmp = NULL;
    }

    InterlockedDecrement(&g_cDllRef);
}


void FileContextMenuExt::OnVerbDisplayFileName(HWND hWnd) {
    std::thread handleThread(&FileContextMenuExt::calculateAndShow, this, hWnd, m_SelectedFiles);
    handleThread.detach();
}


void FileContextMenuExt::calculateAndShow(HWND hWnd, std::map<t_mapKey, t_mapItem> items) {
    try {
        ItemListHandler queueProcessor(items.begin(), items.end());
        std::wstring sMessage = L"Some error happened\n"; 
        
        if (queueProcessor.process()) {
            sMessage = queueProcessor.getResult(MAX_MESSAGE_ROWS - 1);

            if (items.size() > MAX_MESSAGE_ROWS - 1) {
                sMessage += L"(And " +
                            std::to_wstring(items.size() - MAX_MESSAGE_ROWS) + 
                            L" more file(s))\n...\n";
            }
        }
        MessageBox(hWnd, sMessage.c_str(), L"Avid-test", MB_OK);
        
    } catch (...) {
        MessageBox(hWnd, L"Unexpected exception", L"Avid-test", MB_OK);
    }

}

#pragma region IUnknown

// Query to the interface the component supported.
IFACEMETHODIMP FileContextMenuExt::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;
   
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IContextMenu)) {
        *ppv = static_cast<IContextMenu*>(this);
        hr = S_OK;
    } else if (riid == __uuidof(IShellExtInit)) {
        *ppv = static_cast<IShellExtInit*>(this);
        hr = S_OK;
    }
    
    if (hr == S_OK) {
        AddRef();
    }
    
    return hr;
}

// Increase the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) FileContextMenuExt::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// Decrease the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) FileContextMenuExt::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

#pragma endregion


#pragma region IShellExtInit

// Initialize the context menu handler.
IFACEMETHODIMP FileContextMenuExt::Initialize(
    LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
    if (NULL == pDataObj) {
        return E_INVALIDARG;
    }

    HRESULT hr = E_FAIL;

    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stm;
    
    if (SUCCEEDED(pDataObj->GetData(&fe, &stm)))
    {
        HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
        if (hDrop != NULL)
        {
            UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            hr = S_OK;
            
            wchar_t* p_TmpFName = new wchar_t[MAX_PATH];
            memset(p_TmpFName, 0, sizeof(wchar_t)*MAX_PATH);
            
            for (auto i = 0; i < nFiles; ++i) {
				if (0 == DragQueryFile(hDrop, i, p_TmpFName, sizeof(wchar_t)*MAX_PATH)) {
                    hr = E_FAIL;
                }
            
                t_mapKey itemKey = std::wstring(p_TmpFName);
                t_mapItem item(itemKey);
                
                m_SelectedFiles.insert(t_mapElement(itemKey, item));
            }
            delete[] p_TmpFName;
            GlobalUnlock(stm.hGlobal);
        }

        ReleaseStgMedium(&stm);
    }

    // If any value other than S_OK is returned from the method, the context 
    // menu item is not displayed.
    return hr;
}

#pragma endregion


#pragma region IContextMenu

IFACEMETHODIMP FileContextMenuExt::QueryContextMenu(
    HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (CMF_DEFAULTONLY & uFlags)
    {
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
    }

    MENUITEMINFO mii = { sizeof(mii) };
    mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE;
    mii.wID = idCmdFirst + IDM_DISPLAY;
    mii.fType = MFT_STRING;
    mii.dwTypeData = m_pszMenuText;
    mii.fState = MFS_ENABLED;
    mii.hbmpItem = static_cast<HBITMAP>(m_hMenuBmp);
    if (!InsertMenuItem(hMenu, indexMenu, TRUE, &mii))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    MENUITEMINFO sep = { sizeof(sep) };
    sep.fMask = MIIM_TYPE;
    sep.fType = MFT_SEPARATOR;
    if (!InsertMenuItem(hMenu, indexMenu + 1, TRUE, &sep))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_DISPLAY + 1));
}


IFACEMETHODIMP FileContextMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    BOOL fUnicode = FALSE;
    HRESULT hr = E_FAIL;

    if (pici->cbSize == sizeof(CMINVOKECOMMANDINFOEX)) {
        if (pici->fMask & CMIC_MASK_UNICODE) {
            fUnicode = TRUE;
        }
    }

    if (!fUnicode && HIWORD(pici->lpVerb)) {
        if (StrCmpIA(pici->lpVerb, m_pszVerb) == 0) {
            OnVerbDisplayFileName(pici->hwnd);
            hr = S_OK;
        }
    } else if (fUnicode && HIWORD(((CMINVOKECOMMANDINFOEX*)pici)->lpVerbW)) {
        if (StrCmpIW(((CMINVOKECOMMANDINFOEX*)pici)->lpVerbW, m_pwszVerb) == 0) {
            OnVerbDisplayFileName(pici->hwnd);
            hr = S_OK;
        }
    } else {
        if (LOWORD(pici->lpVerb) == IDM_DISPLAY) {
            OnVerbDisplayFileName(pici->hwnd);
            hr = S_OK;
        }
    }

    return hr;
}


IFACEMETHODIMP FileContextMenuExt::GetCommandString(UINT_PTR idCommand, 
    UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr = E_INVALIDARG;

    if (idCommand == IDM_DISPLAY)
    {
        switch (uFlags)
        {
        case GCS_HELPTEXTW:
            hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax, 
                m_pwszVerbHelpText);
            break;

        case GCS_VERBW:
            hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax, 
                m_pwszVerbCanonicalName);
            break;

        default:
            hr = S_OK;
        }
    }

    return hr;
}

#pragma endregion
