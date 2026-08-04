#include <stdio.h>
#include <stdlib.h>
#include "CJson.h"

//Library Structs
typedef struct data {
    char *key;
    char *val;
}data;
struct jsonObj {
    void **jsonData;
    int *depth;
};

//Internal Functions


//Public Functions
int readJsonFile(char* filepath, jsonObj_t* dataObj) {

}