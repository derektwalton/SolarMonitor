#ifndef __return_code_h__
#define __return_code_h__

#define DTW_SUCCESS                  0
#define DTW_ERROR                    1
#define DTW_EOF                      2
#define DTW_BAD_SAMPLE_DATASTREAM    3
#define DTW_BAD_SAMPLE_DATETIME      4
#define DTW_BAD_SAMPLE_VALUE         5
#define DTW_NODATA                   6
#define DTW_BAD_ARGUMENT             7
#define DTW_OUT_OF_MEMORY            8
#define DTW_FILE_NOT_FOUND           9
#define DTW_RECORD_NOT_FOUND        10

typedef int DTW_RETURN_CODE_t;

#endif
