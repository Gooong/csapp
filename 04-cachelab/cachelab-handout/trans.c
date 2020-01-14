// by gongxuri

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

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */

void t32(int M, int N, int A[N][M], int B[M][N])
{
  int ii, i, j, v0, v1, v2, v3, v4, v5, v6, v7;
  int step = 8;
  for (i = 0; i < N; i += step)
  {
    for (j = 0; j < M; j += step)
    {
      for (ii = 0; ii < step && ii + i < N; ii++)
      {
        v0 = A[i + ii][j + 0];
        v1 = A[i + ii][j + 1];
        v2 = A[i + ii][j + 2];
        v3 = A[i + ii][j + 3];
        v4 = A[i + ii][j + 4];
        v5 = A[i + ii][j + 5];
        v6 = A[i + ii][j + 6];
        v7 = A[i + ii][j + 7];

        B[j + 0][i + ii] = v0;
        B[j + 1][i + ii] = v1;
        B[j + 2][i + ii] = v2;
        B[j + 3][i + ii] = v3;
        B[j + 4][i + ii] = v4;
        B[j + 5][i + ii] = v5;
        B[j + 6][i + ii] = v6;
        B[j + 7][i + ii] = v7;
      }
    }
  }
}

void t64(int M, int N, int A[N][M], int B[M][N])
{
  int ii, i, j, v0, v1, v2, v3, v4, v5, v6, v7;
  int step = 4;
  for (i = 0; i < N; i += step)
  {
    for (j = 0; j < M; j += step)
    {
      for (ii = 0; ii < step && ii + i < N; ii += 2)
      {
        v0 = A[i + ii][j + 0];
        v1 = A[i + ii][j + 1];
        v2 = A[i + ii][j + 2];
        v3 = A[i + ii][j + 3];
        v4 = A[i + ii + 1][j + 0];
        v5 = A[i + ii + 1][j + 1];
        v6 = A[i + ii + 1][j + 2];
        v7 = A[i + ii + 1][j + 3];

        B[j + 0][i + ii] = v0;
        B[j + 1][i + ii] = v1;
        B[j + 2][i + ii] = v2;
        B[j + 3][i + ii] = v3;
        B[j + 0][i + ii + 1] = v4;
        B[j + 1][i + ii + 1] = v5;
        B[j + 2][i + ii + 1] = v6;
        B[j + 3][i + ii + 1] = v7;
      }
    }
  }
}

void t61(int M, int N, int A[N][M], int B[M][N])
{
  int ii, i, j, v0, v1, v2, v3, v4, v5, v6, v7;
  int step = 16;
  for (i = 0; i < N; i += step)
  {
    for (j = 0; j < M; j += step / 2)
    {
      for (ii = 0; ii < step && ii + i < N; ii++)
      {
        v0 = A[i + ii][j + 0];
        v1 = A[i + ii][j + 1];
        v2 = A[i + ii][j + 2];
        v3 = A[i + ii][j + 3];
        v4 = A[i + ii][j + 4];
        v5 = A[i + ii][j + 5];
        v6 = A[i + ii][j + 6];
        v7 = A[i + ii][j + 7];

        B[j + 0][i + ii] = v0;
        B[j + 1][i + ii] = v1;
        B[j + 2][i + ii] = v2;
        B[j + 3][i + ii] = v3;
        B[j + 4][i + ii] = v4;
        B[j + 5][i + ii] = v5;
        B[j + 6][i + ii] = v6;
        B[j + 7][i + ii] = v7;
      }
    }
  }
}

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
  if (M == 32)
    t32(M, N, A, B);
  else if (M == 64)
    t64(M, N, A, B);
  else
    t61(M, N, A, B);
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

  for (i = 0; i < N; i++)
  {
    for (j = 0; j < M; j++)
    {
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
  //registerTransFunction(trans, trans_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
  int i, j;

  for (i = 0; i < N; i++)
  {
    for (j = 0; j < M; ++j)
    {
      if (A[i][j] != B[j][i])
      {
        return 0;
      }
    }
  }
  return 1;
}
