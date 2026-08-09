#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <x86intrin.h>
#include "CJson.h"

//Global Defn
#define DYNAMIC_STEP 2
#define MAX_PROPERTY_COUNT__ 32
#define MAX_READBUFFER_SIZE__ 256
#define MAX_VECTOR_SIZE__ 32  //As we used AVX and not AVX512
#if defined(__AVX__)
#define AVX_FUNC_COMPAT 1
#else
#define AVX_FUNC_COMPAT 0
#endif

//Pragmas
#pragma GCC target("avx2,bmi,bmi2")

//Macros
#define setErr(err) { \
    errVal=err;\
    goto Done;\
}
#define setBit1(val,bitNum) val = val | (1<<bitNum)
#define setIntBit0(val,bitNum) val = val & (0xFFFFFFFF ^ (1<<bitnum))
#define getBit(val,bitNum) (val & (1<<bitNum))

//Typdefs

typedef unsigned char bit8_t; // To prevent collision with <stdint.h> type uint8_t
//Enum Typedefs

//Type can be done using enums but I want to do it with bit flags as enums are 4 bytes, flag can do it in 1 byte(3 bits to be exact) [Nvm, I will require both]
typedef enum valueTypes {
    STRING,
    ARRAY,
    OBJECT
}value_t;

//Library Structs
typedef struct data {
    void *key;
    void **val;
    value_t valType;
    unsigned int objIndex; //The index of array with object is 1
}data_t;

typedef struct jsonLineData {
    bit8_t propertyKeyStart;  
    bit8_t propertyKeyEnd;
    bit8_t valueStart;
    bit8_t valueEnd;
    bool isArray;
    bool isObject;
}lineData_t;

struct jsonObj {
    data_t **jsonData;
    int propertyCount;
    int maxSize;
    bool isDynamic;
};

//Global Variables
const char* verboseErr[]={
    "CJ_ERR_OK: No Error",
    "CJ_ERR_INVARG: Passed an invalid argument",
    "CJ_ERR_INIT_FAIL: Failed to initialize pointer",
    "CJ_ERR_INVFILE: No such file found",
    "CJ_ERR_INVFRMT: Wrong file format",
    "CJ_ERR_INVSIZE: Size entered is too big/small",
    "CJ_ERR_INVCODE: Invalid error code",
    "CJ_ERR_NOCOMPAT: Library is incompatible with your device"
};

//Internal Functions

//SIMD based custom strlen() [more inefficient than the strlen() in <string.h> prolly] | Assumes: all passed pointers have memory equivalent to multiple of MAX_VECTOR_SIZE__
static int sslenAligned(char* str) {
    __m256i nullVec = _mm256_setzero_si256();
    unsigned int len=0,mask=0;
    do {
        __m256i data = _mm256_loadu_epi8(str+len);
        mask = _mm256_cmpeq_epi8_mask(data,nullVec);
        len = (mask==0)?len:len+MAX_VECTOR_SIZE__;
    } while (mask==0 && len<MAX_READBUFFER_SIZE__);
    len+=__builtin_ctz(mask);
    return (int)len;
}

//SIMD based deep copy [not as good as memcpy()] | Assumes: all passed pointers have memory equivalent to multiple of MAX_VECTOR_SIZE__
static bool deepncpyAligned(char* srcStr, char* destStr, int byteNum) {
    int iteCount = (byteNum%MAX_VECTOR_SIZE__==0)?byteNum/MAX_VECTOR_SIZE__:(byteNum/MAX_VECTOR_SIZE__)+1;
    
}

//Use SIMD to efficiently parse the 256 byte buffer of fgets() and fill the lineData_t struct with relevant info



//Public Functions

//Checks compatibilty to see if it can run AVX commands for SIMD
void CJsonCompat() {
    if (AVX_FUNC_COMPAT==0) {
        printf("This library uses AVX Functions which arent supported by this device. Hopefully in next upgrade cross compatibility is added\n");
        exit(CJ_ERR_NOCOMPAT);
    } else if (MAX_READBUFFER_SIZE__%32==0) {
        printf("This library max read buffer is set to %d which is not compatible with SIMD functions used. Do not use MAX_READBUDDER_SIZE__ in global definitions\n",MAX_READBUFFER_SIZE__);
        exit(CJ_ERR_NOCOMPAT);
    }
    //printf("%d\n",AVX_FUNC_COMPAT);
}

//Print Error and return error verbose
const char* printError__(cjson_err_t errorCode, int lineNum, const char* funcName, const char* fileName) {
    errorCode=(errorCode>CJ_ERR_INVCODE)?CJ_ERR_INVCODE:errorCode;
    printf("%s at line %d in function `%s` in file \"%s\"\n",verboseErr[errorCode],lineNum,funcName,fileName);

    return verboseErr[errorCode];
}
//Only return error verbose
const char* getErrorV(cjson_err_t errorCode) {
    errorCode=(errorCode>CJ_ERR_INVCODE)?CJ_ERR_INVCODE:errorCode;
    return verboseErr[errorCode];
}

//Create an json Obj 
cjson_err_t createJsonObj(jsonObj_t** dataObj, int propertyCount, bool isDynamic) {
    cjson_err_t errVal = CJ_ERR_OK;
    *dataObj=NULL;

    if (propertyCount>MAX_PROPERTY_COUNT__) {setErr(CJ_ERR_INVSIZE)} 

    jsonObj_t *newObj=malloc(sizeof(jsonObj_t));
    if (newObj==NULL) {setErr(CJ_ERR_INIT_FAIL)}

    newObj->isDynamic=isDynamic;
    newObj->maxSize=propertyCount;
    newObj->propertyCount=0;
    newObj->jsonData=malloc(sizeof(void*)*propertyCount);
    if (newObj->jsonData==NULL) {free(newObj);newObj=NULL;setErr(CJ_ERR_INIT_FAIL)}
    *dataObj=newObj;

    Done:
    return errVal;
}

//load data from json file to json obj passed here
cjson_err_t readJsonFile(char* filepath, jsonObj_t* dataObj) {
    cjson_err_t errVal=CJ_ERR_OK;
    int localPropertyCount=0;

    FILE *jsonFilePtr=fopen(filepath,"r");
    if (jsonFilePtr==NULL) {setErr(CJ_ERR_INVFILE)} 
    char *readBuffer=malloc(MAX_READBUFFER_SIZE__);
    if (readBuffer==NULL) {setErr(CJ_ERR_INIT_FAIL)}
    fgets(readBuffer,MAX_READBUFFER_SIZE__,jsonFilePtr);
    if (!localPropertyCount && *readBuffer!='{') {setErr(CJ_ERR_INVFRMT)} 
    bool isArray=false, isObject=false;
    for (localPropertyCount;localPropertyCount<dataObj->maxSize && feof(jsonFilePtr);) {
        fgets(readBuffer,MAX_READBUFFER_SIZE__,jsonFilePtr);
        if (*readBuffer=='}') {goto Clean;}

    }


    Clean:
    free(readBuffer);
    readBuffer=NULL;
    Done:
    if (jsonFilePtr!=NULL) {fclose(jsonFilePtr);}
    return errVal;
}

cjson_err_t destroyJsonObj(jsonObj_t* dataObj);