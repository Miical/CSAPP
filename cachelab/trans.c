/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void transpose32_32(int A[32][32], int B[32][32]);
void transpose64_64(int A[64][64], int B[64][64]);
void transpose61_67(int A[67][61], int B[61][67]);
void trans(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    if (N == 32 && M == 32) {
        transpose32_32(A, B);
    } else if (N == 64 && M == 64) {
        transpose64_64(A, B);
    } else if (N == 67 && M == 61) {
        transpose61_67(A, B);
    } else {
        trans(M, N, A, B);
    }
}

void transpose32_32(int A[32][32], int B[32][32]) {
    #define BLOCK_SZ 8
    #define N 32
    #define M 32
    #define REVERSE_ID(i) (BLOCK_SZ - 1 - (i - block_i * BLOCK_SZ) + block_i * BLOCK_SZ)

    int tmp;
    for (int block_i = 0; block_i < N / BLOCK_SZ; block_i++) {
        for (int block_j = 0; block_j < M / BLOCK_SZ; block_j++) {
            for (int i = block_i * BLOCK_SZ; i < (block_i + 1) * BLOCK_SZ; i++) {
                for (int j = block_j * BLOCK_SZ; j < (block_j + 1) * BLOCK_SZ; j++) {
                    tmp = A[i][j];
                    if (i == j) {
                        B[REVERSE_ID(i)][REVERSE_ID(i)] = tmp;
                    } else {
                        B[j][i] = tmp;
                    }
                }
            }
            if (block_i == block_j) {
                for (int i = block_i * BLOCK_SZ; i < block_i * BLOCK_SZ + BLOCK_SZ / 2; i++) {
                    tmp = B[i][i];
                    B[i][i] = B[REVERSE_ID(i)][REVERSE_ID(i)];
                    B[REVERSE_ID(i)][REVERSE_ID(i)] = tmp;
                }
            }
        }
    }

    #undef N
    #undef M
    #undef BLOCK_SZ
    #undef REVERSE_ID
}

void transpose64_64(int A[64][64], int B[64][64]) {
    #define N 64
    int i, j, x, y;
    int a, b, c, d, e, f, g, h;
    for (y = 0; y < N; y += 8) {
        for (x = 0; x < N; x += 8) {
            for (i = y; i < y + 4; i++) {
                j = x;
                a = A[i][j], b = A[i][j+1], c = A[i][j+2], d = A[i][j+3],
                e = A[i][j+4], f = A[i][j+5], g = A[i][j+6], h = A[i][j+7];

                B[j][i] = a; B[j+1][i] = b; B[j+2][i] = c; B[j+3][i] = d;
                B[j][i+4] = e; B[j+1][i+4] = f; B[j+2][i+4] = g; B[j+3][i+4] = h;
            }

            for (j = x; j < x + 4; j++) {
                i = y + 4;
                a = B[j][i], b = B[j][i+1], c = B[j][i+2], d = B[j][i+3];
                B[j][i] = A[i][j]; B[j][i+1] = A[i+1][j]; B[j][i+2] = A[i+2][j]; B[j][i+3] = A[i+3][j];
                B[j+4][i-4] = a; B[j+4][i-3] = b; B[j+4][i-2] = c; B[j+4][i-1] = d;
            }

            for (i = y + 4; i < y + 8; i++) {
                j = x + 4;
                a = A[i][j], b = A[i][j+1], c = A[i][j+2], d = A[i][j+3];
                B[j][i] = a; B[j+1][i] = b; B[j+2][i] = c; B[j+3][i] = d;
            }
        }
    }

    #undef N
}

void transpose61_67(int A[67][61], int B[61][67]) {
    #define N 67
    #define M 61
    int i, j, x, y;
    for (y = 0; y < N; y += 16) {
        for (x = 0; x < M; x += 16) {
            for (i = y; i < y + 16 && i < N; i++) {
                for (j = x; j < x + 16 && j < M; j++) {
                    B[j][i] = A[i][j];
                }
            }
        }
    }

    #undef N
    #undef M
}


/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

