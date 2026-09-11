#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <x86intrin.h>
#include "CJson.h"

//Global Defn
#define DYNAMIC_STEP 2
#define MAX_PROPERTY_COUNT__ 32
#define MAX_READBUFFER_SIZE__ 256
#define DEFAULT_ARRAY_SIZE 8
#define ARRAY_STEP_SIZE 2

//Constant Macros
//Ascii Macros
#define DOUBLE_QUOTES_64B 0x2222222222222222
#define COLON_64B 0x3A3A3A3A3A3A3A3A
#define COMMA_64B 0x2C2C2C2C2C2C2C2C
//INT type Macros
#define BIT8_T_MAX__ 255 
//Placeholder MAcros
#define EMPTY_VALUE ""

//Compat Macros
#if defined(__AVX__)
#define AVX_FUNC_COMPAT 1
#else
#define AVX_FUNC_COMPAT 0
#endif

//Pragmas
#pragma GCC target("avx2,bmi,bmi2")

//Macros
#define setErr(err) errVal=err;goto Done
#define cleanErr(err) errVal=err;goto Clean
#define setBit1(val,bitNum) val = val | (1<<bitNum)
#define setIntBit0(val,bitNum) val = val & (0xFFFFFFFF ^ (1<<bitNum))
#define getBit(val,bitNum) (val & (1<<bitNum))

//Typdefs

typedef unsigned char bit8_t; // To prevent collision with <stdint.h> type uint8_t

#if __SIZEOF_LONG__==8
typedef unsigned long mask_t;
#define MASK_SIZE 8
#elif __SIZEOF_LONG_LONG__==8
typedef unsigned long long mask_t;
#define MASK_SIZE 8
#elif __SIZEOF_LONG_LONG__==4
typedef unsigned long long mask_t;
#define MASK_SIZE 4
#else
#error "Unable to set up mask as long long is less than 4 bytes"
#endif

//Enum Typedefs

//Library Structs
typedef struct jsonArrayData {
    void *arrayHolder;
    int arraySize;
    mask_t *mask; //Array Mask | 1-isObject | 0-string 
}jArray_t;

union valueData {
    char* stringPtr;
    jArray_t* arrayPtr;
    jsonObj_t* objectPtr;  
};

typedef struct data {
    char *key;
    value_t val;
    valueType_t valType;
    unsigned int objIndex; //The index of array with object is 1
}data_t;

typedef struct jsonLineData {
    bit8_t propertyKeyStart;  
    bit8_t propertyKeyEnd;
    bit8_t valueStart;
    bit8_t valueEnd;
    bool isArray;
    bool isObject;
    bool isEnd;
}lineData_t;

struct jsonObj {
    data_t **jsonData;
    int propertyCount;
    int maxSize;
    bit8_t stepSize;
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
    "CJ_ERR_NOCOMPAT: Library is incompatible with your device",
    "CJ_ERR_FULL: JSON object is full. File reading terminated.",
    "CJ_ERR_FAIL_RESIZE: JSON object Failed to resize. Max size reached.",
    "CJ_ERR_FAIL_OBJ_WR: Failed to write data to json object",
    "CJ_ERR_DATACOR: Data in structure is corrupted"
    "CJ_ERR_INVCODE: Invalid error code",
};

/*
==============================================================================================================================
                                               INTERNAL FUNCTIONS
==============================================================================================================================
*/

//SIMD based custom strlen() [more inefficient than the strlen() in <string.h> prolly] | Assumes: all passed pointers have memory equivalent to nearest multiple of MAX_VECTOR_SIZE__
static int sslenAligned(const char* str) {
    __m256i nullVec = _mm256_setzero_si256();
    unsigned int len=0,mask=0;
    do {
        __m256i data = _mm256_loadu_si256((const __m256i*)(str+len));
        mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(data,nullVec));
        len = (mask==0)?len:len+MAX_VECTOR_SIZE__;
    } while (mask==0 && len<MAX_READBUFFER_SIZE__);
    len+=__builtin_ctz(mask);
    return (int)len;
}

