#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdint.h>
#include <stdbool.h>

#include "openfhe_embedded.h"
#include "internal/ckks_params.h"

static void fill_minimal_params_2048(oe_ckks_params_t *p)
{
  memset(p, 0, sizeof(*p));
  p->ring_dim = 2048;
  p->num_q = 0;
  p->scale = 0.0;
  p->ntt_mode = OE_NTT_ON_THE_FLY;
  p->ifft_mode = OE_IFFT_ON_THE_FLY;
  p->mempool_hint_bytes = 0;
}

static const uint32_t kPreset_q[3] = {
    998244353u,
    1004535809u,
    469762049u};

/* Test ckks_params_get_preset_2048() */
ZTEST(ckks_params_suite, test_get_preset_2048_valid)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);

  oe_status_t status = ckks_params_get_preset_2048(&params);
  zassert_equal(status, OE_OK, "Preset function should succeed");

  zassert_true(params.num_q > 0 && params.num_q <= OE_MAX_LIMBS, "num_q invalid");
  zassert_true(params.scale > 0.0, "scale should be positive");

  for (uint32_t i = 0; i < params.num_q; i++)
  {
    zassert_true(ckks_qi_is_ntt_friendly(params.q[i], params.ring_dim),
                 "Prime q[%u] not NTT-friendly: %u", i, params.q[i]);
  }

  if (params.num_q == 3)
  {
    for (uint32_t i = 0; i < 3; i++)
    {
      zassert_equal(params.q[i], kPreset_q[i], "Preset prime mismatch at index %u", i);
    }
  }
}

ZTEST(ckks_params_suite, test_get_preset_2048_invalid_ring_dim)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  params.ring_dim = 4096; /* invalid for this preset */

  oe_status_t status = ckks_params_get_preset_2048(&params);
  zassert_equal(status, OE_ERR_INVALID_ARG, "Should reject wrong ring_dim");
}

ZTEST(ckks_params_suite, test_get_preset_2048_non_power_of_two_ring_dim)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  params.ring_dim = 3000; /* not power of two */

  oe_status_t status = ckks_params_get_preset_2048(&params);
  zassert_equal(status, OE_ERR_INVALID_ARG, "Should reject non-power-of-two ring_dim");
}

/* Test ckks_params_validate() */
ZTEST(ckks_params_suite, test_validate_valid_preset)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  oe_status_t status = ckks_params_validate(&params);
  zassert_equal(status, OE_OK, "Validation should pass on valid preset");
}

ZTEST(ckks_params_suite, test_validate_invalid_ring_dim)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  params.ring_dim = 1234; /* not power of two */
  oe_status_t status = ckks_params_validate(&params);
  zassert_equal(status, OE_ERR_INVALID_ARG, "Validation should fail for invalid ring_dim");
}

ZTEST(ckks_params_suite, test_validate_num_q_zero_and_overflow)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  params.num_q = 0;
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "num_q=0 should fail");

  params.num_q = OE_MAX_LIMBS + 1;
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "num_q overflow should fail");
}

ZTEST(ckks_params_suite, test_validate_invalid_q_values)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  uint32_t saved = params.q[0];

  /* qi == 0 */
  params.q[0] = 0;
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "qi=0 should fail");

  /* qi >= 2^OE_Q_MAX_BITS */
  params.q[0] = (1u << OE_Q_MAX_BITS);
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "qi too large should fail");

  /* qi not congruent to 1 mod 2N */
  params.q[0] = saved + 2;
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "qi not 1 mod 2N should fail");

  params.q[0] = saved;
}

ZTEST(ckks_params_suite, test_validate_nonpositive_scale)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  params.scale = 0.0;
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "scale=0 should fail");

  params.scale = -1.0;
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "scale<0 should fail");
}

ZTEST(ckks_params_suite, test_validate_invalid_modes)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  params.ntt_mode = (oe_ntt_mode_t)999;
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "Invalid ntt_mode should fail");
  params.ntt_mode = OE_NTT_ON_THE_FLY;

  params.ifft_mode = (oe_ifft_mode_t)888;
  zassert_equal(ckks_params_validate(&params), OE_ERR_INVALID_ARG, "Invalid ifft_mode should fail");
}

/* Test ckks_qi_is_ntt_friendly() */
ZTEST(ckks_params_suite, test_qi_is_ntt_friendly_true)
{
  for (size_t i = 0; i < 3; i++)
  {
    zassert_true(ckks_qi_is_ntt_friendly(kPreset_q[i], 2048), "Preset prime should be NTT friendly");
  }
}

ZTEST(ckks_params_suite, test_qi_is_ntt_friendly_false)
{
  /* Not 1 mod 2N */
  zassert_false(ckks_qi_is_ntt_friendly(998244354u, 2048), "Prime not 1 mod 2N should be false");

  /* N not power of two */
  zassert_false(ckks_qi_is_ntt_friendly(998244353u, 3000), "N not power of two should be false");

  /* qi too large */
  uint32_t large = (1u << OE_Q_MAX_BITS);
  zassert_false(ckks_qi_is_ntt_friendly(large, 2048), "qi too large should be false");
}

