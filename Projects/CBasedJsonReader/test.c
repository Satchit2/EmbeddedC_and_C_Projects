#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Include/CJson.h"

int main() {
    jsonObj_t *configData;
    printError(createJsonObj(&configData,3,0));    
    return 0;
}