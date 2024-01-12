
#include "arithmetic_operations.hpp"
#include "utils.hpp"

#define CHUNK_SIZE 0x4000
#define WORKER_NONE 0
#define WORKER_REQUEST_DATA 1
#define WORKER_SENT_DATA 2
#define WORKER_DONE 3


void GrayscaleSequential(const std::string& path){

    HANDLE hGrayScaledBMP;

    std::string filename = getFilename(path);
    std::string absolutePath = "C:\\Facultate\\CSSO\\Week6\\" + filename;

    SECURITY_ATTRIBUTES sa = currentUserReadONLY();
   
    hGrayScaledBMP = CreateFileA(absolutePath.c_str(),
                                GENERIC_WRITE,
                                0,
                                &sa,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (hGrayScaledBMP == INVALID_HANDLE_VALUE){

        DWORD error = GetLastError();
        throw std::runtime_error("Could not create file " + std::to_string(error));
    }

    std::vector<BYTE> loadedImage = loadFileToVector(path);

    std::vector<BYTE> offsetBytes (loadedImage.begin() + 10, loadedImage.begin() + 14 );

    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    for (size_t i = pixelArrayOffset; i + 4 <= loadedImage.size(); i += 4){

        DWORD* currentPixel = reinterpret_cast<DWORD*>(&loadedImage[i]);
        GrayscaleOperation(currentPixel);

    }

    //writing processed vector to file.

    DWORD dwBytesToWrite = (DWORD)loadedImage.size();

    if(!(WriteFile(hGrayScaledBMP, loadedImage.data(), dwBytesToWrite, NULL, NULL))){

        DWORD error = GetLastError();
        CloseHandle(hGrayScaledBMP);
        throw std::runtime_error("Error writing to file " + std::to_string(error));
    }

    CloseHandle(hGrayScaledBMP);

}

void InverseSequential(const std::string& path){

    HANDLE hInversedBMP;
   
    std::string filename = getFilename(path);
    std::string absolutePath = "C:\\Facultate\\CSSO\\Week6\\" + filename;

    SECURITY_ATTRIBUTES sa = currentUserReadONLY();
   
    hInversedBMP = CreateFileA(absolutePath.c_str(),
                                GENERIC_WRITE,
                                0,
                                &sa,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (hInversedBMP == INVALID_HANDLE_VALUE){

        DWORD error = GetLastError();
        throw std::runtime_error("Could not create file " + std::to_string(error));
    }

    std::vector<BYTE> loadedImage = loadFileToVector(path);

    std::vector<BYTE> offsetBytes (loadedImage.begin() + 10, loadedImage.begin() + 14 );

    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    for (size_t i = pixelArrayOffset; i + 4 <= loadedImage.size(); i += 4){

        DWORD* currentPixel = reinterpret_cast<DWORD*>(&loadedImage[i]);
        InverseOperation(currentPixel);

    }

    //writing processed vector to file.

    DWORD dwBytesToWrite = (DWORD)loadedImage.size();

    if(!(WriteFile(hInversedBMP, loadedImage.data(), dwBytesToWrite, NULL, NULL))){

        DWORD error = GetLastError();
        CloseHandle(hInversedBMP);
        throw std::runtime_error("Error writing to file " + std::to_string(error));
    }

    CloseHandle(hInversedBMP);

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
    void (*operation)(pixel_t *p) = **(void (**)(pixel_t * p))((PCHAR)data + 2 * sizeof(size_t));
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


void SaveVectorToFile(const std::string& filepath, const std::vector<BYTE>& buffer)
{
    SECURITY_ATTRIBUTES sa = currentUserReadONLY();

    HANDLE handle = CreateFileA(filepath.c_str(), GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Could not create file " + std::to_string(GetLastError()));

    DWORD dwBytesToWrite = (DWORD)buffer.size();

    if (!(WriteFile(handle, buffer.data(), dwBytesToWrite, NULL, NULL))) {
        CloseHandle(handle);
        throw std::runtime_error("Error writing to file " + std::to_string(GetLastError()));
    }

    CloseHandle(handle);
}


DWORD WINAPI staticThreadFunctionGrayscale (LPVOID lpParam){
    auto params = *static_cast<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>*>(lpParam);
    auto pixels = params.first;
    auto range = params.second;

    size_t start = range.first * sizeof(DWORD) + 54;
    size_t end = range.second * sizeof(DWORD) + 54;

    for(size_t i = start; i + 4 <= end; i += 4){

        DWORD* currentPixel = reinterpret_cast<DWORD*>(&(*pixels)[i]);
        GrayscaleOperation(currentPixel);
     
    }
    
    return 0;
}

void StaticParellelizedGrayscale(const std::string& path){
    
    int workers = 32;

    HANDLE hThreads[workers];
    std::string filename = getFilename(path);
    std::string absolutePath = "C:\\Facultate\\CSSO\\Week6\\" + filename;

   

    std::vector<BYTE> loadedImage = loadFileToVector(path); 

    std::vector<BYTE> offsetBytes (loadedImage.begin() + 10, loadedImage.begin() + 14 );

    size_t pixelArrayOffset = bytesToInt(offsetBytes); // Offset processed. Starting Image processing now.

    size_t pixels = (loadedImage.size() - pixelArrayOffset) / sizeof(DWORD);
    
    size_t avgPixelsPerThread = pixels / workers;
    size_t leftover = pixels % workers;
   
    

    std::vector<std::pair<size_t, size_t>> threadRanges;

    size_t startIndex = 0;
    size_t endIndex = 0;
    for (size_t i = 0; i < workers; i++) {
        endIndex = startIndex + avgPixelsPerThread - 1 + (leftover > 0 ? 1 : 0);
        threadRanges.push_back({startIndex, endIndex});
        startIndex = endIndex + 1;
        leftover = (leftover > 0) ? leftover - 1 : 0;
    }

    std::vector<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>> params;
    std::vector<BYTE>* pixelDataPtr = &loadedImage;

    for(size_t i = 0; i < workers; i++){

        params.push_back({pixelDataPtr, threadRanges[i]});
    }

    for(size_t i = 0; i < workers; i++){

        hThreads[i] = CreateThread(NULL, 0, staticThreadFunctionGrayscale, &params[i], 0, NULL);
        if (hThreads[i] == NULL) {

            DWORD error = GetLastError(); 

            throw std::runtime_error("CreateThread error " + std::to_string(error));
        }

         
    }

    WaitForMultipleObjects(workers, hThreads, TRUE, INFINITE);

    for (int i = 0; i < workers; ++i) {
        CloseHandle(hThreads[i]);
    }
    
    
    SaveVectorToFile(absolutePath, loadedImage);

}


 