#ifndef CJSON_PARSER_H
#define CJSON_PARSER_H 1

#include "CJson.c"

//Opaque Type Struct
typedef struct jsonObj jsonObj_t;

//Public Functions
//Load a .json file into a program interactable C structure
int readJsonFile(char* filepath, jsonObj_t* dataObj);

#endif