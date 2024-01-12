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

#define THREADS_COUNT 32


void GrayscaleSequential(const std::string& path){

    std::string filename = getFilename(path);
    std::string absolutePath = "C:\\Facultate\\CSSO\\Week6\\" + filename;

    std::vector<BYTE> loadedImage = loadFileToVector(path);

    std::vector<BYTE> offsetBytes (loadedImage.begin() + 10, loadedImage.begin() + 14 );

    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    for (size_t i = pixelArrayOffset; i + 4 <= loadedImage.size(); i += 4){

        pixel_t* currentPixel = reinterpret_cast<pixel_t*>(&loadedImage[i]);
        GrayscaleOperation(currentPixel);

    }
    
    SaveVectorToFile(absolutePath, loadedImage);
}

void InverseSequential(const std::string& path){

    std::string filename = getFilename(path);
    std::string absolutePath = "C:\\Facultate\\CSSO\\Week6\\" + filename;

    std::vector<BYTE> loadedImage = loadFileToVector(path);

    std::vector<BYTE> offsetBytes (loadedImage.begin() + 10, loadedImage.begin() + 14 );

    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    for (size_t i = pixelArrayOffset; i + 4 <= loadedImage.size(); i += 4){

        pixel_t* currentPixel = reinterpret_cast<pixel_t*>(&loadedImage[i]);
        InverseOperation(currentPixel);

    }

    SaveVectorToFile(absolutePath, loadedImage);
   
}


DWORD ApplyOperationChunked(void(*operation)(pixel_t*), pixel_t* buffer, pixel_t* end)
{
    while (buffer < end)
    {
        operation(buffer);
        buffer++;
    }
    return 0;
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

    while (sharedData->state != WORKER_NONE);
    sharedData->state = WORKER_DONE;

    return 0;
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
        ApplyOperationChunked(operation, pixels + index, pixels + end);

        if (WaitForSingleObject(hMutex, 20) == WAIT_OBJECT_0)
        {
            if (sharedData->state == WORKER_DONE)
            {
                ReleaseMutex(hMutex);
                break;
            }

            while (sharedData->state != WORKER_NONE);
            sharedData->state = WORKER_REQUEST_DATA;
            while (sharedData->state != WORKER_SENT_DATA);
            sharedData->state = WORKER_NONE;

            index = sharedData->index;
            end = sharedData->end;

            ReleaseMutex(hMutex);
        }
    }

    return 0;
}


