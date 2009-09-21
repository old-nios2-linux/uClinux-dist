#include <stdio.h>

int test_func(void)
{
    return 1;
}

int main(void)
{
    int (*pf)(void);

    pf = test_func;
    printf("pf is 0x%08x!\n", (int)pf);

    return 0;
}