//SIMD based deep copy [not as good as memcpy()] | Assumes: all passed pointers have memory equivalent to multiple of MAX_VECTOR_SIZE__
static bool deep32ncpy(const char* restrict srcStr, char* restrict destStr, int byteNum) {
    int iteCount = (byteNum%MAX_VECTOR_SIZE__==0)?byteNum/MAX_VECTOR_SIZE__:(byteNum/MAX_VECTOR_SIZE__)+1;
    
}

//Copies N bytes from src to dest as efficiently as possible
static void deepNCopy(const char* restrict src, char* restrict dest, int N) {
    int cnt=0;

    #ifdef ZMM_SIZE
    while ((N-cnt)/ZMM_SIZE>0) {
        __m512i srcBuffer64 = _mm512_loadu_si512(src+cnt);
        _mm512_storeu_si512(dest+cnt,srcBuffer64);
        cnt+=ZMM_SIZE;
    }
    #endif

    #ifdef YMM_SIZE
    while ((N-cnt)/YMM_SIZE>0) {
        __m256i srcBuffer32 = _mm256_loadu_si256((const __m256i_u*)(src+cnt));
        _mm256_storeu_si256((__m256i_u*)(dest+cnt),srcBuffer32);
        cnt+=YMM_SIZE;
    }
    #endif

    #ifdef XMM_SIZE
    while ((N-cnt)/XMM_SIZE>0) {
        __m128i srcBuffer16 = _mm_loadu_si128((const __m128i_u*)(src+cnt));
        _mm_storeu_si128((__m128i_u*)(dest+cnt),srcBuffer16);
        cnt+=XMM_SIZE;
    }
    #endif

    while ((N-cnt)>0) {
        *(dest+cnt)=*(src+cnt);
        ++cnt;
    }
}

//Use SIMD to efficiently parse the 256 byte buffer of fgets() and fill the lineData_t struct with relevant info
static cjson_err_t parseBufferLine(char* buffer, lineData_t* lineData, bool isActiveArray, bool isNestedObject) {
    cjson_err_t errVal=CJ_ERR_OK;

    lineData->isArray=isActiveArray;
    lineData->isObject=isNestedObject;
    lineData->propertyKeyEnd=BIT8_T_MAX__;
    lineData->valueEnd=BIT8_T_MAX__;
    int bufferSize=sslenAligned(buffer), cnt=0, iteration=0, indexHolder=0, colonIndex=0, commaIndex=0;
    unsigned int dQuotesMask=0, colonMask=0, commaMask=0;

    //Singular Init for constantly used vectors
    static __m256i_u doubleQuotesArr, colonArr, commaArr;
    static bool isInit;
    if (!isInit) {
        doubleQuotesArr = _mm256_set_epi64x(DOUBLE_QUOTES_64B,DOUBLE_QUOTES_64B,DOUBLE_QUOTES_64B,DOUBLE_QUOTES_64B);
        colonArr = _mm256_set_epi64x(COLON_64B,COLON_64B,COLON_64B,COLON_64B);
        commaArr = _mm256_set_epi64x(COMMA_64B,COMMA_64B,COMMA_64B,COMMA_64B);
        //Add square Bracket and Curly bracket masks to check if passed value is an array or nested obj (prolly v0.2)
    }

    while (bufferSize>=cnt) {
        __m256i_u bufferVec = _mm256_loadu_si256((__m256i_u*)(buffer+cnt));
        dQuotesMask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(bufferVec,doubleQuotesArr));
        colonMask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(bufferVec,colonArr));
        commaMask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(bufferVec,commaArr));

        if (dQuotesMask==0 || colonMask==0 || commaMask==0) {setErr(CJ_ERR_INVFRMT);} //Again once Array and Obj check is implemented we cant use the comma mask = 0 clause

        while (dQuotesMask!=0) {
            indexHolder=__builtin_ctz(dQuotesMask)+cnt;
            setIntBit0(dQuotesMask,indexHolder);
            
            //Escape character check.
            if (*(buffer+cnt+indexHolder-1)=='\\') {continue;}

            switch(iteration) {
                case 0: 
                lineData->propertyKeyStart=cnt+indexHolder+1;
                break;

                case 1: 
                lineData->propertyKeyEnd=cnt+indexHolder;
                break;

                case 2: 
                lineData->valueStart=cnt+indexHolder+1;
                break;

                case 3: 
                lineData->valueEnd=cnt+indexHolder;
                break;

                default:
                fprintf(stderr,"Faced a logical error\nWARNING:Data provided by the parser is corrupted/invalid.\n");
                break;
            }
            iteration++;
        }

        //This is purely for input validation
        while (colonMask!=0 && colonIndex==0 && lineData->propertyKeyEnd!=BIT8_T_MAX__) {
            indexHolder=__builtin_ctz(colonMask)+cnt;
            setIntBit0(colonMask,indexHolder);

            if (indexHolder>lineData->propertyKeyEnd && indexHolder<lineData->valueStart) {
                colonIndex=indexHolder;
                break;
            }
        }

        while (commaMask!=0 && commaIndex==0 && lineData->valueEnd!=BIT8_T_MAX__) {
            indexHolder=__builtin_ctz(colonMask)+cnt;
            setIntBit0(commaMask,indexHolder);

            if (indexHolder>lineData->valueEnd) {
                commaIndex=indexHolder;
                break;
            }
        }

        cnt+=32;
    }

    //To verify the Colon and comma are in correct place. This wont hold once we have nested objects.
    if (colonIndex==0 || commaIndex==0) {setErr(CJ_ERR_INVFRMT);}

    //Put a check to if array or nested object ends
    lineData->isEnd=true;

    Done:
    return errVal;
}

