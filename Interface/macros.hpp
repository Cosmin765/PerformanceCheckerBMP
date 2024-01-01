#define EXPECT_ELSE(c, ...) if(!(c)) { \
		printf("Warning: %d - line %d on file %s\n", GetLastError(), __LINE__, __FILE__); \
		__VA_ARGS__; \
	}

#define EXPECT(c, ...) if(!(c)) { \
		printf("Error: %d - line %d on file %s\n", GetLastError(), __LINE__, __FILE__); \
		__VA_ARGS__; \
		ExitProcess(1); \
	}

#define SMALL_BUFFER_SIZE 100

#define UPLOAD_BUTTON_ID 1
