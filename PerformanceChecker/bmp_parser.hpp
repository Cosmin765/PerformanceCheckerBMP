#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#define MAX_BYTES_TO_READ 0x4000


int bytesToInt(const std::vector<BYTE>& bytes) {
    
    int result = 0;
    for (int i = 0; i < bytes.size(); i++) {
        result |= static_cast<int>(bytes[i]) << (i * 8); // BMP file header uses little endian format
    }
    
    return result;
}

std::vector<BYTE> loadFileToVector(const std::string& path){

	HANDLE hFile = CreateFileA(path.c_str(),
							  GENERIC_READ,
							  FILE_SHARE_READ,
							  NULL,
							  OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL,
							  NULL);

	if (hFile == INVALID_HANDLE_VALUE){
		
		DWORD error = GetLastError();
		throw std::runtime_error("Failed to open file " + path + ". Error " + std::to_string(error));
	}

	std::vector<BYTE> buffer;

	while (TRUE){

		DWORD bytesRead = 0;
		buffer.resize(buffer.size() + MAX_BYTES_TO_READ);

		if(!ReadFile(hFile,
					 buffer.data() + buffer.size() - MAX_BYTES_TO_READ,
					 MAX_BYTES_TO_READ,
					 &bytesRead,
					 NULL))
		{	
			CloseHandle(hFile);
			DWORD error = GetLastError();
			throw std::runtime_error("Error reading file " + path + " " + std::to_string(error));

		}

		if (bytesRead < MAX_BYTES_TO_READ){ // reached EOF 
			buffer.resize(buffer.size() - MAX_BYTES_TO_READ + bytesRead);
			break;
		}

	}

	CloseHandle(hFile);
	return buffer;
}






