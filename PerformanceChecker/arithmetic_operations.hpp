#include "bmp_parser.hpp"

#define GRAYSCALE_RED 0.299
#define GRAYSCALE_GREEN 0.587
#define GRAYSCALE_BLUE 0.114

std::vector<BYTE> RGBA_GRAYSCALE(const std::vector<BYTE>& pixel){

    if (pixel.size() != 4){

        throw std::runtime_error("Invalid argument. Must be of type RGBA32/ARGB32");
    }

    std::vector<BYTE> grayscaledPixel = pixel;
    
    grayscaledPixel[0] = static_cast<BYTE>(GRAYSCALE_RED * static_cast<float>(pixel[0]));

    grayscaledPixel[1] = static_cast<BYTE>(GRAYSCALE_GREEN * static_cast<float>(pixel[1]));

    grayscaledPixel[2] = static_cast<BYTE>(GRAYSCALE_BLUE * static_cast<float>(pixel[2]));

    return grayscaledPixel;
}

std::vector<BYTE> RGBA_INVERSE(const std::vector<BYTE>& pixel){

    if(pixel.size() != 4){
        
        throw std::runtime_error("Invalid argument. Must be of type RGBA32/ARGB32");
    }

    std::vector<BYTE> inversedPixel = pixel;

    inversedPixel[0] = static_cast<BYTE>(0xFF - static_cast<int>(pixel[0]));

    inversedPixel[1] = static_cast<BYTE>(0xFF - static_cast<int>(pixel[1]));

    inversedPixel[2] = static_cast<BYTE>(0xFF - static_cast<int>(pixel[2]));

    return inversedPixel;


}