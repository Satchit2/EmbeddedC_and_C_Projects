#include <stdio.h>
#include <stdlib.h>
#include "CJson.h"

int main() {
    jsonObj configData;
    char *filepath=malloc(64); //Assuming Max file Name is 64 Char
    printf("Enter the File Path(relative): ");
    fgets(filepath,64,STDOUT_FILENO);
    int err=readJsonFile(filepath,&configData);
    if (err!=0) {
        printf("The Json parser returned error!");
    } 
    return 0;
}