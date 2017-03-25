#include <stdio.h>
int main(int argc, char const *argv[])
{
    int a=0,b,c,i;
    for(i=0;i<10;i++)
    {
        b=a+1;
        c=2*i;
    }
    printf("%d\n",c);
    return 0;
}