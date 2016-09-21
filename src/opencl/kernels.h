#pragma once

namespace nano
{
        ///
        /// \brief basic linear algebra kernels:
        ///     c - constant
        //      v - vector
        ///     m - matrix
        ///     p - plus (addition)
        ///     N - N-elements at a time
        ///
        inline const char* opencl_kernels()
        {
                return R"xxx(

                float vsum4(const float4 in)
                {
                        const float4 unit = 1.0f;
                        return dot(in, unit);
                }

                float dotx4(__global const float* x, __global const float* y, const int size)
                {
                        float4 acc = 0.0f;
                        for (int i = 0; i < size; i += 4)
                        {
                                acc += vload4(0, &x[i]) * vload4(0, &y[i]);
                        }

                        return vsum4(acc);
                }

                ////////////////////////////////////////////////////////////////////////////////////
                // level 1 vector-vector operations
                ////////////////////////////////////////////////////////////////////////////////////

                __kernel void vpc(
                        __global const float* x,
                        const float c,
                        __global float* z, const int size)
                {
                        const int i = get_global_id(0);
                        vstore4(vload4(i, x) + c, i, z);
                }

                __kernel void vpv(
                        __global const float* x,
                        __global const float* y,
                        __global float* z, const int size)
                {
                        const int i = get_global_id(0);
                        vstore4(vload4(i, x) + vload4(i, y), i, z);
                }

                __kernel void vcpc(
                        __global const float* x, const float a,
                        const float c,
                        __global float* z, const int size)
                {
                        const int i = get_global_id(0);
                        vstore4(vload4(i, x) * a + c, i, z);
                }

                __kernel void vcpv(
                        __global const float* x, const float a,
                        __global const float* y,
                        __global float* z, const int size)
                {
                        const int i = get_global_id(0);
                        vstore4(vload4(i, x) * a + vload4(i, y), i, z);
                }

                __kernel void vcpvc(
                        __global const float* x, const float a,
                        __global const float* y, const float b,
                        __global float* z, const int size)
                {
                        const int i = get_global_id(0);
                        vstore4(vload4(i, x) * a + vload4(i, y) * b, i, z);
                }

                __kernel void vcpvcpc(
                        __global const float* x, const float a,
                        __global const float* y, const float b,
                        const float c,
                        __global float* z, const int size)
                {
                        const int i = get_global_id(0);
                        vstore4(vload4(i, x) * a + vload4(i, y) * b + c, i, z);
                }

                ////////////////////////////////////////////////////////////////////////////////////
                // level 2 matrix-vector operations
                ////////////////////////////////////////////////////////////////////////////////////

                __kernel void mv(
                        __global const float* A,
                        __global const float* x,
                        __global float* z, const int rows, const int cols)
                {
                        const int row = get_global_id(0);
                        z[row] = dotx4(&A[row * cols], x, cols);
                }

                __kernel void mvpc(
                        __global const float* A,
                        __global const float* x,
                        const float c,
                        __global float* z, const int rows, const int cols)
                {
                        const int row = get_global_id(0);
                        z[row] = dotx4(&A[row * cols], x, cols) + c;
                }

                __kernel void mvpv(
                        __global const float* A,
                        __global const float* x,
                        __global const float* y,
                        __global float* z, const int rows, const int cols)
                {
                        const int row = get_global_id(0);
                        z[row] = dotx4(&A[row * cols], x, cols) + y[row];
                }

                ////////////////////////////////////////////////////////////////////////////////////
                // level 3 matrix-matrix operations
                ////////////////////////////////////////////////////////////////////////////////////

#define LEVEL3_LOCAL_SIZE 16

                __kernel void mm(
                        __global const float* A,
                        __global const float* B,
                        __global float* Z, const int rowsA, const int colsA, const int colsB)
                {
                        const int rowA = get_global_id(0);
                        const int colB = get_global_id(1);

                        const int lrow = get_local_id(0);
                        const int lcol = get_local_id(1);

                        __local float Asub[LEVEL3_LOCAL_SIZE * LEVEL3_LOCAL_SIZE];
                        __local float Bsub[LEVEL3_LOCAL_SIZE * LEVEL3_LOCAL_SIZE];

                        __local float* arow = Asub + lrow * LEVEL3_LOCAL_SIZE;
                        __local float* bcol = Bsub + lcol * LEVEL3_LOCAL_SIZE;

                        float4 acc = 0.0f;
                        for (int colA = lcol, rowB = lrow; colA < colsA; colA += LEVEL3_LOCAL_SIZE, rowB += LEVEL3_LOCAL_SIZE)
                        {
                                arow[lcol] = A[rowA * colsA + colA];
                                bcol[lrow] = B[rowB * colsB + colB];
                                barrier(CLK_LOCAL_MEM_FENCE);

                                for (int k = 0; k < LEVEL3_LOCAL_SIZE/4; ++ k)
                                {
                                        acc += vload4(k, arow) * vload4(k, bcol);
                                }
                                barrier(CLK_LOCAL_MEM_FENCE);
                        }

                        Z[rowA * colsB + colB] = vsum4(acc);
                }

                )xxx";
        }
}

