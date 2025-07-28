#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "scheme/ckks/ckks_cryptoparams.h"
#include "core/math/biginteger.h"

LOG_MODULE_REGISTER(test_ckks_cryptoparams);

int main(void)
{
    ckks_cryptoparams_t params;
    bigint_t moduli[CKKS_MAX_MODULI];

    // Initialize modulus chain with small primes for testing
    for (uint32_t i = 0; i < CKKS_MAX_MODULI; i++)
    {
        bigint_init_u64(&moduli[i], 17 + i); // Example primes: 17, 18, 19, 20
    }

    uint32_t ring_dimension = 8;
    double scaling_factor = 1 << 20;
    uint32_t max_depth = 2;
    uint32_t num_moduli = 2;

    int ret = ckks_cryptoparams_init(&params, ring_dimension, moduli, num_moduli, scaling_factor, max_depth);
    if (ret != 0)
    {
        LOG_INF("ckks_cryptoparams_init failed!");
        return;
    }

    // Check values
    if (params.ring_dimension != ring_dimension ||
        params.num_moduli != num_moduli ||
        params.scaling_factor != scaling_factor ||
        params.max_depth != max_depth)
    {
        LOG_INF("Parameter mismatch!");
        return;
    }

    char buf[32];
    for (uint32_t i = 0; i < num_moduli; i++)
    {
        bigint_to_string(&params.modulus_chain[i], buf, sizeof(buf));
        LOG_INF("Modulus %u: %s (bits: %u)", i, buf, params.modulus_bits[i]);
    }

    LOG_INF("Poly params: ring_dimension=%u, modulus_bits=%u",
            params.poly_params.ring_dimension, params.poly_params.modulus_bits);

    // Cleanup
    ckks_cryptoparams_cleanup(&params);

    LOG_INF("Test passed!");

    return 0;
}