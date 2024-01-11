#pragma once

#include <Windows.h>
#include <ShObjIdl.h>
#include <vector>
#include <string>

#include "debugging.hpp"

#include "interface_utils.hpp"
#include "thread_pool.hpp"

VOID HandlePaint(HWND hWnd, int width, int height)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    // paint the update region with the default window bg color
    RECT windowRect;
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = width;
    windowRect.bottom = height;

    FillRect(hdc, &windowRect, (HBRUSH)(COLOR_WINDOW + 1));

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

VOID HandleDisplayImages(HWND hWnd, const std::vector<std::u16string>& filepaths)
{
    std::u16string buffer = u"BMP images:\r\n";

    for (const auto& filepath : filepaths) {
        buffer += filepath;
        buffer += u"\r\n";
    }
    SetWindowText(hWnd, (LPCWSTR)buffer.c_str());
}


VOID HandleProcessingImage(std::u16string filepath, DWORD opsMask, DWORD modesMask)
{
    std::string ansii_filepath = ConvertFromU16(filepath);

    BOOL grayscale = opsMask & (1 << GRAYSCALE_OP_INDEX);
    BOOL invert = opsMask & (1 << INVERT_OP_INDEX);

    if (modesMask & (1 << SEQUENTIAL_MODE_INDEX))
    {
        // TODO: handle output paths
        if (grayscale) 
            grayscaleSequential(ansii_filepath);

        if (invert)
            InverseSequential(ansii_filepath);
    }
    
    if(modesMask & (1 << STATIC_MODE_INDEX))
    {
        // TODO: implement
    }
    
    if(modesMask & (1 << DYNAMIC_MODE_INDEX))
    {
        auto operation = grayscale ? GrayscaleOperation : InverseOperation;

        std::vector<BYTE> loadedImage = loadFileToVector(ansii_filepath);
        DWORD offset = *(LPDWORD)(loadedImage.data() + 10);
        pixel_t* buffer = (pixel_t*)(loadedImage.data() + offset);
        DWORD size = (loadedImage.size() - offset) / sizeof(pixel_t);

        ThreadPool pool(THREADS_COUNT + 1);

        worker_cs sharedData; memset(&sharedData, 0, sizeof(sharedData));
        
        HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
        EXPECT(hMutex);

        void* workerData[] = { buffer, &hMutex, operation, &sharedData };
        void* coordinatorData[] = { &size, &sharedData };

        pool.Submit(LoadBalancer, coordinatorData);
        for (int i = 0; i < THREADS_COUNT; ++i)
        {
            pool.Submit(ApplyOperationDynamicParallel, workerData);
        }
        pool.Shutdown();

        CloseHandle(hMutex);

        std::string output_path = "C:\\Facultate\\CSSO\\Week6\\image_ctu.bmp";
        SaveVectorToFile(output_path, loadedImage);
    }
}
