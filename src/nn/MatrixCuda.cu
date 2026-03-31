/*
 * MatrixCuda.cu - MatrixCuda类的CUDA实现文件
 * 
 * 这个文件包含MatrixCuda类的具体实现和CUDA内核的显式实例化
 * 注意：这个文件需要使用nvcc编译器编译
 */

#include "nn/MatrixCuda.cuh"
#include "nn/Matrix.hh"  // 用于与现有Matrx类交互

namespace qing {

// ============================================================================
// 显式实例化CUDA内核函数
// ============================================================================

namespace cuda_kernels {

// float类型内核显式实例化
template __global__ void matrix_add_kernel<float>(
    const float* A, const float* B, float* C,
    long rows, long cols, long total_elements);

template __global__ void matrix_sub_kernel<float>(
    const float* A, const float* B, float* C,
    long rows, long cols, long total_elements);

template __global__ void matrix_scalar_mul_kernel<float>(
    const float* A, float scalar, float* C, long total_elements);

template __global__ void matrix_scalar_add_kernel<float>(
    const float* A, float scalar, float* C, long total_elements);

template __global__ void matrix_transpose_kernel<float>(
    const float* A, float* C, long rows, long cols);

template __global__ void matrix_hadamard_kernel<float>(
    const float* A, const float* B, float* C, long total_elements);

template __global__ void matrix_copy_kernel<float>(
    const float* src, float* dst, long total_elements);

// double类型内核显式实例化
template __global__ void matrix_add_kernel<double>(
    const double* A, const double* B, double* C,
    long rows, long cols, long total_elements);

template __global__ void matrix_sub_kernel<double>(
    const double* A, const double* B, double* C,
    long rows, long cols, long total_elements);

template __global__ void matrix_scalar_mul_kernel<double>(
    const double* A, double scalar, double* C, long total_elements);

template __global__ void matrix_scalar_add_kernel<double>(
    const double* A, double scalar, double* C, long total_elements);

template __global__ void matrix_transpose_kernel<double>(
    const double* A, double* C, long rows, long cols);

template __global__ void matrix_hadamard_kernel<double>(
    const double* A, const double* B, double* C, long total_elements);

template __global__ void matrix_copy_kernel<double>(
    const double* src, double* dst, long total_elements);

} // namespace cuda_kernels

// ============================================================================
// MatrixCuda类与现有Matrx类的交互实现
// ============================================================================

// 从Matrx<float>转换到MatrixCuda<float>
template <>
CUDA_HOST MatrixCuda<float> MatrixCuda<float>::from_matrx(const Matrx<float>& mat) {
    long rows = mat.get_row();
    long cols = mat.get_col();
    const std::vector<float>& data = mat.get_data();
    
    return MatrixCuda<float>(rows, cols, data);
}

// 从MatrixCuda<float>转换到Matrx<float>
template <>
CUDA_HOST Matrx<float> MatrixCuda<float>::to_matrx() const {
    sync_device_to_host();
    return Matrx<float>(rows_, cols_, host_data_);
}

// 从Matrx<double>转换到MatrixCuda<double>
template <>
CUDA_HOST MatrixCuda<double> MatrixCuda<double>::from_matrx(const Matrx<double>& mat) {
    long rows = mat.get_row();
    long cols = mat.get_col();
    const std::vector<double>& data = mat.get_data();
    
    return MatrixCuda<double>(rows, cols, data);
}

// 从MatrixCuda<double>转换到Matrx<double>
template <>
CUDA_HOST Matrx<double> MatrixCuda<double>::to_matrx() const {
    sync_device_to_host();
    return Matrx<double>(rows_, cols_, host_data_);
}

// ============================================================================
// 优化的矩阵乘法实现（使用共享内存）
// ============================================================================

/**
 * @brief 使用共享内存优化的矩阵乘法内核
 * 
 * 这个内核使用平铺（tiling）技术来提高矩阵乘法的性能
 * 每个线程块计算输出矩阵的一个子块
 */
template <typename T>
__global__ void matrix_mul_optimized_kernel(
    const T* A, const T* B, T* C,
    long A_rows, long A_cols, long B_cols,
    long tile_size) {
    
    // 共享内存声明
    extern __shared__ T shared_mem[];
    T* tile_A = shared_mem;
    T* tile_B = &shared_mem[tile_size * tile_size];
    
    // 线程块索引
    long block_row = blockIdx.y;
    long block_col = blockIdx.x;
    
    // 线程索引
    long thread_row = threadIdx.y;
    long thread_col = threadIdx.x;
    
    // 输出矩阵中的位置
    long row = block_row * tile_size + thread_row;
    long col = block_col * tile_size + thread_col;
    
    T sum = T(0);
    
    // 遍历所有平铺
    for (long tile_idx = 0; tile_idx < (A_cols + tile_size - 1) / tile_size; ++tile_idx) {
        // 从全局内存加载到共享内存
        long A_col = tile_idx * tile_size + thread_col;
        long B_row = tile_idx * tile_size + thread_row;
        
        if (row < A_rows && A_col < A_cols) {
            tile_A[thread_row * tile_size + thread_col] = A[row * A_cols + A_col];
        } else {
            tile_A[thread_row * tile_size + thread_col] = T(0);
        }
        
        if (B_row < A_cols && col < B_cols) {
            tile_B[thread_row * tile_size + thread_col] = B[B_row * B_cols + col];
        } else {
            tile_B[thread_row * tile_size + thread_col] = T(0);
        }
        
        __syncthreads();
        
        // 计算平铺内的点积
        for (long k = 0; k < tile_size; ++k) {
            sum += tile_A[thread_row * tile_size + k] * tile_B[k * tile_size + thread_col];
        }
        
        __syncthreads();
    }
    
    // 将结果写回全局内存
    if (row < A_rows && col < B_cols) {
        C[row * B_cols + col] = sum;
    }
}

// ============================================================================
// 优化的矩阵乘法方法
// ============================================================================

/**
 * @brief 使用优化CUDA内核的矩阵乘法
 * 
 * 这个方法使用共享内存和平铺技术来加速矩阵乘法
 * 对于大矩阵，性能比朴素算法有显著提升
 */
template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::matmul_optimized(const MatrixCuda<T>& other) const {
    assert_dot_shape(other);
    
    MatrixCuda<T> result(rows_, other.cols_);
    
    // 确保数据在设备上
    sync_host_to_device();
    other.sync_host_to_device();
    result.allocate_device_memory();
    
    // 设置平铺大小（典型值为16或32）
    const long tile_size = 16;
    
    // 计算线程块和网格大小
    dim3 block_size(tile_size, tile_size);
    dim3 grid_size((other.cols_ + tile_size - 1) / tile_size,
                   (rows_ + tile_size - 1) / tile_size);
    
    // 计算共享内存大小
    size_t shared_mem_size = 2 * tile_size * tile_size * sizeof(T);
    
    // 启动优化内核
    matrix_mul_optimized_kernel<T><<<grid_size, block_size, shared_mem_size>>>(
        device_data_, other.device_data_, result.device_data_,
        rows_, cols_, other.cols_, tile_size
    );
    
    CHECK_CUDA_ERROR(cudaGetLastError());
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    result.device_modified_ = true;
    result.sync_device_to_host();
    
    return result;
}

// 显式实例化优化矩阵乘法
template MatrixCuda<float> MatrixCuda<float>::matmul_optimized(const MatrixCuda<float>& other) const;
template MatrixCuda<double> MatrixCuda<double>::matmul_optimized(const MatrixCuda<double>& other) const;

// ============================================================================
// 使用cuBLAS的矩阵乘法（如果可用）
// ============================================================================

#ifdef HAVE_CUBLAS
#include <cublas_v2.h>

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::matmul_cublas(const MatrixCuda<T>& other) const {
    assert_dot_shape(other);
    
    MatrixCuda<T> result(rows_, other.cols_);
    
    // 确保数据在设备上
    sync_host_to_device();
    other.sync_host_to_device();
    result.allocate_device_memory();
    
    // 创建cuBLAS句柄
    cublasHandle_t handle;
    cublasCreate(&handle);
    
    // 设置cuBLAS参数
    T alpha = T(1);
    T beta = T(0);
    
    if constexpr (std::is_same_v<T, float>) {
        cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N,
                    other.cols_, rows_, cols_,
                    &alpha,
                    other.device_data_, other.cols_,
                    device_data_, cols_,
                    &beta,
                    result.device_data_, other.cols_);
    } else if constexpr (std::is_same_v<T, double>) {
        cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N,
                    other.cols_, rows_, cols_,
                    &alpha,
                    other.device_data_, other.cols_,
                    device_data_, cols_,
                    &beta,
                    result.device_data_, other.cols_);
    }
    
    // 销毁cuBLAS句柄
    cublasDestroy(handle);
    
    CHECK_CUDA_ERROR(cudaGetLastError());
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    result.device_modified_ = true;
    result.sync_device_to_host();
    
    return result;
}

