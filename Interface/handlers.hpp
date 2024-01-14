#pragma once

#include <Windows.h>
#include <ShObjIdl.h>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
using namespace std::chrono;

#include "interface_utils.hpp"

HWND hPerformancePanel = NULL;

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
        buffer += filepath + u"\r\n";
    }
    SetWindowText(hWnd, (LPCWSTR)buffer.c_str());
}


VOID HandleStartProcessing(const std::string& filepath, const std::string& outputPath, const std::string& operation, std::function<void(std::vector<BYTE>&)> entryPoint, HANDLE hCsv, int workers = 1)
{
    std::vector<BYTE> loadedImage = loadFileToVector(filepath);

    auto start = high_resolution_clock::now();
    entryPoint(loadedImage);
    auto delta = duration_cast<milliseconds>(high_resolution_clock::now() - start);

    std::string durationStr = std::to_string(delta.count());

    std::string outPath = outputPath;
    CheckPaths(filepath, outPath, operation, durationStr);

    SaveVectorToFile(outPath, loadedImage);

    std::string stats = "Processed " + filepath + " with operation " + operation + 
        (workers == -1 ? "" : " and " + std::to_string(workers) + " threads") + " in " + 
        durationStr + "ms. Result saved at location " + outPath + "\r\n";
    AppendTextToWindow(hPerformancePanel, stats);

    std::string csvHeader = "filepath,operation,duration(ms),workers";

    std::string csvLine = filepath + "," + operation + "," + durationStr + "," + std::to_string(workers) + "\r\n";
    EXPECT_ELSE(WriteFile(hCsv, csvLine.c_str(), csvLine.size(), NULL, NULL));
}


VOID HandleProcessingImage(std::u16string filepath, std::u16string grayscaleOutputPath, std::u16string invertOutputPath, DWORD opsMask, DWORD modesMask, HANDLE hCsv)
{
    std::string ansiiFilepath = ConvertFromU16(filepath);

    BOOL grayscale = opsMask & (1 << GRAYSCALE_OP_INDEX);
    BOOL invert = opsMask & (1 << INVERT_OP_INDEX);

    std::string ansiiGrayscaleOutputPath = ConvertFromU16(grayscaleOutputPath);
    std::string ansiiInvertOutputPath = ConvertFromU16(invertOutputPath);

    DWORD maxWorkers = getProcessorCores() * 2;

    if (modesMask & (1 << SEQUENTIAL_MODE_INDEX))
    {
        if (grayscale)
            HandleStartProcessing(ansiiFilepath, ansiiGrayscaleOutputPath, "grayscale_sequential", GrayscaleSequential, hCsv);

        if (invert)
            HandleStartProcessing(ansiiFilepath, ansiiInvertOutputPath, "invert_sequential", InverseSequential, hCsv);
    }

    if (modesMask & (1 << STATIC_MODE_INDEX))
    {
        if (grayscale)
        {
            for (DWORD workers = 1; workers <= maxWorkers; ++workers)
            {
                auto entryPoint = [workers](std::vector<BYTE>& image) {
                    StaticParellelizedGrayscale(image, workers);
                    };
                HandleStartProcessing(ansiiFilepath, ansiiGrayscaleOutputPath, "grayscale_static", entryPoint, hCsv, workers);
            }
        }

        if (invert)
        {
            for (DWORD workers = 1; workers <= maxWorkers; ++workers)
            {
                auto entryPoint = [workers](std::vector<BYTE>& image) {
                    StaticParellelizedInverse(image, workers);
                    };
                HandleStartProcessing(ansiiFilepath, ansiiInvertOutputPath, "invert_static", entryPoint, hCsv, workers);
            }
        }
    }

    if (modesMask & (1 << DYNAMIC_MODE_INDEX))
    {
        if (grayscale)
        {
            for (DWORD workers = 1; workers <= maxWorkers; ++workers)
            {
                auto entryPoint = [workers](std::vector<BYTE>& image) {
                        StartDynamicParallel(GrayscaleOperation, image, workers);
                    };
                HandleStartProcessing(ansiiFilepath, ansiiGrayscaleOutputPath, "grayscale_dynamic", entryPoint, hCsv, workers);
            }
        }
        if (invert)
        {
            for (DWORD workers = 1; workers <= maxWorkers; ++workers)
            {
                auto entryPoint = [workers](std::vector<BYTE>& image) {
                    StartDynamicParallel(InverseOperation, image, workers);
                    };
                HandleStartProcessing(ansiiFilepath, ansiiInvertOutputPath, "invert_dynamic", entryPoint, hCsv, workers);
            }
        }
    }
}