/* Test ckks_params_derive() */
ZTEST(ckks_params_suite, test_derive_valid)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);
  zassert_equal(ckks_params_validate(&params), OE_OK, "Params should validate");

  oe_derived_consts_t derived;
  memset(&derived, 0, sizeof(derived));

  oe_status_t status = ckks_params_derive(&params, &derived);
  zassert_equal(status, OE_OK, "Derive should succeed");

  zassert_equal(derived.ring_dim, params.ring_dim, "Derived ring_dim mismatch");
  zassert_equal(derived.twoN, params.ring_dim * 2, "Derived twoN mismatch");
  zassert_equal(derived.num_q, params.num_q, "Derived num_q mismatch");

  for (uint32_t i = 0; i < derived.num_q; i++)
  {
    uint32_t q = derived.primes[i].q;
    uint32_t ninv = derived.primes[i].n_inv_mod_q;
    zassert_equal(q, params.q[i], "Prime q mismatch at index %u", i);
    zassert_not_equal(ninv, 0u, "n_inv_mod_q should be nonzero");

    uint32_t lhs = (uint32_t)(((uint64_t)(params.ring_dim % q) * (uint64_t)ninv) % (uint64_t)q);
    zassert_equal(lhs, 1u, "n_inv_mod_q is not inverse mod q");

    uint64_t mu = derived.primes[i].barrett_mu;
    zassert_true(mu >= (UINT64_C(1) << 32), "barrett_mu too small");
    zassert_true(mu <= UINT64_MAX / q, "barrett_mu * q overflow");
  }
}

ZTEST(ckks_params_suite, test_derive_invalid_q)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  uint32_t saved = params.q[0];
  params.q[0] = saved + 2; /* break congruence */

  oe_derived_consts_t derived;
  memset(&derived, 0, sizeof(derived));

  oe_status_t status = ckks_params_derive(&params, &derived);
  zassert_equal(status, OE_ERR_INVALID_ARG, "Derive should fail for invalid q");

  params.q[0] = saved;
}

/* Test ckks_params_estimate_ntt_plan_bytes() */
ZTEST(ckks_params_suite, test_estimate_ntt_plan_bytes_precomputed)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  size_t expected = params.num_q * (2 * params.ring_dim * 4 + 32);
  size_t est = ckks_params_estimate_ntt_plan_bytes(params.ring_dim, params.num_q, params.q, OE_NTT_PRECOMPUTED);
  zassert_equal(est, expected, "Unexpected NTT plan bytes estimate");
}

ZTEST(ckks_params_suite, test_estimate_ntt_plan_bytes_on_the_fly_and_invalid)
{
  oe_ckks_params_t params;
  fill_minimal_params_2048(&params);
  ckks_params_get_preset_2048(&params);

  size_t est = ckks_params_estimate_ntt_plan_bytes(params.ring_dim, params.num_q, params.q, OE_NTT_ON_THE_FLY);
  zassert_equal(est, 0, "Estimate should be zero for ON_THE_FLY mode");

  est = ckks_params_estimate_ntt_plan_bytes(0, params.num_q, params.q, OE_NTT_PRECOMPUTED);
  zassert_equal(est, 0, "Estimate zero for ring_dim=0");

  est = ckks_params_estimate_ntt_plan_bytes(params.ring_dim, 0, params.q, OE_NTT_PRECOMPUTED);
  zassert_equal(est, 0, "Estimate zero for num_q=0");

  est = ckks_params_estimate_ntt_plan_bytes(params.ring_dim, params.num_q, NULL, OE_NTT_PRECOMPUTED);
  zassert_equal(est, 0, "Estimate zero for NULL q array");
}

/* Test ckks_params_estimate_ifft_plan_bytes() */
ZTEST(ckks_params_suite, test_estimate_ifft_plan_bytes_precomputed)
{
  size_t est = ckks_params_estimate_ifft_plan_bytes(2048, OE_IFFT_PRECOMPUTED);
  size_t expected = 2048 * 18 + 32;
  zassert_equal(est, expected, "Unexpected IFFT plan bytes estimate");
}

ZTEST(ckks_params_suite, test_estimate_ifft_plan_bytes_on_the_fly_and_invalid)
{
  size_t est = ckks_params_estimate_ifft_plan_bytes(2048, OE_IFFT_ON_THE_FLY);
  zassert_equal(est, 0, "Estimate zero for ON_THE_FLY mode");

  est = ckks_params_estimate_ifft_plan_bytes(0, OE_IFFT_PRECOMPUTED);
  zassert_equal(est, 0, "Estimate zero for ring_dim=0");

  est = ckks_params_estimate_ifft_plan_bytes(3000, OE_IFFT_PRECOMPUTED);
  zassert_equal(est, 0, "Estimate zero for non-power-of-two ring_dim");
}

/* Register test suite */
ZTEST_SUITE(ckks_params_suite, NULL, NULL, NULL, NULL, NULL);
