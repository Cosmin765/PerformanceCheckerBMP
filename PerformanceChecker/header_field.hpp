#pragma once
#include <string>
#include <vector>

class HeaderField{
    public:
        HeaderField(const std::string& fieldDescription, int startingOffset, int size);

        std::string getFieldDescription() const;
        int getStartingOffset() const;
        int getSize() const;

    private:
        std::string fieldDescription;
        int startingOffset;
        int size;      
};
extern std::vector<HeaderField> BMP_FILE_HEADER_AND_BITMAPINFOHEADER_FIELDS;