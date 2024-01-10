
#include "arithmetic_operations.hpp"
#include "utils.hpp"


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

DWORD WINAPI staticThreadFunctionGrayscale (LPVOID lpParam){
    auto params = static_cast<std::pair<std::vector<BYTE>*, std::pair<size_t, size_t>>*>(lpParam);
    auto pixels = params->first;
    auto range = params->second;

    size_t start = range.first;
    size_t end = range.second;
    
    for(size_t i = start; i < end; i += 4){
        RGBA_GRAYSCALE(*pixels, i, i + 4);
    }
    
    
    delete params;
    return 0;
}

void StaticParellelizedGrayscale(const std::string& path){
    
    int workers = 32;

    HANDLE hThreads[workers];
    
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

    size_t bytes = (loadedImage.size() - pixelArrayOffset);
    size_t avgBytesPerThread = bytes / workers;
    size_t leftover = bytes % workers;
    

    size_t startIndex = 0;
    size_t endIndex = avgBytesPerThread - 1;
    for(size_t i = 0; i < workers; i ++ ){
        
        if(leftover > 0){
            
            endIndex ++;
            std::pair<size_t, size_t> range(startIndex, endIndex);

            std::pair<std::vector<BYTE>* , std::pair<size_t, size_t>> param(&loadedImage, range);
            
            
            hThreads[i] = CreateThread(NULL, 0, staticThreadFunctionGrayscale, &param, 0, NULL);

            if(hThreads[i] = NULL){

                DWORD error = GetLastError();
                throw std::runtime_error("Thread " + std::to_string(i) + " error " + std::to_string(error));
            }
        }

        else{

            std::pair<size_t, size_t> range(startIndex, endIndex);

            std::pair<std::vector<BYTE>* , std::pair<size_t, size_t>> param(&loadedImage, range);

             hThreads[i] = CreateThread(NULL, 0, staticThreadFunctionGrayscale, &param, 0, NULL);

            if(hThreads[i] = NULL){

                DWORD error = GetLastError();
                throw std::runtime_error("Thread " + std::to_string(i) + " error " + std::to_string(error));
            }
        }

        startIndex = endIndex + 1;
        endIndex = startIndex + avgBytesPerThread - 1;

        
    }

    WaitForMultipleObjects(workers, hThreads, TRUE, INFINITE);

    
    for (int i = 0; i < workers; ++i) {
        CloseHandle(hThreads[i]);
    }   



    
} 