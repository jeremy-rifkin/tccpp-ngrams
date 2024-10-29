#ifndef SHA_HPP
#define SHA_HPP

#include <array>
#include <string_view>

#include <openssl/evp.h>
#include <openssl/sha.h>

#include "ngram.hpp"

// This is a really really awful hack to squeeze some more performance out of openssl's setup
struct ShaContextManager {
    EVP_MD_CTX* working_context = EVP_MD_CTX_create();
    EVP_MD_CTX* sha_256_fixed_context = EVP_MD_CTX_create();
    const EVP_MD* sha256 = EVP_MD_fetch(NULL, "SHA256", NULL);
    ShaContextManager() {
        EVP_DigestInit_ex(working_context, sha256, NULL); // ex or ex2
        EVP_DigestInit_ex(sha_256_fixed_context, sha256, NULL); // ex or ex2
    }
    ~ShaContextManager() {
        EVP_MD_CTX_destroy(working_context);
        EVP_MD_CTX_destroy(sha_256_fixed_context);
    }
    EVP_MD_CTX* fresh_context() {
        EVP_MD_CTX_copy(working_context, sha_256_fixed_context);
        return working_context;
    }
};

inline ShaContextManager sha_context_manager;

using sha256_digest = std::array<unsigned char, 256 / 8>;

template<typename T, size_t N>
sha256_digest sha256(const ngram_tmpl<T, N>& ngram, std::string_view nonce) {
    sha256_digest dst;

    auto* context = sha_context_manager.fresh_context();
    for(const auto& part : ngram) {
        EVP_DigestUpdate(context, part.data(), part.size());
    }
    EVP_DigestUpdate(context, nonce.data(), nonce.size());
    auto n = N;
    EVP_DigestUpdate(context, &n, sizeof(n));
    EVP_DigestFinal_ex(context, dst.data(), 0);

    // This is faster but deprecated...
    // SHA256_CTX sha256;
    // SHA256_Init(&sha256);
    // for(const auto& part : ngram) {
    //     SHA256_Update(&sha256, part.data(), part.size());
    // }
    // SHA256_Update(&sha256, nonce.data(), nonce.size());
    // auto n = N;
    // SHA256_Update(&sha256, &n, sizeof(n));
    // SHA256_Final(dst.data(), &sha256);

    // Deprecated API:
    // -----------------------------------------------------
    // Benchmark           Time             CPU   Iterations
    // -----------------------------------------------------
    // Sha_1gram        39.9 ns         39.9 ns     17513538
    // Sha_2gram        42.0 ns         42.0 ns     16668647
    // Sha_3gram        45.9 ns         45.9 ns     15248781
    // Sha_4gram        49.4 ns         49.4 ns     14171578
    // Sha_5gram        55.9 ns         55.9 ns     12714481

    // EVP API:
    // -----------------------------------------------------
    // Benchmark           Time             CPU   Iterations
    // -----------------------------------------------------
    // Sha_1gram        67.3 ns         67.3 ns     10209807
    // Sha_2gram        77.8 ns         77.7 ns      9129239
    // Sha_3gram        82.5 ns         82.5 ns      8526000
    // Sha_4gram        88.9 ns         88.9 ns      7885772
    // Sha_5gram         127 ns          127 ns      5521589
    // (both sets of benchmarks before some modifications above)

    return dst;
}

#endif
