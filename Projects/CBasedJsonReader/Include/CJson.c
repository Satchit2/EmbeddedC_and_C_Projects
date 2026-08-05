#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "CJson.h"

//Global Defn
#define DYNAMIC_STEP 2
#define MAX_PROPERTY_COUNT__ 32
#define MAX_READBUFFER_SIZE__ 128

//Macros
#define setErr(err) { \
    errVal=err;\
    goto Done;\
}
#define setIntBit(val,bitNum) val | (1<<bitNum)
#define getIntBit(val,bitNum) (val & (1<<bitNum))

//Library Structs
typedef struct data {
    char *key;
    char *val;
}data_t;
struct jsonObj {
    void **jsonData;
    int depth; //use 1 bit/property count [for now compatible or json file with maximum of 32 properties]
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
    "CJ_ERR_INVCODE: Invalid error code"
};

//Internal Functions


//Public Functions

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
    newObj->depth=0;
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
    for (localPropertyCount;localPropertyCount<dataObj->maxSize && feof(jsonFilePtr);) {
        fgets(readBuffer,MAX_READBUFFER_SIZE__,jsonFilePtr);
        if (!localPropertyCount && *readBuffer!='{') {setErr(CJ_ERR_INVFRMT)} 
        else {fgets(readBuffer,MAX_READBUFFER_SIZE__,jsonFilePtr);}
        // Identify if it is a property or another object
        // Enter the data to json object
    }



    Done:
    return errVal;
}

cjson_err_t destroyJsonObj(jsonObj_t* dataObj);