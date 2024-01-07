
#include "arithmetic_operations.hpp"

void grayscaleSequential(const std::string& path){

    HANDLE hGrayScaledBMP;
    HANDLE hFile;
   
    hGrayScaledBMP = CreateFileA("C:\\Facultate\\CSSO\\Week6\\imagine.bmp",
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (hGrayScaledBMP == INVALID_HANDLE_VALUE){

        DWORD error = GetLastError();
        throw std::runtime_error("Could not create file " + std::to_string(error));
    }

    std::vector<BYTE> loadedImage = loadFileToVector(path); 

    hFile = CreateFileA(path.c_str(),
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE){

        DWORD error = GetLastError();
        throw std::runtime_error("Error opening image " + std::to_string(error));
    }

    size_t pixelArrayOffset = bytesToInt(getPixelArrayOffset(hFile)); // Offset processed. Starting Image processing now.

    CloseHandle(hFile);

    for (size_t i = pixelArrayOffset; i < loadedImage.size(); i += 4){

        RGBA_GRAYSCALE(loadedImage, i, i + 4);

    }

    //writing processed vector to file.

    DWORD dwBytesToWrite = (DWORD)loadedImage.size();

    if(!(WriteFile(hGrayScaledBMP, loadedImage.data(), dwBytesToWrite, NULL, NULL))){

        DWORD error = GetLastError();
        CloseHandle(hFile);
        CloseHandle(hGrayScaledBMP);
        throw std::runtime_error("Error writing to file " + std::to_string(error));
    }

    CloseHandle(hGrayScaledBMP);

}

void InverseSequential(const std::string& path){

    HANDLE hInversedBMP;
    HANDLE hFile;
   
    hInversedBMP = CreateFileA("C:\\Facultate\\CSSO\\Week6\\imagine.bmp",
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (hInversedBMP == INVALID_HANDLE_VALUE){

        DWORD error = GetLastError();
        throw std::runtime_error("Could not create file " + std::to_string(error));
    }

    std::vector<BYTE> loadedImage = loadFileToVector(path); 

    hFile = CreateFileA(path.c_str(),
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE){

        DWORD error = GetLastError();
        throw std::runtime_error("Error opening image " + std::to_string(error));
    }

    size_t pixelArrayOffset = bytesToInt(getPixelArrayOffset(hFile)); // Offset processed. Starting Image processing now.

    CloseHandle(hFile);

    for (size_t i = pixelArrayOffset; i < loadedImage.size(); i += 4){

        RGBA_INVERSE(loadedImage, i, i + 4);

    }

    //writing processed vector to file.

    DWORD dwBytesToWrite = (DWORD)loadedImage.size();

    if(!(WriteFile(hInversedBMP, loadedImage.data(), dwBytesToWrite, NULL, NULL))){

        DWORD error = GetLastError();
        CloseHandle(hFile);
        CloseHandle(hInversedBMP);
        throw std::runtime_error("Error writing to file " + std::to_string(error));
    }

    CloseHandle(hInversedBMP);

}