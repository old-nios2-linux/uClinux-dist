#include <stdio.h>

#define TEST_SCRATCHPAD_LOW      0xffb00000
#define TEST_SCRATCHPAD_HIGH     0xffb01000


int main(int argc, char *argv[])
{
        char *str;
        str="Hello world!";
	printf("%s, %p\n", str,&str);

	if (((unsigned long)&str > TEST_SCRATCHPAD_HIGH) || ((unsigned long)&str < TEST_SCRATCHPAD_LOW)) {
		printf("        TEST FAIL, stack address not in L1\n");
	} else {
		if (strstr(argv[0], "helloworld"))
			printf("        TEST PASS, %s\n", argv[0]);
		else
			printf("        TEST FAIL, argv is wrong\n");
	}
}

