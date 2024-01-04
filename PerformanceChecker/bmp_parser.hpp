#include <iostream>
#include <windows.h>
#include <vector>
#include <string>

std::vector<BYTE> getPixelArrayOffset(HANDLE hFile){

	std::vector<BYTE> bytes (4);
	DWORD bytesToRead = 4;
	DWORD bytesRead;
	OVERLAPPED ol{};
	
	ol.Offset = 10; // the offset where the pixel array starts is located at the 10th byte

	if (!ReadFile(hFile, bytes.data(), bytesToRead, &bytesRead, &ol)){
		DWORD error = GetLastError();
		throw std::runtime_error ("ReadFile error " + std::to_string(error));

	}
	
	return bytes;

}

int bytesToInt(const std::vector<BYTE>& bytes) {
    
    int result = 0;
    for (int i = 0; i < bytes.size(); i++) {
        result |= static_cast<int>(bytes[i]) << (i * 8); // BMP file header uses little endian format
    }
    
    return result;
}



