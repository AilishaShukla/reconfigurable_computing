# V2 Design and Debugging

## Definitions

```cpp
typedef int DTYPE;
const int M = 256;
const int PORT_WIDTH_B = 64;
const int PORT_WIDTH_b = PORT_WIDTH_B * 8;
const int DTYPE_WIDTH_b = sizeof(DTYPE) * 8;
typedef ap_int<PORT_WIDTH_b> block_t;
const int DTYPE_PER_PORT = PORT_WIDTH_B / sizeof(DTYPE);
```

- `block_t` is 512 bits.
- Each INT is 32 bits.
- `DTYPE_PER_PORT` is 16 INTs per 512-bit word.

## What V2 needs

- C block partition
- Pipelined init loop
- B array partition
- Pipelined B read loop
- Read from the packed B pointer
- Unpack each packed B word into `Bj`
- Pipelined `i` loop
- Pipelined C write loop
- Pack the C block into 512-bit C words
- Write to the packed C pointer

## Addressing

The V0 source is the reference. The host confirms B and C are flat `SIZE x SIZE` scalar integer arrays.

```cpp
readB_j_loop: for(int j = 0; j < M; j++) {
    int B_temp = B[(kb*M+k)*N+jb*M+j];
    Bj[j] = B_temp;
}
```

```cpp
writeC_j_loop: for(int j = 0; j < M; j++) {
    C[(ib*M+i)*N+jb*M+j] = C_block[i][j];
}
```

| Array | V0 scalar index | V2 packed word index |
|---|---|---|
| B | `(kb*M + k)*N + jb*M + j` | `((kb*M + k)*N + jb*M) / 16 + jj` |
| C | `(ib*M + i)*N + jb*M + j` | `((ib*M + i)*N + jb*M) / 16 + jj` |

Packing 16 INTs into one word divides the scalar index by 16. This is only exact when N and M are multiples of 16, which holds for N=512 and M=256. Keep it in mind if using smaller matrices for emulation.

## First attempt: failed

```cpp
#pragma HLS array_partition variable=C_block complete dim=2
#pragma HLS pipeline II=1
#pragma HLS array_partition variable=Bj complete
#pragma HLS pipeline II=1
B_temp = B[((kb * M + k) * (M / DTYPE_PER_PORT)) + jj];
Bj[j] = B_temp(j*DTYPE_WIDTH_b + DTYPE_WIDTH_b - 1, j*DTYPE_WIDTH_b);
#pragma HLS pipeline II=1
#pragma HLS pipeline II=1
C_temp(j*DTYPE_WIDTH_b + DTYPE_WIDTH_b - 1, j*DTYPE_WIDTH_b) = C_block[i][jj*DTYPE_PER_PORT+j];
C[((ib*M+i)*N + jb*M)/DTYPE_PER_PORT + jj] = C_temp;
```

The hardware build succeeded, but the test failed:

```text
i:0 j:0 sw:6391 hw:5959
TEST FAILED! Error count : 262124
```

Likely causes, from comparing with the V0 source:

1. **Wrong B address.** It left out `jb` and used `M` where the scalar address uses `N`.
2. **Incomplete `Bj`.** `Bj[j]` only overwrote the first 16 elements instead of filling all 256.

## Corrected source

```cpp
block_t B_temp = 0;

B_temp = B[((kb * M + k) * N + jb * M) / DTYPE_PER_PORT + jj];

for(int j = 0; j < DTYPE_PER_PORT; j++) {
    Bj[jj * DTYPE_PER_PORT + j] =
        B_temp(j*DTYPE_WIDTH_b + DTYPE_WIDTH_b - 1,
               j*DTYPE_WIDTH_b);
}
```

`Bj[jj*16+j]` now fills all 256 entries.

C packing was left unchanged:

```cpp
C_temp(j*DTYPE_WIDTH_b + DTYPE_WIDTH_b - 1, j*DTYPE_WIDTH_b)
    = C_block[i][jj*DTYPE_PER_PORT+j];

C[((ib*M+i)*N + jb*M)/DTYPE_PER_PORT + jj] = C_temp;
```

The B changes were confirmed in `src/pl.cpp` at lines 94, 96, and 100.

## Status

**Fixed.** The corrected V2 passed on hardware at 0.0082593 s and 32.501 GOPS, as recorded by Ailisha. Only the B code changed, so the pass supports the diagnosis above.
