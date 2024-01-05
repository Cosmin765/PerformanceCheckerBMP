#include "bmp_parser.hpp"

#define GRAYSCALE_RED 0.299
#define GRAYSCALE_GREEN 0.587
#define GRAYSCALE_BLUE 0.114

void RGBA_GRAYSCALE(std::vector<BYTE>& pixel, size_t startIndex, size_t finishIndex){

    if ((finishIndex - startIndex) != 4){

        throw std::runtime_error("Invalid argument. Must be of type RGBA32/ARGB32");
    }

    BYTE red = pixel[startIndex];

    BYTE green = pixel[startIndex + 1];

    BYTE blue = pixel[startIndex + 2];

    BYTE grayscaleValue = static_cast<BYTE>(GRAYSCALE_RED * red + GRAYSCALE_GREEN * green + GRAYSCALE_BLUE * blue);

    pixel[startIndex] = grayscaleValue;

    pixel[startIndex + 1] = grayscaleValue;

    pixel[startIndex + 2] = grayscaleValue;

}

void RGBA_INVERSE(std::vector<BYTE>& pixel, size_t startIndex, size_t finishIndex){

    if ((finishIndex - startIndex) != 4){

        throw std::runtime_error("Invalid argument. Must be of type RGBA32/ARGB32");
    }

    pixel[startIndex] = static_cast<BYTE>(0xFF - static_cast<int>(pixel[startIndex]));

    pixel[startIndex + 1] = static_cast<BYTE>(0xFF - static_cast<int>(pixel[startIndex + 1]));

    pixel[startIndex + 2] = static_cast<BYTE>(0xFF - static_cast<int>(pixel[startIndex + 2]));

}