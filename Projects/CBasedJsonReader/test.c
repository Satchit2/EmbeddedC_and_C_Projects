#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Include/CJson.h"
#include "Include/privateInfo.h"

int main() {
    CJsonCompat();
    jsonObj_t *configData;
    printError(createJsonObj(&configData,3,0));  

    printError(readJsonFile(FILELOC,configData));  
    return 0;
}