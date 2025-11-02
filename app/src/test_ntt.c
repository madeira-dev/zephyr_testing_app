#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "openfhe_embedded.h"
#include "internal/ckks_params.h"
#include "internal/ntt.h"

static void fill_minimal_params_2048(oe_ckks_params_t *p)
{
  memset(p, 0, sizeof(*p));
  p->ring_dim = 2048;
  p->num_q = 0;   /* let preset fill q’s */
  p->scale = 0.0; /* let preset set default */
  p->ntt_mode = OE_NTT_ON_THE_FLY;
  p->ifft_mode = OE_IFFT_ON_THE_FLY;
  p->mempool_hint_bytes = 0;
}

static void prepare_valid_params_and_derived(oe_ckks_params_t *params,
                                             oe_derived_consts_t *derived)
{
  fill_minimal_params_2048(params);
  zassert_equal(ckks_params_get_preset_2048(params), OE_OK, "preset failed");
  zassert_equal(ckks_params_validate(params), OE_OK, "validate failed");

  memset(derived, 0, sizeof(*derived));
  zassert_equal(ckks_params_derive(params, derived), OE_OK, "derive failed");
  zassert_equal(derived->ring_dim, params->ring_dim, "ring_dim mismatch");
  zassert_equal(derived->num_q, params->num_q, "num_q mismatch");
}

static void random_poly(uint32_t *a, uint32_t N, uint32_t q, uint32_t seed)
{
  /* Very small LCG; deterministic for test reproducibility */
  uint32_t x = seed ? seed : 1;
  for (uint32_t i = 0; i < N; ++i)
  {
    x = 1664525u * x + 1013904223u;
    a[i] = (q == 0) ? 0 : (x % q);
  }
}

/* ---------- Requirements & exact size tests ---------- */

ZTEST(ntt_suite, test_requirements_on_the_fly)
{
  size_t hint = 1234;
  oe_status_t st = ntt_plan_requirements(2048, 1, (const uint32_t[]){998244353u}, OE_NTT_ON_THE_FLY, &hint);
  zassert_equal(st, OE_OK, "requirements ON_THE_FLY should succeed");
  zassert_equal(hint, 0, "ON_THE_FLY hint must be 0");
}

ZTEST(ntt_suite, test_requirements_precomputed_valid)
{
  oe_ckks_params_t params;
  oe_derived_consts_t dc;
  prepare_valid_params_and_derived(&params, &dc);

  size_t hint = 0;
  oe_status_t st = ntt_plan_requirements(params.ring_dim, params.num_q, params.q, OE_NTT_PRECOMPUTED, &hint);
  zassert_equal(st, OE_OK, "requirements should succeed");
  zassert_true(hint > 0, "hint should be > 0 in PRECOMPUTED");

  size_t exact = ntt_plan_exact_bytes(params.ring_dim, params.num_q, OE_NTT_PRECOMPUTED);
  zassert_true(exact > 0, "exact should be > 0");
  /* requirements() returns exact + small align cushion (up to +3).
     Our implementation returns exact itself (+3 is already included in exact).
     So they should match. */
  zassert_equal(exact, hint, "exact and requirements hint mismatch");
}

ZTEST(ntt_suite, test_requirements_precomputed_invalid_args)
{
  size_t hint = 0;
  /* Null out_size_hint */
  zassert_equal(ntt_plan_requirements(2048, 1, (const uint32_t[]){998244353u}, OE_NTT_PRECOMPUTED, NULL),
                OE_ERR_INVALID_ARG, "should reject null out_size_hint");
  /* Non power-of-two N */
  zassert_equal(ntt_plan_requirements(3000, 1, (const uint32_t[]){998244353u}, OE_NTT_PRECOMPUTED, &hint),
                OE_ERR_INVALID_ARG, "should reject non-power-of-two N");
  /* num_q zero */
  zassert_equal(ntt_plan_requirements(2048, 0, (const uint32_t[]){998244353u}, OE_NTT_PRECOMPUTED, &hint),
                OE_ERR_INVALID_ARG, "should reject num_q=0");
  /* null q */
  zassert_equal(ntt_plan_requirements(2048, 1, NULL, OE_NTT_PRECOMPUTED, &hint),
                OE_ERR_INVALID_ARG, "should reject null q");
}

