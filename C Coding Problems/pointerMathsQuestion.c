#include <stdio.h>
#include <stdlib.h>

//Complete the following function.
//Hacker Rank Question

int marks_summation(int* marks, int number_of_students, char gender) {
    int i = (gender=='b') ? 0 : 1 ;
    int totMarks=0;
    for (i;i<number_of_students;i=i+2) {
        totMarks= totMarks + *(marks+i);
    }
    marks=marks-number_of_students-((i+1)%2);
    return totMarks;
}

int main() {
    int number_of_students;
    char gender;
    int sum;
  
    scanf("%d", &number_of_students);
    int *const marks = (int *) malloc(number_of_students * sizeof (int));
 
    for (int student = 0; student < number_of_students; student++) {
        scanf("%d", (marks + student));
    }
    
    scanf(" %c", &gender);
    sum = marks_summation(marks, number_of_students, gender);
    printf("%d", sum);
    free(marks);
 
    return 0;
}