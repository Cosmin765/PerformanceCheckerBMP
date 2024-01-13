#pragma once

#include <Windows.h>
#include <ShObjIdl.h>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
using namespace std::chrono;

#include "debugging.hpp"

#include "interface_utils.hpp"

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


VOID HandleStartProcessing(const std::string& filepath, const std::string& outputPath, std::function<void(std::vector<BYTE>&)> entryPoint)
{
    std::vector<BYTE> loadedImage = loadFileToVector(filepath);

    auto start = high_resolution_clock::now();
    entryPoint(loadedImage);
    auto delta = duration_cast<milliseconds>(high_resolution_clock::now() - start);
    LogString(std::to_string(delta.count()).c_str());

    std::string outPath = outputPath;
    CheckPaths(filepath, outPath);

    SaveVectorToFile(outPath, loadedImage);
}


VOID HandleProcessingImage(std::u16string filepath, std::u16string grayscaleOutputPath, std::u16string invertOutputPath, DWORD opsMask, DWORD modesMask)
{
    std::string ansiiFilepath = ConvertFromU16(filepath);

    BOOL grayscale = opsMask & (1 << GRAYSCALE_OP_INDEX);
    BOOL invert = opsMask & (1 << INVERT_OP_INDEX);

    std::string ansiiGrayscaleOutputPath = ConvertFromU16(grayscaleOutputPath);
    std::string ansiiInvertOutputPath = ConvertFromU16(invertOutputPath);

    if (modesMask & (1 << SEQUENTIAL_MODE_INDEX))
    {
        if (grayscale)
            HandleStartProcessing(ansiiFilepath, ansiiGrayscaleOutputPath, GrayscaleSequential);

        if (invert)
            HandleStartProcessing(ansiiFilepath, ansiiInvertOutputPath, InverseSequential);
    }

    if (modesMask & (1 << STATIC_MODE_INDEX))
    {
        if (grayscale)
            HandleStartProcessing(ansiiFilepath, ansiiGrayscaleOutputPath, StaticParellelizedGrayscale);

        if (invert)
            HandleStartProcessing(ansiiFilepath, ansiiInvertOutputPath, StaticParellelizedInverse);

    }

    if (modesMask & (1 << DYNAMIC_MODE_INDEX))
    {
        if (grayscale)
            HandleStartProcessing(ansiiFilepath, ansiiGrayscaleOutputPath, [](std::vector<BYTE>& image)
                {
                    StartDynamicParallel(GrayscaleOperation, image);
                });

        if (invert)
        {
            HandleStartProcessing(ansiiFilepath, ansiiInvertOutputPath, [](std::vector<BYTE>& image)
                {
                    StartDynamicParallel(InverseOperation, image);
                });
        }
    }
}
