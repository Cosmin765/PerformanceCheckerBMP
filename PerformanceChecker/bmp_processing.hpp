
#include "arithmetic_operations.hpp"
#include "utils.hpp"

#define CHUNK_SIZE 0x4000
#define WORKER_NONE 0
#define WORKER_REQUEST_DATA 1
#define WORKER_SENT_DATA 2
#define WORKER_DONE 3


void grayscaleSequential(const std::string& path){

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

    for (size_t i = pixelArrayOffset; i < loadedImage.size(); i += 4){

        RGBA_GRAYSCALE(loadedImage, i, i + 4);

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

    for (size_t i = pixelArrayOffset; i < loadedImage.size(); i += 4){

        RGBA_INVERSE(loadedImage, i, i + 4);

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
