#include <zephyr/kernel.h>
#include "hello_lib.h" // Include your library's header

int main(void)
{
    // Call your library function
    hello_lib_say_hello();
    return 0;
}