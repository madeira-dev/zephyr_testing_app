#include <zephyr/logging/log.h>
#include "scheme/ckks/ckks_cryptoparams.h"
#include "scheme/ckks/ckks_keygen.h"
#include "scheme/ckks/ckks_encoder.h"
#include "scheme/ckks/ckks_encryptor.h"
#include "scheme/ckks/ckks.h"
#include "core/math/polynomial.h"
#include "core/math/biginteger.h"

LOG_MODULE_REGISTER(test_ckks_encrypt, LOG_LEVEL_INF);

extern int ckks_encryptor_dummy_global;

void print_poly(const char *label, const polynomial_t *poly)
{
    LOG_INF("%s degree: %u", label, poly->degree);
    for (uint32_t i = 0; i <= poly->degree; i++)
    {
        char buf[32];
        bigint_to_string(&poly->coeffs[i], buf, sizeof(buf));
        LOG_INF("%s Coeff[%u] = %s", label, i, buf);
    }
}

void main(void)
{
    LOG_INF("ckks_encryptor_dummy_global = %d", ckks_encryptor_dummy_global);
    static bigint_t modulus_chain[1];
    static ckks_cryptoparams_t params;
    static ckks_encoder_t encoder;
    static ckks_publickey_t pk;
    static ckks_secretkey_t sk;
    static ckks_plaintext_t plaintext;
    static ckks_ciphertext_t ciphertext;
    double values[4] = {1.5, -2.0, 3.25, 0.0};

    // 1. Setup modulus chain (small prime for test)
    bigint_init_u64(&modulus_chain[0], 65537);

    // 2. Setup CKKS crypto params
    ckks_cryptoparams_init(&params, 8, modulus_chain, 1, 120000, 1);

    // 3. Key generation
    if (ckks_keygen(&params, &pk, &sk) != 0)
    {
        LOG_ERR("Key generation failed");
        return;
    }
    print_poly("PublicKey[0]", &pk.pk[0]);
    print_poly("PublicKey[1]", &pk.pk[1]);

    // 4. Encoder
    ckks_encoder_init(&encoder, &params);

    // 5. Encode plaintext
    poly_init(&plaintext.poly, params.ring_dimension - 1);
    ckks_encode(&encoder, values, 4, &plaintext.poly);
    print_poly("Plaintext", &plaintext.poly);

    // 6. Encrypt
    LOG_INF("Starting encryption");
    if (ckks_encrypt(&params, &pk, &plaintext, &ciphertext) != 0)
    {
        LOG_ERR("Encryption failed");
        return;
    }
    print_poly("Ciphertext[0]", &ciphertext.parts[0]);
    print_poly("Ciphertext[1]", &ciphertext.parts[1]);

    LOG_INF("CKKS encryption test completed.");
}