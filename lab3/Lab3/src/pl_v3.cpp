/**********
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/*******************************************************************************
Description:
    HLS pragmas can be used to optimize the design : improve throughput, reduce
latency and
    device resource utilization of the resulting RTL code
    This is vector addition example to demonstrate how HLS optimizations are
used in kernel.
*******************************************************************************/

#include "hls_stream.h"
#include "ap_int.h"

typedef int DTYPE;
const int M = 256;
const int PORT_WIDTH_B = 64;
const int PORT_WIDTH_b = PORT_WIDTH_B * 8;
const int DTYPE_WIDTH_b = sizeof(DTYPE) * 8;
typedef ap_int<PORT_WIDTH_b> block_t;
const int DTYPE_PER_PORT = PORT_WIDTH_B / sizeof(DTYPE);

extern "C" {

void pl(block_t *A_p,  block_t *B_p, block_t *C_p,   int N )
{
#pragma HLS INTERFACE m_axi port = A_p offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = B_p offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = C_p offset = slave bundle = gmem2
#pragma HLS INTERFACE s_axilite port = A_p bundle = control
#pragma HLS INTERFACE s_axilite port = B_p bundle = control
#pragma HLS INTERFACE s_axilite port = C_p bundle = control
#pragma HLS INTERFACE s_axilite port = N bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    DTYPE C_block[M][M];
    // Fill This Part !!! Add pragma to partition C_block
    #pragma HLS array_partition variable=C_block complete dim=2



    ib_loop: for(int ib = 0; ib < N/M; ib++) {
        jb_loop: for(int jb = 0; jb < N/M; jb++) {
            init_i_loop: for(int i = 0; i < M; i++) {
                // Fill This Part !!!
                // Add #pragma HLS pipeline II=1 pragma in init_i_loop loop, therefore, init_j_loop is fully unrolled
                #pragma HLS pipeline II=1
                init_j_loop: for(int j = 0; j < M; j++) {
                    C_block[i][j] = 0;
                }
            }

            kb_loop: for(int kb = 0; kb < N/M; kb++) {
                k_loop: for(int k = 0; k < M; k++) {
                    DTYPE Bj[M];
                    // Fill This Part !!!
                    // Add pragma to partition Bj
                    readB_j_loop: for(int jj = 0; jj < M/DTYPE_PER_PORT; jj++) {
                        #pragma HLS pipeline II=1
                        block_t B_temp = B_p[((kb*M+k)*N+jb*M)/DTYPE_PER_PORT+jj];
						for (int j = 0; j < DTYPE_PER_PORT; j++) {
                            #pragma HLS unroll
                            // Fill This Part !!! 
							// Assign temp to Bj
						}
                    }

					DTYPE A_line[M];
                    // Technically doesn't need to be complete partition, could be cyclic.
                    #pragma HLS array_partition variable=A_line complete

					// Load in line of A
                    ii_loop: for(int ii = 0; ii < M/DTYPE_PER_PORT; ii++) {
                        // Fill This Part !!! 
                        // Add #pragma HLS pipeline II=1 pragma in ii_loop loop, therefore, in_i_loop is fully unrolled
						block_t Ai_block =  A_p[((kb*M+k)*N+ib*M)/DTYPE_PER_PORT+ii];
						in_i_loop: for (int i = 0; i < DTYPE_PER_PORT; i++) {
                            #pragma HLS unroll
                            // Fill This Part !!! 
                            // Assign temp to A_line

						}
					}
					
					i_loop: for(int i = 0; i < M; i++) {
                        // Fill This Part !!! 
                        // Add #pragma HLS pipeline II=1 pragma in i loop, therefore, j is fully unrolled
                        
						j_loop: for(int j = 0; j < M; j++) {
                            #pragma HLS unroll
							C_block[i][j] += A_line[i] * Bj[j];
						}
					}
                }
            }

            writeC_i_loop: for(int i = 0; i < M; i++) {
                writeC_j_loop: for(int jj = 0; jj < M/DTYPE_PER_PORT; jj++) {
                    #pragma HLS pipeline II=1
					block_t C_temp;
					for (int j = 0; j < DTYPE_PER_PORT; j++) {
                        #pragma HLS unroll
						C_temp(DTYPE_WIDTH_b * (j + 1) - 1, DTYPE_WIDTH_b * j) = (ap_int<DTYPE_WIDTH_b>) C_block[i][jj*DTYPE_PER_PORT+j]; 
					}
					C_p[((ib*M+i)*N+jb*M)/DTYPE_PER_PORT+jj] = C_temp;
                }
            }
        }
    }
}

}
