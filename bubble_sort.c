#include <stdio.h>

int main()
{
    
    int array[] = {5,3,6,8,9,1,4,7,2,10};
    int n = 9;
    
    
    for (int i = 0; i < 9; i++)
    {
        for(int j = 0; j < 9 - i; j++)
        {
            if(array[j] > array[j+1])
            {
                int temp = array[j];
                array[j] = array[j+1];
                array[j+1] = temp;
            }
        }
    }
    
    for (i = 0; i < n ;i++)
    {
        printf("%d ", array[i]);
    }
    return 0;
}
