#ifndef CJSON_PARSER_H
#define CJSON_PARSER_H 1

#define printError(errorCode) printError__(errorCode,__LINE__,__func__,__FILE_NAME__)

//Public Size Macros
#define RESIZE_STEP 2

/*
Requirements: compile the code with flag -march=native when using gcc and use function CJsonCompat() at start to ensure that your device can run the CJson lib
*/

//Compat Macros
//Compile Time Intrinsic Check
#ifdef __AVX512__
#define ZMM_SIZE 64
#endif

//Similar Macros to be set up for YMM and XMM later
#define YMM_SIZE 32
#define XMM_SIZE 16

//Check Max Vector Support
#ifdef ZMM_SIZE
//AVX512 Support
#define MAX_VECTOR_SIZE__ 64
#elifdef YMM_SIZE
//AVX2 / AVX support
#define MAX_VECTOR_SIZE__ 32
#elifdef XMM_SIZE
//SSE Support
#define MAX_VECTOR_SIZE__ 16
#else
#define MAX_VECTOR_SIZE__ 0
#error "Vector Functions not supported"
#endif

#if defined(__AVX2__) && defined(__SSE4_1__)
#define LIB_COMPAT 1
#else
#define LIB_COMPAT 0
#warning "This device doesn't support AVX2 and SSE4.1 Intrinsics used in the library" //SET TO ERROR, POST TESTING
#endif


/*
==============================================================================================================================
                                        DATA DECLARATIONS & DEFINITION
==============================================================================================================================
*/

//Error Enum
typedef enum cjson_err {
    CJ_ERR_OK,
    CJ_ERR_INVARG,
    CJ_ERR_INIT_FAIL,
    CJ_ERR_INVFILE, //It may be triggered even if file pointer fails to init.
    CJ_ERR_INVFRMT,
    CJ_ERR_INVSIZE,
    CJ_ERR_NOCOMPAT,
    CJ_ERR_FULL,
    CJ_ERR_FAIL_RESIZE,
    CJ_ERR_FAIL_OBJ_WR, //Failed Object write
    CJ_ERR_DATACOR, //Corruption of json obj
    CJ_ERR_INVCODE,
}cjson_err_t;

//Value Type Enum
typedef enum valueTypes {
    STRING,
    ARRAY,
    OBJECT
}valueType_t;

//Opaque Type Struct
typedef struct jsonObj jsonObj_t;
typedef union valueData value_t;

//Public Type Struct

typedef struct keyValuePair {
    char* key;
    value_t value;
    valueType_t valueType;
}kv_t;

typedef struct linkedKeyValuePair {
    char* key;
    value_t* value;
    valueType_t* valueType;
}lkv_t;


//======================================== Errror Display Functions ========================================

//Error Display Function mapped from macro. It prints error statement to STDOUT and returns verbose error. Entering invalid errno is 
extern const char* printError__(cjson_err_t errorCode, int lineNum, const char* funcName, const char* fileName);
//It doesnt print any error message and just returns verbose error
extern const char* getErrorV(cjson_err_t errorCode);

//Compatibility Functions
extern void CJsonCompat();

/*
==============================================================================================================================
                                              CJSON FUNCTIONS
==============================================================================================================================
*/

//======================================== Object Creation Functions ========================================

//Create json obj to be used in the project. If isDynamic is 0 then the property count will be fixed and if the number of properties mismatch between json file and object 
//can cause unintended behaviour when using readJsonObj like memory corruption. The memory step-size of dynamic realloc can be changed by defining step size using DYNAMIC_STEP.
extern cjson_err_t createJsonObj(jsonObj_t** dataObj, int propertyCount, bool isDynamic);
//Free all the memeory associated with the json obj
extern cjson_err_t destroyJsonObj(jsonObj_t* dataObj);



//========================================= Object Status Functions ==========================================

//Set default stepsize for dynamic resizing to N
extern cjson_err_t setResizeStep(jsonObj_t* dataObj, int stepSize);
//Set the object to dynamic size with stepsize N
extern cjson_err_t setDynamicSize(jsonObj_t* dataObj, int stepSize);
//Fix the Object to current size
extern cjson_err_t unsetDynamicSize(jsonObj_t* dataObj);



//======================================== Object Processing Functions ========================================

//Load a .json file into a program interactable C structure
extern cjson_err_t readJsonFile(char* filepath, jsonObj_t* dataObj);
//Read print the key-value pair at index. If index is passed as -1, it will print all values. Index starts at 1
extern cjson_err_t printJsonProperty(jsonObj_t* dataObj, int index);
//Deep copy the keyvalue pair of the given index to the passed pointer to an initialised kv_t variable. Index starts at 1. 
extern cjson_err_t getJsonProperty(jsonObj_t* dataObj, int index, kv_t* dataHolder);
//link the member of the Json Object at given index to the initialised lkv_t variable. Index starts at 1. 
extern cjson_err_t linkJsonProperty(jsonObj_t* dataObj, int index, lkv_t* dataHolder);

#endif