ZTEST(ntt_suite, test_exact_bytes_matches_requirements)
{
  oe_ckks_params_t params;
  oe_derived_consts_t dc;
  prepare_valid_params_and_derived(&params, &dc);

  size_t hint = 0;
  zassert_equal(ntt_plan_requirements(params.ring_dim, params.num_q, params.q, OE_NTT_PRECOMPUTED, &hint),
                OE_OK, "req failed");
  size_t exact = ntt_plan_exact_bytes(params.ring_dim, params.num_q, OE_NTT_PRECOMPUTED);
  zassert_equal(exact, hint, "exact != hint");
}

/* ---------- Plan init / free and accessors ---------- */

ZTEST(ntt_suite, test_plan_init_on_the_fly_and_accessors)
{
  oe_ckks_params_t params;
  oe_derived_consts_t dc;
  prepare_valid_params_and_derived(&params, &dc);

  /* ON_THE_FLY needs no mempool */
  oe_ntt_plan_t *plan = NULL;
  zassert_equal(ntt_plan_init(params.ring_dim,
                              params.num_q,
                              params.q,
                              &dc,
                              OE_NTT_ON_THE_FLY,
                              NULL,
                              0,
                              &plan),
                OE_OK, "plan init ON_THE_FLY failed");
  zassert_not_null(plan, "plan is null");

  zassert_equal(ntt_plan_mode(plan), OE_NTT_ON_THE_FLY, "mode mismatch");
  zassert_equal(ntt_plan_ring_dim(plan), params.ring_dim, "ring_dim accessor mismatch");

  /* No precomputed tables should be present */
  size_t len = 999;
  zassert_is_null(ntt_plan_get_fwd_roots(plan, 0, &len), "fwd roots should be NULL in ON_THE_FLY");
  zassert_equal(len, 0, "len should be 0");
  zassert_is_null(ntt_plan_get_inv_roots(plan, 0, &len), "inv roots should be NULL in ON_THE_FLY");
  zassert_equal(len, 0, "len should be 0");

  ntt_plan_free(plan);
}

ZTEST(ntt_suite, test_plan_init_precomputed_and_accessors)
{
  oe_ckks_params_t params;
  oe_derived_consts_t dc;
  prepare_valid_params_and_derived(&params, &dc);

  size_t bytes = ntt_plan_exact_bytes(params.ring_dim, params.num_q, OE_NTT_PRECOMPUTED);
  zassert_true(bytes > 0, "exact bytes must be > 0");

  void *pool = malloc(bytes);
  zassert_not_null(pool, "malloc failed for mempool");

  oe_ntt_plan_t *plan = NULL;
  zassert_equal(ntt_plan_init(params.ring_dim,
                              params.num_q,
                              params.q,
                              &dc,
                              OE_NTT_PRECOMPUTED,
                              pool,
                              bytes,
                              &plan),
                OE_OK, "plan init PRECOMPUTED failed");
  zassert_not_null(plan, "plan is null");
  zassert_equal(ntt_plan_mode(plan), OE_NTT_PRECOMPUTED, "mode mismatch");
  zassert_equal(ntt_plan_ring_dim(plan), params.ring_dim, "ring_dim accessor mismatch");

  /* Accessors must return non-NULL tables of length N for each prime */
  for (uint32_t i = 0; i < params.num_q; ++i)
  {
    size_t len_f = 0, len_i = 0;
    const uint32_t *f = ntt_plan_get_fwd_roots(plan, i, &len_f);
    const uint32_t *iv = ntt_plan_get_inv_roots(plan, i, &len_i);
    zassert_not_null(f, "fwd table is NULL at i=%u", i);
    zassert_not_null(iv, "inv table is NULL at i=%u", i);
    zassert_equal(len_f, params.ring_dim, "fwd len mismatch");
    zassert_equal(len_i, params.ring_dim, "inv len mismatch");
  }

  /* Invalid index should return NULL */
  size_t dummy = 777;
  zassert_is_null(ntt_plan_get_fwd_roots(plan, params.num_q, &dummy), "should be NULL for out-of-range");
  zassert_equal(dummy, 0, "len should be 0 for out-of-range");
  zassert_is_null(ntt_plan_get_inv_roots(plan, params.num_q, &dummy), "should be NULL for out-of-range");
  zassert_equal(dummy, 0, "len should be 0 for out-of-range");

  ntt_plan_free(plan);
  free(pool);
}

