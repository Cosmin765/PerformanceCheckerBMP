#pragma once

#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include "header_field.hpp"
#include "utils.hpp"
#include <iomanip>
#include <sstream>
#include <unordered_set>
#define MAX_BYTES_TO_READ 0x4000

HWND hBMPInfoPanel = NULL;

std::string bytesToHexString(const std::vector<BYTE> &bytes, size_t start, size_t end){

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for(size_t i = start; i < end; i++)
        ss << std::setw(2) << static_cast<int>(bytes[i]) << ' ';

    return ss.str();
}

int bytesToInt(const std::vector<BYTE>& bytes) {
    
    int result = 0;
    for (int i = 0; i < bytes.size(); i++) {
        result |= static_cast<int>(bytes[i]) << (i * 8); // BMP file header uses little endian format
    }
    
    return result;
}

std::vector<std::pair<std::string, std::string>> headerInfoFromLoadedFile(const std::vector<BYTE>& file) {

	std::string hexString;
	std::pair<std::string, std::string> field;
	std::vector<std::pair<std::string, std::string>> fieldVector;
	int start, end;

	for (const auto& headerField : BMP_FILE_HEADER_AND_BITMAPINFOHEADER_FIELDS) {

		start = headerField.getStartingOffset();
		end = start + headerField.getSize();
		hexString = bytesToHexString(file, start, end);

		field.first = headerField.getFieldDescription();
		field.second = hexString;

		fieldVector.push_back(field);
	}

	return fieldVector;
}

std::unordered_set<std::string> displayedImagesInfo;


VOID DisplayBMPFileInfo(const std::vector<BYTE>& image, const std::string& filepath)
{
	if (displayedImagesInfo.find(filepath) != displayedImagesInfo.end())
		return;

	displayedImagesInfo.insert(filepath);

	std::string info = "--- BMP info for " + filepath + " ---\r\n";
	for (const auto& item : headerInfoFromLoadedFile(image))
		info += item.first + ": " + item.second + "\r\n";
	
	info += "\r\n";

	AppendTextToWindow(hBMPInfoPanel, info);
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

	DisplayBMPFileInfo(buffer, path);

	return buffer;
}
