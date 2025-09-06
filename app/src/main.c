#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "openfhe_embedded.h"
#include "core/utils/serialization.h"
#include <string.h>

LOG_MODULE_REGISTER(test_fhe_lib, LOG_LEVEL_INF);

// Define test parameters
#define TEST_RING_DIMENSION 256
#define TEST_SCALING_FACTOR 4096.0

// A large buffer for the serialized JSON output.
// Adjust size if needed for larger ring dimensions.
#define SERIALIZATION_BUFFER_SIZE 8192
static char serialization_buffer[SERIALIZATION_BUFFER_SIZE];

/**
 * @brief Main test routine for the embedded FHE library.
 */
int main(void)
{
  LOG_INF("--- Starting OpenFHE-Embedded Library Test ---");

  static fhe_context_t context;
  memset(&context, 0, sizeof(fhe_context_t));

  // ========================================================================
  // 1. Test FHE Context Initialization
  // ========================================================================
  LOG_INF("1. Testing FHE Context Initialization...");
  LOG_INF("   Ring Dimension: %u, Scaling Factor: %.1f", TEST_RING_DIMENSION, TEST_SCALING_FACTOR);

  uint64_t start_cycles = math_hal_get_cycles();
  int ret = fhe_context_init(&context, TEST_RING_DIMENSION, TEST_SCALING_FACTOR);
  uint64_t end_cycles = math_hal_get_cycles();

  if (ret == 0 && context.is_initialized)
  {
    LOG_INF("   SUCCESS: Context initialized in %llu cycles.", (end_cycles - start_cycles));
    LOG_INF("   - CryptoParams: %u moduli generated.", context.params.num_moduli);
    LOG_INF("   - Public Key: Generated successfully.");
  }
  else
  {
    LOG_ERR("   FAILURE: Context initialization failed with code %d.", ret);
    return -1;
  }

  // ========================================================================
  // 2. Test CKKS Encryption
  // ========================================================================
  LOG_INF("\n2. Testing CKKS Encryption...");
  double sample_data[] = {0.1, 0.2, 0.3, 0.4};
  size_t sample_count = sizeof(sample_data) / sizeof(double);

  printk("   Plaintext to encrypt (%d values): [", sample_count);
  for (size_t i = 0; i < sample_count; i++)
  {
    printk("%.2f%s", sample_data[i], (i == sample_count - 1) ? "" : ", ");
  }
  printk("]\n");

  static ckks_ciphertext_t ciphertext;
  start_cycles = math_hal_get_cycles();
  ret = fhe_encrypt(&context, sample_data, sample_count, &ciphertext);
  end_cycles = math_hal_get_cycles();

  if (ret == 0)
  {
    LOG_INF("   SUCCESS: Encryption completed in %llu cycles.", (end_cycles - start_cycles));
    LOG_INF("   - Ciphertext scaling factor: %.1f (Expected: %.1f)", ciphertext.scaling_factor, TEST_SCALING_FACTOR);
    LOG_INF("   - Ciphertext level: %u (Expected: 0)", ciphertext.level);
  }
  else
  {
    LOG_ERR("   FAILURE: Encryption failed with code %d.", ret);
    fhe_context_cleanup(&context);
    return -1;
  }

  // ========================================================================
  // 3. Test Ciphertext Serialization (for Interoperability)
  // ========================================================================
  LOG_INF("\n3. Testing Ciphertext Serialization...");
  memset(serialization_buffer, 0, SERIALIZATION_BUFFER_SIZE);

  int bytes_written = fhe_ciphertext_serialize(&ciphertext, serialization_buffer, SERIALIZATION_BUFFER_SIZE);

  if (bytes_written > 0)
  {
    LOG_INF("   SUCCESS: Ciphertext serialized to %d bytes.", bytes_written);
    LOG_INF("   --- BEGIN SERIALIZED CIPHERTEXT (JSON) ---");
    // Print the JSON string in chunks to avoid logger limitations
    const size_t chunk_size = 128;
    for (size_t i = 0; i < bytes_written; i += chunk_size)
    {
      size_t remaining = bytes_written - i;
      size_t len = (remaining < chunk_size) ? remaining : chunk_size;
      printk("%.*s", len, &serialization_buffer[i]);
    }
    printk("\n"); // Print a final newline
    LOG_INF("   --- END SERIALIZED CIPHERTEXT (JSON) ---");
    LOG_INF("\n   VALIDATION STEP: Copy the JSON output above and use it in a full OpenFHE host application to verify decryption.");
  }
  else
  {
    LOG_ERR("   FAILURE: Serialization failed with code %d.", bytes_written);
    fhe_context_cleanup(&context);
    return -1;
  }

  // ========================================================================
  // 4. Test Context Cleanup
  // ========================================================================
  LOG_INF("\n4. Testing Context Cleanup...");
  fhe_context_cleanup(&context);
  if (!context.is_initialized)
  {
    LOG_INF("   SUCCESS: Context cleaned up successfully.");
  }
  else
  {
    LOG_ERR("   FAILURE: Context cleanup failed.");
  }

  LOG_INF("\n--- All tests complete ---");

  return 0;
}