ZTEST(ntt_suite, test_plan_init_precomputed_insufficient_pool)
{
  oe_ckks_params_t params;
  oe_derived_consts_t dc;
  prepare_valid_params_and_derived(&params, &dc);

  size_t bytes = ntt_plan_exact_bytes(params.ring_dim, params.num_q, OE_NTT_PRECOMPUTED);
  zassert_true(bytes > 0, "exact bytes must be > 0");

  /* Provide too small pool */
  void *pool_small = malloc(bytes - 4);
  zassert_not_null(pool_small, "malloc failed");

  oe_ntt_plan_t *plan = NULL;
  zassert_equal(ntt_plan_init(params.ring_dim,
                              params.num_q,
                              params.q,
                              &dc,
                              OE_NTT_PRECOMPUTED,
                              pool_small,
                              bytes - 4,
                              &plan),
                OE_ERR_INVALID_ARG, "init should fail with insufficient pool");
  zassert_is_null(plan, "plan should remain NULL");

  free(pool_small);
}

/* ---------- Functional tests: forward + inverse roundtrip ---------- */

static void roundtrip_once(uint32_t N,
                           uint32_t q,
                           uint32_t n_inv_mod_q,
                           oe_ntt_mode_t mode)
{
  /* Allocate vector and backup */
  uint32_t *vec = (uint32_t *)malloc(N * sizeof(uint32_t));
  uint32_t *bak = (uint32_t *)malloc(N * sizeof(uint32_t));
  zassert_not_null(vec, "malloc vec failed");
  zassert_not_null(bak, "malloc bak failed");

  random_poly(vec, N, q, 123456789u);
  memcpy(bak, vec, N * sizeof(uint32_t));

  oe_ckks_params_t params;
  oe_derived_consts_t dc;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);
  ckks_params_derive(&params, &dc);

  /* Build plan for the given mode (if PRECOMPUTED, allocate pool) */
  oe_ntt_plan_t *plan = NULL;
  void *pool = NULL;
  size_t bytes = 0;

  if (mode == OE_NTT_PRECOMPUTED)
  {
    bytes = ntt_plan_exact_bytes(N, dc.num_q, OE_NTT_PRECOMPUTED);
    zassert_true(bytes > 0, "exact bytes must be > 0");
    pool = malloc(bytes);
    zassert_not_null(pool, "malloc pool failed");
  }

  zassert_equal(ntt_plan_init(N, dc.num_q, params.q, &dc, mode, pool, bytes, &plan), OE_OK, "plan init failed");
  zassert_not_null(plan, "plan is NULL");

  /* Perform forward and inverse for prime_index 0 (and we’ll loop per prime in a higher-level test) */
  uint32_t prime_index = 0;
  ntt_forward_inplace(vec, q, plan, prime_index);
  ntt_inverse_inplace(vec, q, n_inv_mod_q, plan, prime_index);

  /* Validate exact recovery */
  for (uint32_t i = 0; i < N; ++i)
  {
    /* Reduce both sides to [0,q) before compare */
    uint32_t lhs = vec[i] % q;
    uint32_t rhs = bak[i] % q;
    zassert_equal(lhs, rhs, "roundtrip mismatch at i=%u (mode=%d)", i, (int)mode);
  }

  ntt_plan_free(plan);
  if (pool)
    free(pool);
  free(vec);
  free(bak);
}

