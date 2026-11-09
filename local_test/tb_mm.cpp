// Local test bench for the Lab 3 matrix multiply kernels (V3, V4).
// It does the same thing as src/host.cpp, but calls pl() directly
// instead of going through XRT and the FPGA.
//
// It checks the kernel gives the right answer. It does NOT give real
// FPGA timing or resource numbers. Never put these times in the report.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "ap_int.h"
#include "hls_stream.h"

#ifndef SIZE
#define SIZE 512
#endif

typedef int DTYPE;
typedef ap_int<512> block_t;
const int DTYPE_PER_PORT = 16;

extern "C" void pl(block_t *A_p, block_t *B_p, block_t *C_p, int N);

// same as mm_sw in host.cpp (A is passed in transposed: At[k*SIZE + i])
static void mm_sw(const std::vector<DTYPE> &At, const std::vector<DTYPE> &B, std::vector<DTYPE> &C) {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++) {
            DTYPE sum = 0;
            for (int k = 0; k < SIZE; k++) sum += At[k * SIZE + i] * B[k * SIZE + j];
            C[i * SIZE + j] = sum;
        }
}

static void pack(const std::vector<DTYPE> &src, std::vector<block_t> &dst) {
    for (size_t w = 0; w < dst.size(); w++) {
        block_t t = 0;
        for (int j = 0; j < DTYPE_PER_PORT; j++) t(32 * j + 31, 32 * j) = (ap_int<32>)src[w * DTYPE_PER_PORT + j];
        dst[w] = t;
    }
}

static void unpack(const std::vector<block_t> &src, std::vector<DTYPE> &dst) {
    for (size_t w = 0; w < src.size(); w++)
        for (int j = 0; j < DTYPE_PER_PORT; j++)
            dst[w * DTYPE_PER_PORT + j] = (DTYPE)(ap_int<32>)src[w](32 * j + 31, 32 * j);
}

int main() {
    const int n = SIZE * SIZE;
    printf("Local C test, SIZE = %d\n", SIZE);
    if (SIZE % 256 != 0) {
        printf("SIZE must be a multiple of 256 (the tile size M)\n");
        return 1;
    }

    std::vector<DTYPE> A(n), B(n), C_sw(n), C_hw(n);
    for (int i = 0; i < n; ++i) {   // same random data as host.cpp
        A[i] = rand() % 8;
        B[i] = rand() % 8;
    }

    std::vector<block_t> A_p(n / DTYPE_PER_PORT), B_p(n / DTYPE_PER_PORT), C_p(n / DTYPE_PER_PORT);
    pack(A, A_p);
    pack(B, B_p);
    // fill C with junk so a kernel that skips writes is caught
    for (auto &c : C_p) c = -12345;

    printf("Running kernel on CPU...\n");
    auto t0 = std::chrono::high_resolution_clock::now();
    pl(A_p.data(), B_p.data(), C_p.data(), SIZE);
    auto t1 = std::chrono::high_resolution_clock::now();
    printf("Done (CPU time %.2f s, NOT an FPGA number).\n", std::chrono::duration<double>(t1 - t0).count());

    unpack(C_p, C_hw);
    mm_sw(A, B, C_sw);

    int err_cnt = 0;
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (C_sw[i * SIZE + j] != C_hw[i * SIZE + j]) {
                if (++err_cnt <= 5)
                    printf("mismatch i:%d j:%d sw:%d hw:%d\n", i, j, C_sw[i * SIZE + j], C_hw[i * SIZE + j]);
            }

    if (hls::g_stream_leftover) {
        printf("LOCAL TEST FAILED! Stream counts do not match (see message above).\n");
        return 1;
    }
    if (err_cnt != 0) {
        printf("LOCAL TEST FAILED! Error count : %d\n", err_cnt);
        return 1;
    }
    printf("LOCAL TEST PASSED!\n");
    return 0;
}
