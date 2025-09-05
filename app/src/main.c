#include <zephyr/logging/log.h>
#include "scheme/ckks/ckks_cryptoparams.h"
#include "scheme/ckks/ckks_keygen.h"
#include "scheme/ckks/ckks_encoder.h"
#include "scheme/ckks/ckks_encryptor.h"
#include "scheme/ckks/ckks.h"
#include "core/math/polynomial.h"
#include "core/math/biginteger.h"
#include "core/math/math_hal.h"

LOG_MODULE_REGISTER(test_ntt, LOG_LEVEL_INF);

// Helper to print a simple math_word_t array
// void print_array(const char *label, const math_word_t *arr, uint32_t size)
// {
//   printk("%s: [", label);
//   for (uint32_t i = 0; i < size; i++)
//   {
//     printk("%u", (uint32_t)arr[i]);
//     if (i < size - 1)
//     {
//       printk(", ");
//     }
//   }
//   printk("]\n");
// }

// Test NTT init params
// void test_ntt_init(void)
// {
//   ntt_params_t params;
//   uint32_t n = 8;
//   math_word_t modulus = 17; // Small prime for testing: 17-1 = 16, which is divisible by 8

//   // LOG_INF("--- Testing NTT Init (n=%u, mod=%u) ---", n, (uint32_t)modulus);
//   LOG_INF("please tell me anything updated inside test_ntt_init!!!!!!!!");
// int ret = math_hal_ntt_init_params(&params, n, modulus);
// if (ret == 0)
// {
//   LOG_INF("NTT init success: root=%u, inv_root=%u, inv_n=%u",
//           (uint32_t)params.root_of_unity,
//           (uint32_t)params.inv_root_of_unity,
//           (uint32_t)params.inv_n);

//   // Verify root^n ≡ 1 mod modulus
//   math_word_t check = math_hal_mod_pow(params.root_of_unity, n, modulus);
//   if (check == 1)
//   {
//     LOG_INF("Primitive root verification passed (root^n mod m == 1)");
//   }
//   else
//   {
//     LOG_ERR("Primitive root verification failed: %u", (uint32_t)check);
//   }
// }
// else
// {
//   LOG_ERR("NTT init failed: %d", ret);
// }
// LOG_INF("--- End Test NTT Init ---\n");
// }

// Test forward and inverse NTT transform
// void test_ntt_transform(void)
// {
//   ntt_params_t params;
//   uint32_t n = 8;
//   math_word_t modulus = 17;

//   LOG_INF("--- Testing NTT Forward/Inverse Transform (n=%u, mod=%u) ---", n, (uint32_t)modulus);

//   if (math_hal_ntt_init_params(&params, n, modulus) != 0)
//   {
//     LOG_ERR("Failed to init NTT params for transform test.");
//     return;
//   }

//   math_word_t poly[8] = {1, 2, 3, 4, 0, 0, 0, 0};
//   math_word_t original_poly[8];
//   memcpy(original_poly, poly, sizeof(poly));

//   print_array("Original", original_poly, n);

//   // Forward NTT
//   math_hal_ntt_forward(poly, &params);
//   print_array("Forward NTT", poly, n);

//   // Inverse NTT
//   math_hal_ntt_inverse(poly, &params);
//   print_array("Inverse NTT", poly, n);

//   // Verification
//   bool success = true;
//   for (uint32_t i = 0; i < n; i++)
//   {
//     if (poly[i] != original_poly[i])
//     {
//       success = false;
//       break;
//     }
//   }

//   if (success)
//   {
//     LOG_INF("SUCCESS: Inverse transform matches original polynomial.");
//   }
//   else
//   {
//     LOG_ERR("FAILURE: Inverse transform does not match original.");
//   }
//   LOG_INF("--- End Test NTT Transform ---\n");
// }

// Test polynomial multiplication using NTT
// void test_ntt_multiplication(void)
// {
//   ntt_params_t params;
//   uint32_t n = 8;
//   math_word_t modulus = 17;

//   LOG_INF("--- Testing NTT Multiplication (n=%u, mod=%u) ---", n, (uint32_t)modulus);

//   if (math_hal_ntt_init_params(&params, n, modulus) != 0)
//   {
//     LOG_ERR("Failed to init NTT params for multiplication test.");
//     return;
//   }

//   // Polynomials a(x) = 1 + 2x and b(x) = 3 + x
//   math_word_t poly_a[8] = {1, 2, 0, 0, 0, 0, 0, 0};
//   math_word_t poly_b[8] = {3, 1, 0, 0, 0, 0, 0, 0};
//   math_word_t ntt_result[8];

//   print_array("Poly A", poly_a, n);
//   print_array("Poly B", poly_b, n);

//   // Transform both polynomials
//   math_hal_ntt_forward(poly_a, &params);
//   math_hal_ntt_forward(poly_b, &params);

//   // Point-wise multiplication in NTT domain
//   math_hal_ntt_mult(ntt_result, poly_a, poly_b, &params);

//   // Inverse transform to get result
//   math_hal_ntt_inverse(ntt_result, &params);

//   print_array("NTT Result", ntt_result, n);

//   // Expected result: (1 + 2x)(3 + x) = 3 + 7x + 2x^2
//   math_word_t expected_result[8] = {3, 7, 2, 0, 0, 0, 0, 0};
//   print_array("Expected", expected_result, n);

//   // Verification
//   bool success = true;
//   for (uint32_t i = 0; i < n; i++)
//   {
//     if (ntt_result[i] != expected_result[i])
//     {
//       success = false;
//       break;
//     }
//   }

//   if (success)
//   {
//     LOG_INF("SUCCESS: NTT multiplication matches expected result.");
//   }
//   else
//   {
//     LOG_ERR("FAILURE: NTT multiplication does not match expected result.");
//   }
//   LOG_INF("--- End Test NTT Multiplication ---\n");
// }

int main(void)
{
  LOG_INF("what about inside main????????");

  ntt_params_t params;
  uint32_t n = 8;
  math_word_t modulus = 17; // Small prime for testing: 17-1 = 16, which is divisible by 8

  // LOG_INF("--- Testing NTT Init (n=%u, mod=%u) ---", n, (uint32_t)modulus);
  LOG_INF("please tell me anything updated inside test_ntt_init!!!!!!!!");
  int ret = math_hal_ntt_init_params(&params, n, modulus);

  // LOG_INF("--- Starting NTT Tests ---");

  // 1. Test NTT parameter initialization
  // test_ntt_init();

  // 2. Test forward and inverse transform
  // test_ntt_transform();

  // 3. Test polynomial multiplication
  // test_ntt_multiplication();

  // LOG_INF("--- All NTT Tests Completed ---");

  return 0;
}