#pragma once

#include <stdio.h>


#define EXPECT_ELSE(c, ...) if(!(c)) { \
		printf("Warning: %d - line %d on file %s\n", GetLastError(), __LINE__, __FILE__); \
		__VA_ARGS__; \
	}

#define EXPECT(c, ...) if(!(c)) { \
		printf("Error: %d - line %d on file %s\n", GetLastError(), __LINE__, __FILE__); \
		__VA_ARGS__; \
		ExitProcess(1); \
	}

#define SMALL_BUFFER_SIZE 128
#define MEDIUM_BUFFER_SIZE 1024
#define LARGE_BUFFER_SIZE 8192

#define UPLOAD_BUTTON_ID 1
#define START_BUTTON_ID 2

#define SEPARATOR_PERCENT_X 60
#define SEPARATOR_PERCENT_Y_1 33
#define SEPARATOR_PERCENT_Y_2 66
#define PADDING 15

#define UPLOAD_BUTTON_W 150
#define UPLOAD_BUTTON_H 30

#define LABEL_W 200
#define LABEL_H 20

#define INPUT_W 300
#define INPUT_H 20

#define LIST_BOX_H 50

#define HOME_DIR "C:\\Facultate\\CSSO\\Week6"

#define GRAYSCALE_OUTPUT_DIR "C:\\Facultate\\CSSO\\Week6\\grayscale"
#define INVERT_OUTPUT_DIR "C:\\Facultate\\CSSO\\Week6\\invert"
#define COMP_OUTPUT_PATH "C:\\Facultate\\CSSO\\Week6\\comp.txt"

#define INFO_FILEPATH "C:\\Facultate\\CSSO\\Week6\\info.txt"

#define GRAYSCALE_OP_INDEX 0
#define INVERT_OP_INDEX 1

#define SEQUENTIAL_MODE_INDEX 0
#define STATIC_MODE_INDEX 1
#define DYNAMIC_MODE_INDEX 2
