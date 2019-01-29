/* 
 * File:   num_cores.c
 * ID:     676683456
 * Name:   Wentao Hao
 * upi:    hwen398
 */

#include <stdio.h>
#include <unistd.h>

int main()
{
	int num = sysconf(_SC_NPROCESSORS_ONLN);
	printf("This machine has %d cores.\n", num);
}