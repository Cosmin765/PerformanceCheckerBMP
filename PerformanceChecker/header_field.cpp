#include "header_field.hpp"
#include <vector>

HeaderField::HeaderField(const std::string& fieldDescription, int startingOffset, int size){
    
    this->fieldDescription = fieldDescription;
    this->startingOffset = startingOffset;
    this->size = size;

}

std::string HeaderField::getFieldDescription() const{
    return fieldDescription;
}

int HeaderField::getStartingOffset() const{
    return startingOffset;
}

int HeaderField::getSize() const{
    return size;
}

std::vector<HeaderField> BMP_FILE_HEADER_AND_BITMAPINFOHEADER_FIELDS{
    HeaderField("ID field", 0x0, 2),
    HeaderField("Size of file", 0x2, 4),
    HeaderField("Application specific", 0x6, 2),
    HeaderField("Application specific", 0x8, 2),
    HeaderField("Pixel array offset", 0xA, 4),
    HeaderField("Number of bytes in the DIB header", 0xE, 4),
    HeaderField("Image width in pixels", 0x12, 4),
    HeaderField("Image height in pixels", 0x16, 4),
    HeaderField("Number of color planes used", 0x1A, 2),
    HeaderField("Number of bits per pixel", 0x1C, 2),
    HeaderField("Compression method being used", 0x1E, 4),
    HeaderField("Image size", 0x22, 4),
    HeaderField("Horizontal image resolution", 0x26, 4),
    HeaderField("Vertical image resolution", 0x2A, 4),
    HeaderField("Number of colors in the color pallete", 0x2E, 4),
    HeaderField("Number of important colors used", 0x32, 4)
};