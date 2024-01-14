#pragma once

#include "bmp_parser.hpp"
#include "utils.hpp"

#define GRAYSCALE_RED 0.299
#define GRAYSCALE_GREEN 0.587
#define GRAYSCALE_BLUE 0.114

void InverseOperation(pixel_t* pixel)
{
    *(LPDWORD)pixel = 0xFFFFFF00 - *(LPDWORD)pixel;
}

void GrayscaleOperation(pixel_t* p)
{
    BYTE grayscaleValue = GRAYSCALE_RED * p->r + GRAYSCALE_GREEN * p->g + GRAYSCALE_BLUE * p->b;
    p->r = p->g = p->b = grayscaleValue;
}