static cjson_err_t resizeObject(jsonObj_t* objPtr) {
    cjson_err_t errVal = CJ_ERR_OK;

    data_t **temp = realloc(objPtr->jsonData,(objPtr->maxSize+objPtr->stepSize)*(sizeof(data_t*)));
    if (temp==NULL) {setErr(CJ_ERR_INIT_FAIL);}

    objPtr->jsonData=temp;
    objPtr->maxSize+=objPtr->stepSize;

    Done:
    return errVal;
}

static inline void createDataObject(data_t** objectAddress, lineData_t* bufferInfo, char* buffer, int index) {

    *objectAddress=NULL;

    data_t *newObject = malloc(sizeof(data_t));
    if (newObject==NULL) {goto Done;}


    newObject->key=(bufferInfo->propertyKeyStart==bufferInfo->propertyKeyEnd)?EMPTY_VALUE:calloc(bufferInfo->propertyKeyEnd-bufferInfo->propertyKeyStart+1,1);
    if (newObject->key==NULL) {goto Clean;}
    deepNCopy(buffer+bufferInfo->propertyKeyStart,newObject->key,bufferInfo->propertyKeyEnd-bufferInfo->propertyKeyStart);

    //Doesn't support array of objects and incomplete logic build for handling arrays and objects. It is like place holder for now
    if (bufferInfo->isArray==true) {
        //Incomplete logic. Need a check for array start and end and increase the property count as per it
        newObject->valType=ARRAY;

    } else  if (bufferInfo->isObject==true) {
        //Incomplete logic. Need a check for object start and end and increase the property count as per it
        newObject->valType=OBJECT;
    } else {
        newObject->valType=STRING;
        newObject->val.stringPtr=(bufferInfo->valueStart==bufferInfo->valueEnd)?EMPTY_VALUE:calloc(bufferInfo->valueEnd-bufferInfo->valueStart+1,1);
        if (newObject->val.stringPtr==NULL) {goto Clean;}
        deepNCopy(buffer+bufferInfo->valueStart,newObject->val.stringPtr,bufferInfo->valueEnd-bufferInfo->valueStart);
    }

    newObject->objIndex=index;

    *objectAddress=newObject;
    goto Done;

    Clean:
    if (newObject->key!=NULL) {free(newObject->key);}
    free(newObject);
    Done:
    return;
}

