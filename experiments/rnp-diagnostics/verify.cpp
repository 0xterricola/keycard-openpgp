#include <rnp/rnp.h>
#include <rnp/rnp_err.h>

#include <cstdio>
#include <cstdlib>

static void check(const char* operation, rnp_result_t result) {
    if (result != RNP_SUCCESS) {
        std::fprintf(stderr, "%s: %s (0x%08x)\n",
                     operation, rnp_result_to_string(result), result);
        std::exit(1);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: %s certificate.pgp\n", argv[0]);
        return 1;
    }

    rnp_ffi_t ffi = nullptr;
    rnp_input_t input = nullptr;
    rnp_key_handle_t key = nullptr;
    rnp_uid_handle_t uid = nullptr;
    char* import_result = nullptr;

    check("Create FFI", rnp_ffi_create(&ffi, "GPG", "GPG"));
    check("Open certificate", rnp_input_from_path(&input, argv[1]));

    check("Import certificate",
          rnp_import_keys(ffi, input, RNP_LOAD_SAVE_PUBLIC_KEYS, &import_result));

    if (import_result)
        rnp_buffer_destroy(import_result);

    const char* fingerprint =
        "07D8539F269583C160E333151763E15ED3E9286C";

    check("Locate key",
          rnp_locate_key(ffi, "fingerprint", fingerprint, &key));

    check("Get User ID",
          rnp_key_get_uid_handle_at(key, 0, &uid));

    size_t count = 0;
    check("Count signatures",
          rnp_uid_get_signature_count(uid, &count));

    std::printf("Certificate: %s\n", fingerprint);
    std::printf("User ID signatures: %zu\n", count);

    if (count == 0) {
        std::fprintf(stderr, "No certification signatures found\n");
        return 1;
    }

    bool all_valid = true;

    for (size_t i = 0; i < count; ++i) {
        rnp_signature_handle_t sig = nullptr;

        check("Get signature",
              rnp_uid_get_signature_at(uid, i, &sig));

        rnp_result_t status =
            rnp_signature_is_valid(sig, RNP_SIGNATURE_REVALIDATE);

        if (status == RNP_SUCCESS) {
            std::printf("Signature %zu: VALID\n", i);
        } else {
            std::printf("Signature %zu: FAILED: %s (0x%08x)\n",
                        i, rnp_result_to_string(status), status);
            all_valid = false;
        }

        rnp_signature_handle_destroy(sig);
    }

    rnp_uid_handle_destroy(uid);
    rnp_key_handle_destroy(key);
    rnp_input_destroy(input);
    rnp_ffi_destroy(ffi);

    return all_valid ? 0 : 1;
}
