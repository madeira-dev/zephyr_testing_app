// #include <zephyr/kernel.h>
// #include <zephyr/logging/log.h>

// // Include the external hello_lib header
// #include "core/lattice/element.h"
// #include "core/lattice/ring_element.h"
// #include "core/math/biginteger.h"
// #include "core/math/math_hal.h"
// #include "core/math/polynomial.h"

// // Define a logging module for this test app (different name to avoid conflicts)
// LOG_MODULE_REGISTER(test_app, LOG_LEVEL_DBG);

// // Test functions for math components
// static void test_bigint_basic(void)
// {
//     LOG_INF("=== Testing BigInteger Basic Operations ===");

//     bigint_t a, b, result;

//     // Initialize
//     bigint_init(&a);
//     bigint_init(&b);
//     bigint_init(&result);

//     // Test initialization with values
//     bigint_init_u64(&a, 12345);
//     bigint_init_u64(&b, 6789);

//     LOG_INF("a initialized to 12345");
//     LOG_INF("b initialized to 6789");

//     // Test basic operations
//     if (bigint_add(&result, &a, &b) == 0)
//     {
//         LOG_INF("Addition test: a + b = %u (expected ~19134)",
//                 (uint32_t)result.words[0]);
//     }
//     else
//     {
//         LOG_ERR("Addition test failed");
//     }

//     if (bigint_mult(&result, &a, &b) == 0)
//     {
//         LOG_INF("Multiplication test: a * b = %llu (expected ~83810205)",
//                 ((uint64_t)result.words[1] << 32) | result.words[0]);
//     }
//     else
//     {
//         LOG_ERR("Multiplication test failed");
//     }

//     // Test comparison
//     if (bigint_compare(&a, &b) > 0)
//     {
//         LOG_INF("Comparison test: a > b (correct)");
//     }
//     else
//     {
//         LOG_INF("Comparison test: a <= b");
//     }

//     LOG_INF("BigInteger basic tests completed");
// }

// static void test_polynomial_basic(void)
// {
//     LOG_INF("=== Testing Polynomial Basic Operations ===");

//     static polynomial_t poly_a, poly_b, poly_result;
//     bigint_t coeff;

//     // Initialize polynomials
//     if (poly_init(&poly_a, 4) != 0)
//     {
//         LOG_ERR("Failed to initialize polynomial A");
//         return;
//     }

//     if (poly_init(&poly_b, 4) != 0)
//     {
//         LOG_ERR("Failed to initialize polynomial B");
//         return;
//     }

//     if (poly_init(&poly_result, 8) != 0)
//     {
//         LOG_ERR("Failed to initialize result polynomial");
//         return;
//     }

//     LOG_INF("Polynomials initialized successfully");

//     // Set some coefficients for poly_a: 3x^2 + 2x + 1
//     bigint_init_u64(&coeff, 1);
//     poly_set_coeff(&poly_a, 0, &coeff); // constant term

//     bigint_init_u64(&coeff, 2);
//     poly_set_coeff(&poly_a, 1, &coeff); // x term

//     bigint_init_u64(&coeff, 3);
//     poly_set_coeff(&poly_a, 2, &coeff); // x^2 term

//     LOG_INF("poly_a = 3x^2 + 2x + 1");

//     // Set coefficients for poly_b: 2x + 5
//     bigint_init_u64(&coeff, 5);
//     poly_set_coeff(&poly_b, 0, &coeff); // constant term

//     bigint_init_u64(&coeff, 2);
//     poly_set_coeff(&poly_b, 1, &coeff); // x term

//     LOG_INF("poly_b = 2x + 5");

//     // Test polynomial addition
//     poly_ring_params_t params;
//     params.ring_dimension = 256;
//     bigint_init_u64(&params.coefficient_modulus, 65537); // Small prime
//     params.modulus_bits = 17;

//     if (poly_add(&poly_result, &poly_a, &poly_b, &params) == 0)
//     {
//         bigint_t result_coeff;
//         poly_get_coeff(&poly_result, 0, &result_coeff);
//         LOG_INF("Addition result: constant term = %u (expected 6)",
//                 (uint32_t)result_coeff.words[0]);