static inline cjson_err_t printObj(data_t *obj) {
    cjson_err_t errVal=CJ_ERR_OK;
    printf("Key: %s\n");
    
    switch(obj->valType) {
        case STRING:
        printf("Value: %s\n",obj->val.stringPtr);
        break;


        case ARRAY:
        //TBA
        break;


        case OBJECT:
        if ((errVal=printJsonObj(obj->val.objectPtr,-1))!=CJ_ERR_OK) {
            printError(errVal);
            goto Done;
        }
        break;


        default:
        printError(CJ_ERR_DATACOR);
        setErr(CJ_ERR_DATACOR);
        break;
    }

    Done:
    return;
}

//deep copies pointer and returns 0 on success and 1 on failure to allocate mem
static inline bit8_t deepCopyPointer(const bit8_t* restrict srcPtr, bit8_t* restrict destPtr, int size) {
    bit8_t errVal=0;

    destPtr=malloc(size);
    if (destPtr==NULL) {setErr(1);}

    deepNCopy(srcPtr,destPtr,size);

    Done:
    return errVal;
}



/*
==============================================================================================================================
                                               EXTERNAL FUNCTIONS
==============================================================================================================================
*/

//Checks compatibilty to see if it can run AVX and SSE commands for SIMD
void CJsonCompat() {
    if (AVX_FUNC_COMPAT==0) {
        printf("This library uses AVX Functions which arent supported by this device. Hopefully in next upgrade cross compatibility is added\n");
        exit(CJ_ERR_NOCOMPAT);
    } else if (MAX_READBUFFER_SIZE__%32!=0) {
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

    if (propertyCount>MAX_PROPERTY_COUNT__) {setErr(CJ_ERR_INVSIZE);} 

    jsonObj_t *newObj=malloc(sizeof(jsonObj_t));
    if (newObj==NULL) {setErr(CJ_ERR_INIT_FAIL);}
    
    newObj->stepSize=(isDynamic)?RESIZE_STEP:0;
    newObj->isDynamic=isDynamic;
    newObj->maxSize=propertyCount;
    newObj->propertyCount=0;
    newObj->jsonData=malloc(sizeof(void*)*propertyCount);
    if (newObj->jsonData==NULL) {free(newObj);newObj=NULL;setErr(CJ_ERR_INIT_FAIL);}
    *dataObj=newObj;

    Done:
    return errVal;
}

//load data from json file to json obj passed here
cjson_err_t readJsonFile(char* filepath, jsonObj_t* dataObj) {
    cjson_err_t errVal=CJ_ERR_OK;

    //Static Alloc
    bool isArray=false, isObject=false;

    //File Buffer
    FILE *jsonFilePtr=fopen(filepath,"r");
    if (jsonFilePtr==NULL) {setErr(CJ_ERR_INVFILE);} 

    //Dynamic Alloc
    char *readBuffer=malloc(MAX_READBUFFER_SIZE__+1);
    if (readBuffer==NULL) {fclose(jsonFilePtr);setErr(CJ_ERR_INIT_FAIL);}
    lineData_t *bufferData = malloc(sizeof(lineData_t));
    if (bufferData==NULL) {cleanErr(CJ_ERR_INIT_FAIL);}

    fgets(readBuffer,2,jsonFilePtr); //Assumes first char is '{'
    if (*readBuffer!='{') {cleanErr(CJ_ERR_INVFRMT);} 
    
    while (!feof(jsonFilePtr)) {
        data_t * currentData = (*((dataObj->jsonData)+(dataObj->propertyCount)));

        fgets(readBuffer,MAX_READBUFFER_SIZE__,jsonFilePtr);

        //Input Validation Check
        if (*readBuffer=='\n' || *readBuffer=='\0') {continue;} //Handles Empty lines and first \n after the '{'
        if (*readBuffer=='}') {goto Clean;}

        //Object Size Validation Check
        if (dataObj->propertyCount==dataObj->maxSize) {
            if (!(dataObj->isDynamic)) {
                cleanErr(CJ_ERR_FULL);
            } else {
                if (resizeObject(dataObj)!=CJ_ERR_OK) {cleanErr(CJ_ERR_FAIL_RESIZE);}
            }
        }

        parseBufferLine(readBuffer,bufferData,isArray,isObject);

        createDataObject(&currentData,bufferData,readBuffer,dataObj->propertyCount);

        if (currentData==NULL) {cleanErr(CJ_ERR_FAIL_OBJ_WR);}

        if (bufferData->isEnd==true) {++(dataObj->propertyCount);}
    }


    Clean:
    fclose(jsonFilePtr);
    free(readBuffer);
    readBuffer=NULL;
    Done:
    if (jsonFilePtr!=NULL) {fclose(jsonFilePtr);}
    return errVal;
}

cjson_err_t destroyJsonObj(jsonObj_t* dataObj);

//Set default stepsize for dynamic resizing to N
cjson_err_t setResizeStep(jsonObj_t* dataObj, int stepSize) {
    cjson_err_t errVal =CJ_ERR_OK;
    
    if (dataObj->isDynamic==false) {setErr(CJ_ERR_INVARG);}
    dataObj->stepSize=stepSize;

    Done:
    return errVal;
}

//Set the object to dynamic size with stepsize N
cjson_err_t setDynamicSize(jsonObj_t* dataObj, int stepSize) {
    cjson_err_t errVal =CJ_ERR_OK;
    
    dataObj->isDynamic=true;
    dataObj->stepSize=stepSize;

    Done:
    return errVal;
}

//Fix the Object to current size
cjson_err_t unsetDynamicSize(jsonObj_t* dataObj) {
    cjson_err_t errVal =CJ_ERR_OK;
    
    dataObj->isDynamic=false;
    dataObj->stepSize=0;

    Done:
    return errVal;
}

//Read print the key-value pair at index. If index is passed as -1, it will print all values
cjson_err_t printJsonProperty(jsonObj_t* dataObj, int index) {
    cjson_err_t errVal=CJ_ERR_OK;

    //Verify If a valid index
    if (index>dataObj->propertyCount) {setErr(CJ_ERR_INVARG);}

    switch(index) {
        case -1:
        for (int i=0;i<dataObj->propertyCount;i++) {
            if ((errVal=printObj(*(dataObj->jsonData+i)))!=CJ_ERR_OK) {
                goto Done;
            }
        }
        break;


        default:
        data_t *obj=*(dataObj->jsonData+index-1);
        if (obj->objIndex!=index) {setErr(CJ_ERR_DATACOR);}

        errVal=printObj(obj);
        break;
    }

    Done:
    return errVal;
}

//Load the keyvalue pair of the given index to the passed pointer to an initialised kv_t variable
cjson_err_t getJsonProperty(jsonObj_t* dataObj, int index, kv_t* dataHolder) {
    cjson_err_t errVal=CJ_ERR_OK;

    if (index>dataObj->propertyCount && index>0) {setErr(CJ_ERR_INVARG);}

    data_t *obj= *(dataObj->jsonData+index-1);

    if (deepCopyPointer(obj->key,dataHolder->key,sslenAligned(obj->key)+1)!=0) {setErr(CJ_ERR_INIT_FAIL);}
    dataHolder->value=obj->val;
    dataHolder->valueType=obj->valType;

    Done:
    return errVal;
}

//link the member of the Json Object at given index to the initialised lkv_t variable. Index starts at 1. 
cjson_err_t linkJsonProperty(jsonObj_t* dataObj, int index, lkv_t* dataHolder) {
    cjson_err_t errVal=CJ_ERR_OK;

    if (index>dataObj->propertyCount && index>0) {setErr(CJ_ERR_INVARG);}

    data_t *obj= *(dataObj->jsonData+index-1);

    dataHolder->key=obj->key;
    dataHolder->value=&obj->val;
    dataHolder->valueType=&obj->valType;

    Done:
    return errVal;
}