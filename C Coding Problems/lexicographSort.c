#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Hacker Rank Hard. Lexicographic sorting of strings using a double pointer and quick sort. 

int lexicographic_sort(const char* a, const char* b) {
    int charPos = 0;
    while((*(a+charPos) == *(b+charPos)) && (*(a+charPos) > 96)) {
        charPos++;
    }
    if (*(a+charPos) < *(b+charPos)) {
        return 1;
    } else {return 0;}
}

int lexicographic_sort_reverse(const char* a, const char* b) {
    int charPos = 0;
    while((*(a+charPos) == *(b+charPos)) && (*(a+charPos) > 96)) {
        charPos++;
    }
    if (*(a+charPos) > *(b+charPos)) {
        return 1;
    } else {return 0;}
}

int sort_by_number_of_distinct_characters(const char* a, const char* b) {
    int distChar1=0, distChar2=0, dupli=0,benchmark;
    char distCharList[26];
    memset(distCharList,0,26);
    char* const charListPtr = distCharList;
    for (int i=0; *(a+i)>96;i++) {
        dupli=0;
        for (int j=0;j<strlen(distCharList);j++) {
            if (*(a+i)==*(charListPtr+j)) {
                dupli=1;
                break;
            }
        }
        if (dupli) {continue;}
        else {
            *(charListPtr+(strlen(distCharList)))= *(a+i);
        }
        if (strlen(distCharList)==26) {
            break;
        }
    }
    benchmark = strlen(distCharList);
    memset(distCharList,0,26);
    for (int i=0; *(b+i)>96;i++) {
        dupli=0;
        for (int j=0;j<strlen(distCharList);j++) {
            if (*(b+i)==*(charListPtr+j)) {
                dupli=1;
                break;
            }
        }
        if (dupli) {continue;}
        else {
            *(charListPtr+(strlen(distCharList)))= *(b+i);
        }
        if ((strlen(distCharList)>benchmark) || (strlen(distCharList)==26)) {
            break;
        }
    }
    if (benchmark<strlen(distCharList)) {return 1;}
    else if (benchmark==strlen(distCharList)) {return lexicographic_sort(a,b);}
    else {return 0;}
}

int sort_by_length(const char* a, const char* b) {
    if (strlen(a)<strlen(b)) {return 1;}
    else if (strlen(a)==strlen(b)) {return lexicographic_sort(a,b);}
    else {return 0;}
}

void swap(char** arr, int pos1, int pos2) {
    if (pos1==pos2) {return;}
    char* strHolder = *(arr+pos2);
    *(arr+pos2)=*(arr+pos1);
    *(arr+pos1)=strHolder;
}

void string_sort(char** arr,const int len,int (*cmp_func)(const char* a, const char* b)){
    if (len>1) {
        int pivotPos = len-1;
        char* cmpVal = *(arr+pivotPos);
        int lowPos=-1, i=0;
        while (i<len) {
            if (cmp_func(*(arr+i),cmpVal)) {
                swap(arr,i,lowPos+1);
                lowPos++;
                i++;
            } 
            else {i++;}
        }
        swap(arr,pivotPos,lowPos+1);
        string_sort(arr,lowPos+1,cmp_func);
        string_sort(&(*(arr+lowPos+2)),(len-lowPos-2),cmp_func);
    }
}


int main() 
{
    int n;
    scanf("%d", &n);
  
    char** arr;
	arr = (char**)malloc(n * sizeof(char*));
  
    for(int i = 0; i < n; i++){
        *(arr + i) = malloc(1024 * sizeof(char));
        scanf("%s", *(arr + i));
        *(arr + i) = realloc(*(arr + i), strlen(*(arr + i)) + 1);
    }
  
    string_sort(arr, n, lexicographic_sort);
    for(int i = 0; i < n; i++)
        printf("%s\n", arr[i]);
    printf("\n");

    string_sort(arr, n, lexicographic_sort_reverse);
    for(int i = 0; i < n; i++)
        printf("%s\n", arr[i]); 
    printf("\n");

    string_sort(arr, n, sort_by_length);
    for(int i = 0; i < n; i++)
        printf("%s\n", arr[i]);    
    printf("\n");

    string_sort(arr, n, sort_by_number_of_distinct_characters);
    for(int i = 0; i < n; i++)
        printf("%s\n", arr[i]); 
    printf("\n");
}