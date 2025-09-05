// #include <stdio.h>
// #include <zephyr/logging/log.h>
// #include "scheme/ckks/ckks_encoder.h"
// #include "scheme/ckks/ckks_cryptoparams.h"
// #include "core/math/polynomial.h"
// #include "core/math/biginteger.h"

// LOG_MODULE_REGISTER(test_ckks_encoder, LOG_LEVEL_DBG);

// void print_poly(const polynomial_t *poly)
// {
//     LOG_INF("Polynomial degree: %u", poly->degree);
//     for (uint32_t i = 0; i <= poly->degree; i++)
//     {
//         char buf[32];
//         bigint_to_string(&poly->coeffs[i], buf, sizeof(buf));
//         LOG_INF("Coeff[%u] = %s", i, buf);
//     }
// }

// int main(void)
// {
//     // 1. Setup modulus chain
//     static bigint_t modulus_chain[1];
//     bigint_init_u64(&modulus_chain[0], 65537); // Example small prime modulus

//     // 2. Setup CKKS crypto params
//     static ckks_cryptoparams_t params;
//     ckks_cryptoparams_init(&params, 8, modulus_chain, 1, 1 << 20, 1);

//     // 3. Setup encoder
//     static ckks_encoder_t encoder;
//     ckks_encoder_init(&encoder, &params);

//     // 4. Input values
//     double values[4] = {1.5, -2.0, 3.25, 0.0};

//     // 5. Output polynomial
//     static polynomial_t poly;
//     ckks_encode(&encoder, values, 4, &poly);

//     // 6. Print result
//     print_poly(&poly);

//     return 0;
// }