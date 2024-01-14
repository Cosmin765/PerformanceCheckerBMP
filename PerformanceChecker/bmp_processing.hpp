#pragma once

#include "arithmetic_operations.hpp"
#include "utils.hpp"
#include "bmp_parser.hpp"
#include "thread_pool.hpp"

#define CHUNK_SIZE 0x4000
#define WORKER_NONE 0
#define WORKER_REQUEST_DATA 1
#define WORKER_SENT_DATA 2
#define WORKER_DONE 3


void GrayscaleSequential(std::vector<BYTE>& image)
{
    std::vector<BYTE> offsetBytes (image.begin() + 10, image.begin() + 14 );
    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    for (size_t i = pixelArrayOffset; i + 4 <= image.size(); i += 4){
        pixel_t* currentPixel = reinterpret_cast<pixel_t*>(&image[i]);
        GrayscaleOperation(currentPixel);
    }
}

void InverseSequential(std::vector<BYTE>& image)
{
    std::vector<BYTE> offsetBytes (image.begin() + 10, image.begin() + 14 );
    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    for (size_t i = pixelArrayOffset; i + 4 <= image.size(); i += 4){
        pixel_t* currentPixel = reinterpret_cast<pixel_t*>(&image[i]);
        InverseOperation(currentPixel);
    }
}


DWORD LoadBalancer(LPVOID data)
{
    DWORD size = **(DWORD**)data;
    worker_cs* sharedData = *(worker_cs**)((PCHAR)data + sizeof(size_t));
    
    DWORD index = 0;

    while (index < size)
    {
        while (sharedData->state != WORKER_REQUEST_DATA);

        sharedData->index = index;
        sharedData->end = min(size, index + CHUNK_SIZE);
        
        index += CHUNK_SIZE;

        sharedData->state = WORKER_SENT_DATA;
    }

    while (sharedData->state != WORKER_DONE)
    {
        sharedData->state = WORKER_DONE;
        Sleep(10);
    }

    return 0;
}


#include <fstream>
#include <locale>
#include <codecvt>

constexpr char logLocation[] = "C:\\Facultate\\CSSO\\Week6\\log";

std::string ConvertU16String(const char16_t* str) {
    static std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
    return converter.to_bytes(str);
}

void LogString(const char* str, bool clear = false) {
    int mode = clear ? (std::ios::out | std::ios::trunc) : (std::ios::out | std::ios::app);
    std::ofstream fout(logLocation, mode);
    fout << str << "\n";
    fout.flush();
}

void LogString(const char16_t* str, bool clear = false) {
    int mode = clear ? (std::ios::out | std::ios::trunc) : (std::ios::out | std::ios::app);
    std::ofstream fout(logLocation, mode);
    fout << ConvertU16String(str) << "\n";
    fout.flush();
}

DWORD ApplyOperationDynamicParallel(LPVOID data)
{
    pixel_t* pixels = *(pixel_t**)data;
    HANDLE hMutex = **(HANDLE**)((PCHAR)data + sizeof(size_t));
    void (*operation)(pixel_t *p) = **(void (**)(pixel_t* p))((PCHAR)data + 2 * sizeof(size_t));
    worker_cs* sharedData = *(worker_cs**)((PCHAR)data + 3 * sizeof(size_t));

    DWORD index = 0, end = 0;

    while (sharedData->state != WORKER_DONE)
    {
        if (WaitForSingleObject(hMutex, 20) == WAIT_OBJECT_0)
        {
            LogString(std::to_string(sharedData->state).c_str());

            while (sharedData->state != WORKER_NONE && sharedData->state != WORKER_DONE);
            if (sharedData->state == WORKER_DONE)
            {
                ReleaseMutex(hMutex);
                break;
            }

            sharedData->state = WORKER_REQUEST_DATA;

            while (sharedData->state != WORKER_SENT_DATA && sharedData->state != WORKER_DONE);
            if (sharedData->state == WORKER_DONE)
            {
                ReleaseMutex(hMutex);
                break;
            }

            index = sharedData->index;
            end = sharedData->end;

            sharedData->state = WORKER_NONE;

            ReleaseMutex(hMutex);

            while (index < end)
            {
                operation(pixels + index);
                index++;
            }
        }
    }

    return 0;
}


DWORD WINAPI staticThreadFunctionGrayscale (LPVOID lpParam)
{
    auto params = *static_cast<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>*>(lpParam);
    auto pixels = params.first;
    auto range = params.second;

    std::vector<BYTE> offsetBytes(pixels->begin() + 10, pixels->begin() + 14);

    size_t offset = bytesToInt(offsetBytes);

    size_t start = range.first * sizeof(DWORD) + offset;
    size_t end = range.second * sizeof(DWORD) + offset;

    for(size_t i = start; i + 4 <= end; i += 4)
    {
        pixel_t* currentPixel = reinterpret_cast<pixel_t*>(&(*pixels)[i]);
        GrayscaleOperation(currentPixel);
    }
    
    return 0;
}

