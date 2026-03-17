#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

void print_fibo(int num)
{
    printf("%d\n", fibonacci(num));
}

void print_fibo_and_max(int *nums)
{
    printf("%d %d\n", fibonacci(nums[0]), max_of_four_int(nums[0], nums[1], nums[2], nums[3]));
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        int num = atoi(argv[1]);
        print_fibo(num);
    }
    else if (argc == 5)
    {
        int nums[4];
        for (int i = 0; i < 4; i++)
            nums[i] = atoi(argv[i + 1]);
        print_fibo_and_max(nums);
    }
    else
    {
        printf("Usage: %s <number> or %s <num1> <num2> <num3> <num4>\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}