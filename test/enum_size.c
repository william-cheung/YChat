#include <stdio.h>
#include <limits.h>

enum enum_test1 { CONS1, CONS2, CONS3 };
enum enum_test2 { CONS4 = 256 };
enum enum_test3 { CONS5 = 65536 };
enum enum_test4 { CONS6 = (long long)INT_MAX + 1 }

int main () {
	printf("%d, %d, %d, %d\n", 
			sizeof(enum enum_test1), sizeof(enum enum_test2), sizeof(enum enum_test3), sizeof(enum enum_test4));
	return 0;
}
