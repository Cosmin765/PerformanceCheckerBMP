#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include "header_field.hpp"
#include <iomanip>
#include <sstream>
#define MAX_BYTES_TO_READ 0x4000

std::string bytesToHexString(const std::vector<BYTE> &bytes, size_t start, size_t end){

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for(size_t i = start; i < end; i++){

        ss << std::setw(2) <<static_cast<int>(bytes[i]);
     	ss<< ' ';
		
        
    }

    return ss.str();
}

int bytesToInt(const std::vector<BYTE>& bytes) {
    
    int result = 0;
    for (int i = 0; i < bytes.size(); i++) {
        result |= static_cast<int>(bytes[i]) << (i * 8); // BMP file header uses little endian format
    }
    
    return result;
}

std::vector<BYTE> loadFileToVector(const std::string& path){

	HANDLE hFile = CreateFile(path.c_str(),
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

std::vector<std::pair<std::string, std::string>> headerInfoFromLoadedFile(const std::vector<BYTE>& file){

	std::string hexString;
	std::pair<std::string, std::string> field;
	std::vector<std::pair<std::string, std::string>> fieldVector;
	int start,end;
 
	for (const auto& headerField : BMP_FILE_HEADER_AND_BITMAPINFOHEADER_FIELDS){
		
		start = headerField.getStartingOffset();
		end = start + headerField.getSize();
		hexString = bytesToHexString(file, start, end);

		field.first = headerField.getFieldDescription();
		field.second = hexString;

		fieldVector.push_back(field);
	}

	return fieldVector;
}






