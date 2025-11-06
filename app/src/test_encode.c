#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "openfhe_embedded.h"
#include "internal/encode.h"
#include "internal/ckks_params.h" /* only for OE_MAX_LIMBS / consistency; not strictly required */

/* ---------- Small helpers ---------- */

static uint32_t log2_u32(uint32_t n)
{
  uint32_t bits = 0;
  while (n > 1)
  {
    n >>= 1u;
    bits++;
  }
  return bits;
}

/* Bit-reversal (same definition as in encode.c’s build_bitrev) */
static uint16_t bitrev_of(uint16_t x, uint32_t bits)
{
  uint16_t r = 0;
  for (uint32_t b = 0; b < bits; ++b)
  {
    r = (uint16_t)((r << 1) | (x & 1u));
    x >>= 1u;
  }
  return r;
}

/* ---------- Requirements & exact size ---------- */

ZTEST(encode_suite, test_requirements_on_the_fly)
{
  size_t hint = 1234;
  zassert_equal(encode_plan_requirements(2048, OE_IFFT_ON_THE_FLY, &hint), OE_OK, "ON_THE_FLY req failed");
  zassert_equal(hint, 0, "ON_THE_FLY hint must be 0");

  size_t exact = encode_plan_exact_bytes(2048, OE_IFFT_ON_THE_FLY);
  zassert_equal(exact, 0, "ON_THE_FLY exact must be 0");
}

ZTEST(encode_suite, test_requirements_precomputed_valid)
{
  const uint32_t N = 2048;
  size_t hint = 0;
  zassert_equal(encode_plan_requirements(N, OE_IFFT_PRECOMPUTED, &hint), OE_OK, "PRE req failed");
  zassert_true(hint > 0, "hint must be > 0");

  size_t exact = encode_plan_exact_bytes(N, OE_IFFT_PRECOMPUTED);
  zassert_true(exact > 0, "exact must be > 0");
  zassert_equal(exact, hint, "exact != hint");
}

ZTEST(encode_suite, test_requirements_precomputed_invalid_args)
{
  size_t hint = 0;

  /* null out_size_hint */
  zassert_equal(encode_plan_requirements(2048, OE_IFFT_PRECOMPUTED, NULL),
                OE_ERR_INVALID_ARG, "should reject NULL out_size_hint");

  /* non power-of-two N */
  zassert_equal(encode_plan_requirements(3000, OE_IFFT_PRECOMPUTED, &hint),
                OE_ERR_INVALID_ARG, "should reject non-power-of-two N");

  /* ring_dim==0 */
  zassert_equal(encode_plan_requirements(0, OE_IFFT_PRECOMPUTED, &hint),
                OE_ERR_INVALID_ARG, "should reject N=0");
}

ZTEST(encode_suite, test_exact_bytes_invalid_mode_and_inputs)
{
  zassert_equal(encode_plan_exact_bytes(2048, (oe_ifft_mode_t)999), 0, "invalid mode must return 0");
  zassert_equal(encode_plan_exact_bytes(0, OE_IFFT_PRECOMPUTED), 0, "N=0 must return 0");
  zassert_equal(encode_plan_exact_bytes(3000, OE_IFFT_PRECOMPUTED), 0, "non power-of-two must return 0");
}

/* ---------- Plan init / free and accessors ---------- */

ZTEST(encode_suite, test_plan_init_on_the_fly_and_accessors)
{
  oe_encode_plan_t *plan = NULL;
  zassert_equal(encode_plan_init(0 /* unused in OTF */, OE_IFFT_ON_THE_FLY,
                                 NULL, 0, &plan),
                OE_OK, "init OTF failed");
  zassert_not_null(plan, "plan is NULL");
  zassert_equal(encode_plan_mode(plan), OE_IFFT_ON_THE_FLY, "mode mismatch");

  /* Accessors: OTF => no twiddles/perm present */
  size_t len = 999;
  zassert_is_null(encode_plan_get_twiddles(plan, &len), "twiddles must be NULL in OTF");
  zassert_equal(len, 0, "twiddles len must be 0 in OTF");
  zassert_is_null(encode_plan_get_bitrev(plan, &len), "bitrev must be NULL in OTF");
  zassert_equal(len, 0, "bitrev len must be 0 in OTF");

  /* ring_dim accessor returns what was set in the plan; for OTF we put 0 */
  zassert_equal(encode_plan_ring_dim(plan), 0u, "ring_dim accessor mismatch in OTF");

  encode_plan_free(plan);
}

