#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>

int main(){
    int t; 
    scanf("%d",&t);
    for(int a0 = 0; a0 < t; a0++){
        long n;
        long long sum=0;         
        scanf("%lu",&n);
        long long lp=(n-1)/15;
        sum= ((60*lp)+(105*((((lp)*(lp-1))/2))));
        for (long long i=(n-((n-1)%15));i<n;i++){
            if (i%3==0 || i%5==0) {sum+=i;}
        }
        printf("%llu\n",sum);
    }
    return 0;
}

/*
Logic Used: We convert number n into block of 15 as the sum for it can be reduced to 60 * i + 105*(i)(i-1)/2
Lets Say n=31,
All mulitples of 3 or 5 upto 31 are:
3+6+9+12+15+18+21+24+27+30 + 5+10+20+25
<-----Multi of 3-------->   <Multi of 5> [15,30 is not incl in multi of 5 as we have already added in multi of 3]
This can be written in block of 15s
3+6+9+12+15 + 5+10  +  18+21+24+27+30 + 20+25
3+6+9+12+15 + 5+10  +  (15+3)+(15+6)+(15+9)+(15+12)+(15+15) + (15+5)+(15+10)
(3+6+9+12+15 + 5+10) + (3+6+9+12+15 + 5+10) + (15*7)
60 + 60+105

This generalized into form mentioned above. After that we calculate the remainder of the values which are not included in the block of 15 one by one.
*/