DWORD WINAPI staticThreadFunctionInverse(LPVOID lpParam){
    auto params = *static_cast<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>*>(lpParam);
    auto pixels = params.first;
    auto range = params.second;

    std::vector<BYTE> offsetBytes(pixels->begin() + 10, pixels->begin() + 14);
    size_t offset = bytesToInt(offsetBytes);

    size_t start = range.first * sizeof(DWORD) + offset;
    size_t end = range.second * sizeof(DWORD) + offset;

    for(size_t i = start; i + 4 <= end; i += 4)
    {
        pixel_t* currentPixel = reinterpret_cast<pixel_t*>(&(*pixels)[i]);
        InverseOperation(currentPixel);
    }
    
    return 0;
}

void StaticParellelizedGrayscale(std::vector<BYTE>& image, DWORD workers)
{
    HANDLE* hThreads = new HANDLE[workers];
    if (!hThreads)
        throw std::runtime_error("Could not instantiate threads");

    std::vector<BYTE> offsetBytes (image.begin() + 10, image.begin() + 14 );
    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.
    size_t pixels = (image.size() - pixelArrayOffset) / sizeof(DWORD);
    size_t avgPixelsPerThread = pixels / workers;
    size_t leftover = pixels % workers;
    std::vector<std::pair<size_t, size_t>> threadRanges;

    size_t startIndex = 0;
    size_t endIndex = 0;
    for (size_t i = 0; i < workers; i++)
    {
        endIndex = startIndex + avgPixelsPerThread - 1 + (leftover > 0 ? 1 : 0);
        threadRanges.push_back({startIndex, endIndex});
        startIndex = endIndex + 1;
        leftover = (leftover > 0) ? leftover - 1 : 0;
    }

    std::vector<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>> params;
    std::vector<BYTE>* pixelDataPtr = &image;

    for(size_t i = 0; i < workers; i++)
        params.push_back({pixelDataPtr, threadRanges[i]});

    for(size_t i = 0; i < workers; i++)
    {
        hThreads[i] = CreateThread(NULL, 0, staticThreadFunctionGrayscale, &params[i], 0, NULL);
        if (hThreads[i] == NULL)
            throw std::runtime_error("CreateThread error " + std::to_string(GetLastError()));
    }

    WaitForMultipleObjects(workers, hThreads, TRUE, INFINITE);

    for (int i = 0; i < workers; ++i) {
        CloseHandle(hThreads[i]);
    }

    delete[] hThreads;
}


void StaticParellelizedInverse(std::vector<BYTE>& image, DWORD workers){
    HANDLE* hThreads = new HANDLE[workers];
    if (!hThreads)
        throw std::runtime_error("Could not instantiate threads");

    std::vector<BYTE> offsetBytes (image.begin() + 10, image.begin() + 14 );
    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.
    size_t pixels = (image.size() - pixelArrayOffset) / sizeof(DWORD);
    size_t avgPixelsPerThread = pixels / workers;
    size_t leftover = pixels % workers;
    std::vector<std::pair<size_t, size_t>> threadRanges;

    size_t startIndex = 0;
    size_t endIndex = 0;
    for (size_t i = 0; i < workers; i++)
    {
        endIndex = startIndex + avgPixelsPerThread - 1 + (leftover > 0 ? 1 : 0);
        threadRanges.push_back({startIndex, endIndex});
        startIndex = endIndex + 1;
        leftover = (leftover > 0) ? leftover - 1 : 0;
    }

    std::vector<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>> params;
    std::vector<BYTE>* pixelDataPtr = &image;

    for(size_t i = 0; i < workers; i++)
        params.push_back({pixelDataPtr, threadRanges[i]});

    for(size_t i = 0; i < workers; i++)
    {
        hThreads[i] = CreateThread(NULL, 0, staticThreadFunctionInverse, &params[i], 0, NULL);
        if (hThreads[i] == NULL)
            throw std::runtime_error("CreateThread error " + std::to_string(GetLastError()));
    }

    WaitForMultipleObjects(workers, hThreads, TRUE, INFINITE);

    for (int i = 0; i < workers; ++i)
        CloseHandle(hThreads[i]);

    delete[] hThreads;
}


VOID StartDynamicParallel(void (*operation)(pixel_t* p), std::vector<BYTE>& image, DWORD workers)
{
    DWORD offset = *(LPDWORD)(image.data() + 10);
    pixel_t* buffer = (pixel_t*)(image.data() + offset);
    DWORD size = (image.size() - offset) / sizeof(pixel_t);

    ThreadPool pool(workers + 1);

    worker_cs sharedData; memset(&sharedData, 0, sizeof(sharedData));

    HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
    if (!hMutex)
        exit(1);

    void* workerData[] = { buffer, &hMutex, operation, &sharedData };
    void* coordinatorData[] = { &size, &sharedData };

    pool.Submit(LoadBalancer, coordinatorData);
    for (int i = 0; i < workers; ++i)
        pool.Submit(ApplyOperationDynamicParallel, workerData);

    pool.Shutdown();

    CloseHandle(hMutex);
}
