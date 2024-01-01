#include <Windows.h>
#include <ShObjIdl.h>
#include <vector>
#include <string>

#include "debugging.hpp"

VOID HandlePaint(HWND hWnd) 
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    // paint the update region with the default window bg color
    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

    EndPaint(hWnd, &ps);
}

std::vector<std::u16string> HandlePickImages()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (!SUCCEEDED(hr)) {
        return {};
    }

    IFileOpenDialog* pFileOpen;

    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    std::vector<std::u16string> filepaths;

    if (SUCCEEDED(hr))
    {
        COMDLG_FILTERSPEC ComDlgFS[] = { {L"Bitmap Image files", L"*.bmp"} };
        pFileOpen->SetFileTypes(1, ComDlgFS);

        DWORD dwOptions;
        pFileOpen->GetOptions(&dwOptions);
        pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);

        hr = pFileOpen->Show(NULL);

        if (SUCCEEDED(hr))
        {
            IShellItemArray* pItems;
            hr = pFileOpen->GetResults(&pItems);

            if (SUCCEEDED(hr))
            {
                DWORD count; pItems->GetCount(&count);
                for (DWORD i = 0; i < count; ++i) {
                    IShellItem* pItem;
                    pItems->GetItemAt(i, &pItem);

                    LPWSTR pszFilePath;

                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    if (SUCCEEDED(hr))
                    {
                        filepaths.push_back(std::u16string((char16_t*)pszFilePath));
                        CoTaskMemFree(pszFilePath);
                    }

                    pItem->Release();
                }
                pItems->Release();
            }
        }
        pFileOpen->Release();
    }
    CoUninitialize();

    return filepaths;
}


VOID HandleDisplayImages(const std::vector<std::u16string>& filepaths) {
    for (const auto& filepath : filepaths) {
        LogString(filepath.c_str());
        LogString("\n");
    }
}
