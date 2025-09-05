// #include <zephyr/kernel.h>
// #include <zephyr/logging/log.h>
// #include "scheme/ckks/ckks_cryptoparams.h"
// #include "core/math/biginteger.h"
// #include <string.h>
// #include <stdbool.h>
// #include <stdio.h>

// LOG_MODULE_REGISTER(test_ckks_cryptoparams);

// static int tests_passed = 0;
// static int tests_failed = 0;

// static void test_report_assert(const char *name, bool passed, const char *details)
// {
//     if (passed)
//     {
//         tests_passed++;
//         LOG_INF("[PASS] %s", name);
//     }
//     else
//     {
//         tests_failed++;
//         LOG_ERR("[FAIL] %s: %s", name, details);
//     }
// }

// static void assert_eq_u32(const char *name, uint32_t expected, uint32_t actual)
// {
//     char details[64];
//     if (expected != actual)
//     {
//         snprintf(details, sizeof(details), "expected=%u actual=%u", expected, actual);
//         test_report_assert(name, false, details);
//     }
//     else
//     {
//         test_report_assert(name, true, "");
//     }
// }

// static void assert_eq_double(const char *name, double expected, double actual, double tol)
// {
//     char details[64];
//     if (!(fabs(expected - actual) <= tol))
//     {
//         snprintf(details, sizeof(details), "expected=%g actual=%g tol=%g", expected, actual, tol);
//         test_report_assert(name, false, details);
//     }
//     else
//     {
//         test_report_assert(name, true, "");
//     }
// }

// static void assert_eq_str(const char *name, const char *expected, const char *actual)
// {
//     char details[128];
//     if (strcmp(expected, actual) != 0)
//     {
//         snprintf(details, sizeof(details), "expected=\"%s\" actual=\"%s\"", expected, actual);
//         test_report_assert(name, false, details);
//     }
//     else
//     {
//         test_report_assert(name, true, "");
//     }
// }

// int main(void)
// {
//     ckks_cryptoparams_t params;
//     bigint_t moduli[CKKS_MAX_MODULI];

//     // Initialize modulus chain with small values for testing
//     for (uint32_t i = 0; i < CKKS_MAX_MODULI; i++)
//     {
//         bigint_init_u64(&moduli[i], 17 + i); // values: 17, 18 (CKKS_MAX_MODULI == 2)
//     }

//     uint32_t ring_dimension = 8;
//     double scaling_factor = 1 << 20;
//     uint32_t max_depth = 2;
//     uint32_t num_moduli = 2;

//     int ret = ckks_cryptoparams_init(&params, ring_dimension, moduli, num_moduli, scaling_factor, max_depth);
//     if (ret != 0)
//     {
//         LOG_ERR("ckks_cryptoparams_init failed!");
//         return -1;
//     }

//     // Basic value assertions
//     assert_eq_u32("ring_dimension", ring_dimension, params.ring_dimension);
//     assert_eq_u32("num_moduli", num_moduli, params.num_moduli);
//     assert_eq_u32("max_depth", max_depth, params.max_depth);
//     assert_eq_double("scaling_factor", scaling_factor, params.scaling_factor, 1e-9);

//     // Check modulus chain strings against expected values
//     const char *expected_mods[CKKS_MAX_MODULI] = {"17", "18"};
//     char buf[64];
//     for (uint32_t i = 0; i < num_moduli; i++)
//     {
//         bigint_to_string(&params.modulus_chain[i], buf, sizeof(buf));
//         char name[32];
//         snprintf(name, sizeof(name), "modulus[%u]", i);
//         assert_eq_str(name, expected_mods[i], buf);

//         // Also log the modulus and bits for visibility
//         LOG_INF("Modulus %u: %s (bits: %u)", i, buf, params.modulus_bits[i]);
//     }

//     // Sanity check for polynomial params that should mirror ring_dimension and modulus bits
//     assert_eq_u32("poly_params.ring_dimension", params.poly_params.ring_dimension, params.ring_dimension);
//     assert_eq_u32("poly_params.modulus_bits", params.poly_params.modulus_bits, params.modulus_bits[0]);

//     // Cleanup
//     ckks_cryptoparams_cleanup(&params);

//     // Summary
//     LOG_INF("Tests passed: %d, failed: %d", tests_passed, tests_failed);
//     if (tests_failed > 0)
//     {
//         LOG_ERR("One or more tests failed");
//         return -1;
//     }

//     LOG_INF("All basic tests passed");
//     return 0;
// }