ZTEST(ntt_suite, test_roundtrip_all_primes_both_modes)
{
  oe_ckks_params_t params;
  oe_derived_consts_t dc;
  prepare_valid_params_and_derived(&params, &dc);

  /* For each prime, run roundtrip in both modes */
  for (uint32_t i = 0; i < dc.num_q; ++i)
  {
    uint32_t q = dc.primes[i].q;
    uint32_t ninv = dc.primes[i].n_inv_mod_q;

    /* ON_THE_FLY */
    {
      /* allocate vec here per prime to ensure prime-specific moduli if desired */
      uint32_t *vec = (uint32_t *)malloc(params.ring_dim * sizeof(uint32_t));
      uint32_t *bak = (uint32_t *)malloc(params.ring_dim * sizeof(uint32_t));
      zassert_not_null(vec, "malloc vec failed");
      zassert_not_null(bak, "malloc bak failed");

      random_poly(vec, params.ring_dim, q, 0xA5A5A5u ^ i);
      memcpy(bak, vec, params.ring_dim * sizeof(uint32_t));

      oe_ntt_plan_t *plan = NULL;
      zassert_equal(ntt_plan_init(params.ring_dim, params.num_q, params.q, &dc,
                                  OE_NTT_ON_THE_FLY, NULL, 0, &plan),
                    OE_OK, "init OTF failed");
      zassert_not_null(plan, "plan NULL");

      ntt_forward_inplace(vec, q, plan, i);
      ntt_inverse_inplace(vec, q, ninv, plan, i);

      for (uint32_t k = 0; k < params.ring_dim; ++k)
      {
        zassert_equal(vec[k] % q, bak[k] % q, "roundtrip OTF mismatch (prime=%u k=%u)", i, k);
      }

      ntt_plan_free(plan);
      free(vec);
      free(bak);
    }

    /* PRECOMPUTED */
    {
      size_t bytes = ntt_plan_exact_bytes(params.ring_dim, params.num_q, OE_NTT_PRECOMPUTED);
      zassert_true(bytes > 0, "exact bytes must be > 0");
      void *pool = malloc(bytes);
      zassert_not_null(pool, "malloc pool failed");

      uint32_t *vec = (uint32_t *)malloc(params.ring_dim * sizeof(uint32_t));
      uint32_t *bak = (uint32_t *)malloc(params.ring_dim * sizeof(uint32_t));
      zassert_not_null(vec, "malloc vec failed");
      zassert_not_null(bak, "malloc bak failed");

      random_poly(vec, params.ring_dim, q, 0x5A5A5Au ^ i);
      memcpy(bak, vec, params.ring_dim * sizeof(uint32_t));

      oe_ntt_plan_t *plan = NULL;
      zassert_equal(ntt_plan_init(params.ring_dim, params.num_q, params.q, &dc,
                                  OE_NTT_PRECOMPUTED, pool, bytes, &plan),
                    OE_OK, "init PRE failed");
      zassert_not_null(plan, "plan NULL");

      ntt_forward_inplace(vec, q, plan, i);
      ntt_inverse_inplace(vec, q, ninv, plan, i);

      for (uint32_t k = 0; k < params.ring_dim; ++k)
      {
        zassert_equal(vec[k] % q, bak[k] % q, "roundtrip PRE mismatch (prime=%u k=%u)", i, k);
      }

      ntt_plan_free(plan);
      free(vec);
      free(bak);
      free(pool);
    }
  }
}

/* ---------- Register test suite ---------- */
ZTEST_SUITE(ntt_suite, NULL, NULL, NULL, NULL, NULL);