DWORD WINAPI staticThreadFunctionGrayscale (LPVOID lpParam){
    auto params = *static_cast<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>*>(lpParam);
    auto pixels = params.first;
    auto range = params.second;

    std::vector<BYTE> offsetBytes(pixels->begin() + 10, pixels->begin() + 14);

    size_t offset = bytesToInt(offsetBytes);

    size_t start = range.first * sizeof(DWORD) + offset;
    size_t end = range.second * sizeof(DWORD) + offset;

    for(size_t i = start; i + 4 <= end; i += 4){

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

    for(size_t i = start; i + 4 <= end; i += 4){

        pixel_t* currentPixel = reinterpret_cast<pixel_t*>(&(*pixels)[i]);
        InverseOperation(currentPixel);
     
    }
    
    return 0;
}

void StaticParellelizedGrayscale(const std::string& path){
    
    //DWORD workers = getProcessorCores() * 2;

    //HANDLE hThreads[workers];
    //std::string filename = getFilename(path);
    //std::string absolutePath = "C:\\Facultate\\CSSO\\Week6\\" + filename;

   

    //std::vector<BYTE> loadedImage = loadFileToVector(path); 

    //std::vector<BYTE> offsetBytes (loadedImage.begin() + 10, loadedImage.begin() + 14 );

    //size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    //size_t pixels = (loadedImage.size() - pixelArrayOffset) / sizeof(DWORD);
    //
    //size_t avgPixelsPerThread = pixels / workers;
    //size_t leftover = pixels % workers;
   
    //

    //std::vector<std::pair<size_t, size_t>> threadRanges;

    //size_t startIndex = 0;
    //size_t endIndex = 0;
    //for (size_t i = 0; i < workers; i++) {
    //    endIndex = startIndex + avgPixelsPerThread - 1 + (leftover > 0 ? 1 : 0);
    //    threadRanges.push_back({startIndex, endIndex});
    //    startIndex = endIndex + 1;
    //    leftover = (leftover > 0) ? leftover - 1 : 0;
    //}

    //std::vector<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>> params;
    //std::vector<BYTE>* pixelDataPtr = &loadedImage;

    //for(size_t i = 0; i < workers; i++){

    //    params.push_back({pixelDataPtr, threadRanges[i]});
    //}

    //for(size_t i = 0; i < workers; i++){

    //    hThreads[i] = CreateThread(NULL, 0, staticThreadFunctionInverse, &params[i], 0, NULL);
    //    if (hThreads[i] == NULL) {

    //        DWORD error = GetLastError(); 

    //        throw std::runtime_error("CreateThread error " + std::to_string(error));
    //    }

    //     
    //}

    //WaitForMultipleObjects(workers, hThreads, TRUE, INFINITE);

    //for (int i = 0; i < workers; ++i) {
    //    CloseHandle(hThreads[i]);
    //}
    //
    //SaveVectorToFile(absolutePath, loadedImage);

}


void StaticParellelizedInverse(const std::string& path){
    
    //DWORD workers = getProcessorCores() * 2;

    //HANDLE hThreads[workers];
    //std::string filename = getFilename(path);
    //std::string absolutePath = "C:\\Facultate\\CSSO\\Week6\\" + filename;

   

    //std::vector<BYTE> loadedImage = loadFileToVector(path); 

    //std::vector<BYTE> offsetBytes (loadedImage.begin() + 10, loadedImage.begin() + 14 );

    //size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    //size_t pixels = (loadedImage.size() - pixelArrayOffset) / sizeof(DWORD);
    //
    //size_t avgPixelsPerThread = pixels / workers;
    //size_t leftover = pixels % workers;
   
    //

    //std::vector<std::pair<size_t, size_t>> threadRanges;

    //size_t startIndex = 0;
    //size_t endIndex = 0;
    //for (size_t i = 0; i < workers; i++) {
    //    endIndex = startIndex + avgPixelsPerThread - 1 + (leftover > 0 ? 1 : 0);
    //    threadRanges.push_back({startIndex, endIndex});
    //    startIndex = endIndex + 1;
    //    leftover = (leftover > 0) ? leftover - 1 : 0;
    //}

    //std::vector<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>> params;
    //std::vector<BYTE>* pixelDataPtr = &loadedImage;

    //for(size_t i = 0; i < workers; i++){

    //    params.push_back({pixelDataPtr, threadRanges[i]});
    //}

    //for(size_t i = 0; i < workers; i++){

    //    hThreads[i] = CreateThread(NULL, 0, staticThreadFunctionGrayscale, &params[i], 0, NULL);
    //    if (hThreads[i] == NULL) {

    //        DWORD error = GetLastError(); 

    //        throw std::runtime_error("CreateThread error " + std::to_string(error));
    //    }

    //     
    //}

    //WaitForMultipleObjects(workers, hThreads, TRUE, INFINITE);

    //for (int i = 0; i < workers; ++i) {
    //    CloseHandle(hThreads[i]);
    //}
    //
    //SaveVectorToFile(absolutePath, loadedImage);

}

VOID StartDynamicParallel(void (*operation)(pixel_t* p), std::string filepath, std::string outputPath)
{   
    if (!outputPath.size())
        throw std::runtime_error("You must specify the output path for all the chosen operations.");

    DWORD dwAttrib = GetFileAttributesA(outputPath.c_str());

    if (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);
        outputPath = outputPath + "/" + filename;
    }

    std::vector<BYTE> loadedImage = loadFileToVector(filepath);
    DWORD offset = *(LPDWORD)(loadedImage.data() + 10);
    pixel_t* buffer = (pixel_t*)(loadedImage.data() + offset);
    DWORD size = (loadedImage.size() - offset) / sizeof(pixel_t);

    ThreadPool pool(THREADS_COUNT + 1);

    worker_cs sharedData; memset(&sharedData, 0, sizeof(sharedData));

    HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
    if (!hMutex)
        exit(1);

    void* workerData[] = { buffer, &hMutex, operation, &sharedData };
    void* coordinatorData[] = { &size, &sharedData };

    pool.Submit(LoadBalancer, coordinatorData);
    for (int i = 0; i < THREADS_COUNT; ++i)
        pool.Submit(ApplyOperationDynamicParallel, workerData);

    pool.Shutdown();

    CloseHandle(hMutex);

    SaveVectorToFile(outputPath, loadedImage);
}


 
