#include <zephyr/kernel.h>
#include "hello_lib.h"

int main(void)
{
    hello_lib_say_hello();
    return 0;
}