ZTEST(encode_suite, test_plan_init_precomputed_and_accessors)
{
  const uint32_t N = 2048;
  zassert_true(is_power_of_two(N), "test requires power of two N");

  size_t bytes = encode_plan_exact_bytes(N, OE_IFFT_PRECOMPUTED);
  zassert_true(bytes > 0, "exact bytes must be > 0");

  void *pool = malloc(bytes);
  zassert_not_null(pool, "malloc pool failed");

  oe_encode_plan_t *plan = NULL;
  zassert_equal(encode_plan_init(N, OE_IFFT_PRECOMPUTED, pool, bytes, &plan),
                OE_OK, "init PRE failed");
  zassert_not_null(plan, "plan is NULL");
  zassert_equal(encode_plan_mode(plan), OE_IFFT_PRECOMPUTED, "mode mismatch");
  zassert_equal(encode_plan_ring_dim(plan), N, "ring_dim accessor mismatch");

  /* Twiddles: expect 2*N doubles; Bitrev: N uint16_t */
  size_t tw_len = 0, br_len = 0;
  const double *tw = encode_plan_get_twiddles(plan, &tw_len);
  const uint16_t *br = encode_plan_get_bitrev(plan, &br_len);

  zassert_not_null(tw, "twiddles is NULL");
  zassert_equal(tw_len, (size_t)2 * (size_t)N, "twiddles length mismatch");
  zassert_not_null(br, "bitrev is NULL");
  zassert_equal(br_len, (size_t)N, "bitrev length mismatch");

  /* Twiddle spot checks: k=0 -> (1,0); k=N/4 -> (0,-1); k=N/2 -> (-1,0) */
  const double eps = 1e-12;
  /* k=0 */
  zassert_true(fabs(tw[0] - 1.0) <= eps, "tw[0].Re != 1");
  zassert_true(fabs(tw[1] - 0.0) <= eps, "tw[0].Im != 0");
  /* k=N/4 -> angle = -2π * (N/4)/N = -π/2 */
  zassert_true(fabs(tw[2 * (N / 4) + 0] - 0.0) <= 1e-9, "tw[N/4].Re != 0");
  zassert_true(fabs(tw[2 * (N / 4) + 1] + 1.0) <= 1e-9, "tw[N/4].Im != -1");
  /* k=N/2 -> angle = -π */
  zassert_true(fabs(tw[2 * (N / 2) + 0] + 1.0) <= 1e-12, "tw[N/2].Re != -1");
  zassert_true(fabs(tw[2 * (N / 2) + 1] - 0.0) <= 1e-12, "tw[N/2].Im != 0");

  /* Bit-reversal sanity: br[br[i]] == i */
  const uint32_t bits = log2_u32(N);
  for (uint32_t i = 0; i < N; ++i)
  {
    uint16_t r = br[i];
    zassert_equal(br[r], (uint16_t)i, "bitrev involution failure at i=%u", i);
    /* also matches local bitrev function */
    zassert_equal(r, bitrev_of((uint16_t)i, bits), "bitrev value mismatch at i=%u", i);
  }

  encode_plan_free(plan);
  free(pool);
}

ZTEST(encode_suite, test_plan_init_precomputed_insufficient_pool)
{
  const uint32_t N = 2048;
  zassert_true(is_power_of_two(N), "test requires power of two N");

  /* Compute a hard lower bound that ignores alignment and metadata cushion:
     minimal = 2*N*sizeof(double) + N*sizeof(uint16_t)
     Passing (minimal - 1) must fail regardless of base alignment. */
  size_t minimal = (size_t)2 * (size_t)N * sizeof(double) +
                   (size_t)N * sizeof(uint16_t);
  zassert_true(minimal > 0, "minimal must be > 0");

  void *pool = malloc(minimal - 1);
  zassert_not_null(pool, "malloc pool failed");

  oe_encode_plan_t *plan = NULL;
  zassert_equal(encode_plan_init(N, OE_IFFT_PRECOMPUTED, pool, minimal - 1, &plan),
                OE_ERR_INVALID_ARG, "init should fail with insufficient pool");
  zassert_is_null(plan, "plan should remain NULL");

  free(pool);
}

/* ---------- Register test suite ---------- */
ZTEST_SUITE(encode_suite, NULL, NULL, NULL, NULL, NULL);
