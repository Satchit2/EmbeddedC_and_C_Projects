#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>

int main(){
    int t; 
    scanf("%d",&t);
    for(int a0 = 0; a0 < t; a0++){
        long n,currVal=34,prevVal=21,temp;
        unsigned long sum = 10; 
        scanf("%ld",&n);
        while (n>=currVal) {
            sum+=currVal;
            temp=currVal;
            currVal=currVal*3 + prevVal*2;
            prevVal=temp*2 +prevVal;
        }
        printf("%lu\n",sum);
    }
    return 0;
}