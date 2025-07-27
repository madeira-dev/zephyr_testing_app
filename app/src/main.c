#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Include the external hello_lib header
#include "core/lattice/element.h"
#include "core/lattice/ring_element.h"
#include "core/math/biginteger.h"
#include "core/math/math_hal.h"
#include "core/math/polynomial.h"

// Define a logging module for this test app (different name to avoid conflicts)
LOG_MODULE_REGISTER(test_app, LOG_LEVEL_DBG);

int main(void)
{
    LOG_INF("=== Hello Library Test Application ===");
    LOG_INF("Testing external hello_lib functions...");

    // Call the external library function to test it
    hello_lib_say_hello();

    LOG_INF("=== Test completed ===");

    return 0;
}