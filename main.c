#include "queue.c"
#include <stdio.h>

int main()
{
	int testint[10] = {1,2,3,4,5,6,7,8,9,0};

	printf("%ld\n", testint[1]);

	printf("%ld\n", testint);

	printf("%ld\n", &testint);

	printf("%ld\n", &testint[1]);

}