//         poly_get_coeff(&poly_result, 1, &result_coeff);
//         LOG_INF("Addition result: x term = %u (expected 4)",
//                 (uint32_t)result_coeff.words[0]);

//         poly_get_coeff(&poly_result, 2, &result_coeff);
//         LOG_INF("Addition result: x^2 term = %u (expected 3)",
//                 (uint32_t)result_coeff.words[0]);
//     }
//     else
//     {
//         LOG_ERR("Polynomial addition failed");
//     }

//     // Cleanup
//     poly_cleanup(&poly_a);
//     poly_cleanup(&poly_b);
//     poly_cleanup(&poly_result);

//     LOG_INF("Polynomial basic tests completed");
// }

// static void test_math_hal_basic(void)
// {
//     LOG_INF("=== Testing Math HAL Basic Operations ===");

//     // Test modular arithmetic
//     math_word_t a = 123;
//     math_word_t b = 456;
//     math_word_t m = 1000;

//     math_word_t result = math_hal_mod_add(a, b, m);
//     LOG_INF("(%u + %u) mod %u = %u (expected 579)",
//             (uint32_t)a, (uint32_t)b, (uint32_t)m, (uint32_t)result);

//     result = math_hal_mod_mult(a, b, m);
//     LOG_INF("(%u * %u) mod %u = %u (expected 88)",
//             (uint32_t)a, (uint32_t)b, (uint32_t)m, (uint32_t)result);

//     // Test RNG initialization
//     if (math_hal_rng_init() == 0)
//     {
//         LOG_INF("RNG initialized successfully");

//         // Test random bytes generation
//         LOG_INF("About to generate random bytes");
//         uint8_t random_buffer[1];
//         if (math_hal_rng_bytes(random_buffer, sizeof(random_buffer)) == 0)
//         {
//             LOG_INF("Generated 1 random byte");
//         }
//         else
//         {
//             LOG_ERR("Failed to generate random bytes");
//         }
//     }
//     else
//     {
//         LOG_ERR("Failed to initialize RNG");
//     }

//     LOG_INF("Math HAL basic tests completed");
// }

// static void test_memory_usage(void)
// {
//     LOG_INF("=== Memory Usage Analysis ===");

//     LOG_INF("sizeof(bigint_t) = %u bytes", sizeof(bigint_t));
//     LOG_INF("sizeof(polynomial_t) = %u bytes", sizeof(polynomial_t));
//     LOG_INF("sizeof(math_word_t) = %u bytes", sizeof(math_word_t));

//     LOG_INF("POLY_MAX_DEGREE = %d", POLY_MAX_DEGREE);
//     LOG_INF("POLY_MAX_COEFFS = %d", POLY_MAX_COEFFS);
//     LOG_INF("BIGINT_MAX_WORDS = %d", BIGINT_MAX_WORDS);

//     // Calculate memory usage for common operations
//     size_t single_poly_mem = sizeof(polynomial_t);
//     size_t three_poly_mem = 3 * sizeof(polynomial_t);

//     LOG_INF("Memory for 1 polynomial: %u bytes (%.1f%% of 256KB RAM)",
//             single_poly_mem, (single_poly_mem * 100.0) / (256 * 1024));
//     LOG_INF("Memory for 3 polynomials: %u bytes (%.1f%% of 256KB RAM)",
//             three_poly_mem, (three_poly_mem * 100.0) / (256 * 1024));

//     if (three_poly_mem > (256 * 1024 / 10))
//     {
//         LOG_WRN("Memory usage is high - consider reducing POLY_MAX_DEGREE");
//     }
// }

// int main(void)
// {
//     LOG_INF("=== OpenFHE Embedded Math Library Tests ===");
//     LOG_INF("Target: nRF52840DK (ARM Cortex-M4F, 256KB RAM)");

//     // Run memory analysis first
//     test_memory_usage();

//     // Run basic math tests
//     test_math_hal_basic();
//     test_bigint_basic();
//     test_polynomial_basic();

//     LOG_INF("=== All Math Tests Completed ===");

//     // Wait to allow logs to flush (important for Renode/Zephyr)
//     // k_sleep(K_MSEC(3000));

//     return 0;
// }