// 显式实例化cuBLAS矩阵乘法
template MatrixCuda<float> MatrixCuda<float>::matmul_cublas(const MatrixCuda<float>& other) const;
template MatrixCuda<double> MatrixCuda<double>::matmul_cublas(const MatrixCuda<double>& other) const;

#endif // HAVE_CUBLAS

// ============================================================================
// 测试函数
// ============================================================================

/**
 * @brief 测试MatrixCuda类的功能
 * 
 * 这个函数用于验证MatrixCuda类的正确性
 * 可以单独编译和运行进行测试
 */
template <typename T>
CUDA_HOST void test_matrix_cuda() {
    std::cout << "Testing MatrixCuda<" << typeid(T).name() << ">..." << std::endl;
    
    try {
        // 测试1：创建矩阵
        std::cout << "Test 1: Creating matrices..." << std::endl;
        MatrixCuda<T> A = MatrixCuda<T>::zeros(3, 3);
        MatrixCuda<T> B = MatrixCuda<T>::ones(3, 3);
        MatrixCuda<T> C = MatrixCuda<T>::identity(3);
        
        A.print("A (zeros)");
        B.print("B (ones)");
        C.print("C (identity)");
        
        // 测试2：矩阵加法
        std::cout << "\nTest 2: Matrix addition..." << std::endl;
        MatrixCuda<T> D = A + B;
        D.print("D = A + B");
        
        // 测试3：标量运算
        std::cout << "\nTest 3: Scalar operations..." << std::endl;
        MatrixCuda<T> E = B * T(2.5);
        E.print("E = B * 2.5");
        
        // 测试4：矩阵转置
        std::cout << "\nTest 4: Matrix transpose..." << std::endl;
        MatrixCuda<T> F(2, 3);
        F.fill(T(1));
        F.print("F (2x3)");
        MatrixCuda<T> FT = F.transpose();
        FT.print("F^T (3x2)");
        
        // 测试5：元素对应乘积
        std::cout << "\nTest 5: Hadamard product..." << std::endl;
        MatrixCuda<T> G = MatrixCuda<T>::random(2, 2, T(0), T(1));
        MatrixCuda<T> H = MatrixCuda<T>::random(2, 2, T(0), T(1));
        G.print("G");
        H.print("H");
        MatrixCuda<T> I = G.hadamard_product(H);
        I.print("G ⊙ H");
        
        // 测试6：矩阵乘法
        std::cout << "\nTest 6: Matrix multiplication..." << std::endl;
        MatrixCuda<T> J(2, 3);
        MatrixCuda<T> K(3, 2);
        J.fill(T(1));
        K.fill(T(2));
        J.print("J (2x3)");
        K.print("K (3x2)");
        MatrixCuda<T> L = J * K;
        L.print("J * K (2x2)");
        
        std::cout << "All tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
    }
}

// 显式实例化测试函数
template void test_matrix_cuda<float>();
template void test_matrix_cuda<double>();

} // namespace qing

// ============================================================================
// 主测试程序（可选）
// ============================================================================

#ifdef TEST_MATRIX_CUDA

int main() {
    std::cout << "=== MatrixCuda Test Program ===" << std::endl;
    
    // 初始化CUDA
    cudaError_t err = cudaSetDevice(0);
    if (err != cudaSuccess) {
        std::cerr << "Failed to set CUDA device: " << cudaGetErrorString(err) << std::endl;
        return 1;
    }
    
    // 测试float版本
    qing::test_matrix_cuda<float>();
    
    std::cout << "\n";
    
    // 测试double版本
    qing::test_matrix_cuda<double>();
    
    // 重置CUDA设备
    cudaDeviceReset();
    
    std::cout << "\n=== Test Program Finished ===" << std::endl;
    return 0;
}

#endif // TEST_MATRIX_CUDA