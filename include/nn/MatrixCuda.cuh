/*
 * MatrixCuda.cuh - CUDA加速的矩阵运算库
 * 
 * 基于现有的Matrx类设计，提供GPU加速的矩阵运算
 * 支持float和double类型的矩阵运算
 * 
 * 注意：使用CUDA需要安装NVIDIA CUDA Toolkit
 */

#pragma once

#ifdef __CUDACC__
#define CUDA_HOST_DEVICE __host__ __device__
#define CUDA_HOST __host__
#define CUDA_DEVICE __device__
#else
#define CUDA_HOST_DEVICE
#define CUDA_HOST
#define CUDA_DEVICE
#endif

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstring>

namespace qing {

// 前向声明
template <typename T>
class MatrixCuda;

// CUDA内核函数声明
namespace cuda_kernels {

// 矩阵加法内核
template <typename T>
__global__ void matrix_add_kernel(const T* A, const T* B, T* C, 
                                  long rows, long cols, long total_elements);

// 矩阵减法内核
template <typename T>
__global__ void matrix_sub_kernel(const T* A, const T* B, T* C,
                                  long rows, long cols, long total_elements);

// 矩阵标量乘法内核
template <typename T>
__global__ void matrix_scalar_mul_kernel(const T* A, T scalar, T* C,
                                         long total_elements);

// 矩阵标量加法内核
template <typename T>
__global__ void matrix_scalar_add_kernel(const T* A, T scalar, T* C,
                                         long total_elements);

// 矩阵转置内核
template <typename T>
__global__ void matrix_transpose_kernel(const T* A, T* C,
                                        long rows, long cols);

// 矩阵元素对应乘积内核
template <typename T>
__global__ void matrix_hadamard_kernel(const T* A, const T* B, T* C,
                                       long total_elements);

// 矩阵复制内核
template <typename T>
__global__ void matrix_copy_kernel(const T* src, T* dst, long total_elements);

} // namespace cuda_kernels

/**
 * @brief CUDA加速的矩阵类
 * 
 * 这个类提供了GPU加速的矩阵运算，包括：
 * - 矩阵加法、减法
 * - 标量运算
 * - 矩阵转置
 * - 元素对应乘积
 * - 矩阵乘法（使用cuBLAS）
 */
template <typename T>
class MatrixCuda {
public:
    // 构造函数和析构函数
    CUDA_HOST MatrixCuda();
    CUDA_HOST MatrixCuda(long rows, long cols);
    CUDA_HOST MatrixCuda(long rows, long cols, const std::vector<T>& data);
    CUDA_HOST MatrixCuda(const MatrixCuda<T>& other);
    CUDA_HOST MatrixCuda(MatrixCuda<T>&& other) noexcept;
    CUDA_HOST ~MatrixCuda();
    
    // 赋值运算符
    CUDA_HOST MatrixCuda<T>& operator=(const MatrixCuda<T>& other);
    CUDA_HOST MatrixCuda<T>& operator=(MatrixCuda<T>&& other) noexcept;
    
    // 内存管理
    CUDA_HOST void allocate_device_memory();
    CUDA_HOST void free_device_memory();
    CUDA_HOST void copy_to_device();
    CUDA_HOST void copy_from_device();
    CUDA_HOST void sync_device_to_host();
    CUDA_HOST void sync_host_to_device();
    
    // 访问器
    CUDA_HOST_DEVICE long get_rows() const { return rows_; }
    CUDA_HOST_DEVICE long get_cols() const { return cols_; }
    CUDA_HOST_DEVICE long get_total_elements() const { return rows_ * cols_; }
    
    CUDA_HOST const std::vector<T>& get_host_data() const { return host_data_; }
    CUDA_HOST T* get_device_data() const { return device_data_; }
    
    CUDA_HOST T get_element(long row, long col) const;
    CUDA_HOST void set_element(long row, long col, T value);
    
    // 形状检查
    CUDA_HOST bool has_same_shape(const MatrixCuda<T>& other) const;
    CUDA_HOST bool has_dot_shape(const MatrixCuda<T>& other) const;
    CUDA_HOST void assert_same_shape(const MatrixCuda<T>& other) const;
    CUDA_HOST void assert_dot_shape(const MatrixCuda<T>& other) const;
    
    // 矩阵运算（GPU加速）
    CUDA_HOST MatrixCuda<T> add(const MatrixCuda<T>& other) const;
    CUDA_HOST MatrixCuda<T> subtract(const MatrixCuda<T>& other) const;
    CUDA_HOST MatrixCuda<T> multiply_scalar(T scalar) const;
    CUDA_HOST MatrixCuda<T> add_scalar(T scalar) const;
    CUDA_HOST MatrixCuda<T> transpose() const;
    CUDA_HOST MatrixCuda<T> hadamard_product(const MatrixCuda<T>& other) const;
    
    // 运算符重载
    CUDA_HOST MatrixCuda<T> operator+(const MatrixCuda<T>& other) const;
    CUDA_HOST MatrixCuda<T> operator+(T scalar) const;
    CUDA_HOST MatrixCuda<T> operator-(const MatrixCuda<T>& other) const;
    CUDA_HOST MatrixCuda<T> operator-(T scalar) const;
    CUDA_HOST MatrixCuda<T> operator*(T scalar) const;
    CUDA_HOST MatrixCuda<T> operator/(T scalar) const;
    
    // 复合赋值运算符
    CUDA_HOST MatrixCuda<T>& operator+=(const MatrixCuda<T>& other);
    CUDA_HOST MatrixCuda<T>& operator+=(T scalar);
    CUDA_HOST MatrixCuda<T>& operator-=(const MatrixCuda<T>& other);
    CUDA_HOST MatrixCuda<T>& operator-=(T scalar);
    CUDA_HOST MatrixCuda<T>& operator*=(T scalar);
    CUDA_HOST MatrixCuda<T>& operator/=(T scalar);
    
    // 矩阵乘法（使用cuBLAS）
    CUDA_HOST MatrixCuda<T> matmul(const MatrixCuda<T>& other) const;
    CUDA_HOST MatrixCuda<T> operator*(const MatrixCuda<T>& other) const;
    
    // 工具函数
    CUDA_HOST void print(const std::string& name = "") const;
    CUDA_HOST void fill(T value);
    CUDA_HOST void fill_random(T min = 0, T max = 1);
    CUDA_HOST bool is_valid() const;
    
    // 静态工厂方法
    CUDA_HOST static MatrixCuda<T> zeros(long rows, long cols);
    CUDA_HOST static MatrixCuda<T> ones(long rows, long cols);
    CUDA_HOST static MatrixCuda<T> identity(long size);
    CUDA_HOST static MatrixCuda<T> random(long rows, long cols, T min = 0, T max = 1);
    
    // 从现有Matrx类转换
    CUDA_HOST static MatrixCuda<T> from_matrx(const class Matrx<T>& mat);
    CUDA_HOST class Matrx<T> to_matrx() const;
    
private:
    long rows_;
    long cols_;
    std::vector<T> host_data_;  // 主机内存数据
    T* device_data_;            // 设备内存数据
    bool device_allocated_;     // 设备内存是否已分配
    bool host_modified_;        // 主机数据是否已修改
    bool device_modified_;      // 设备数据是否已修改
    
    // 私有辅助函数
    CUDA_HOST void check_index(long row, long col) const;
    CUDA_HOST void initialize_from_vector(const std::vector<T>& data);
    
    // CUDA错误检查
    CUDA_HOST static void check_cuda_error(cudaError_t error, const char* file, int line);
    
    // 计算线程块和网格大小
    CUDA_HOST static dim3 get_block_size(long total_elements);
    CUDA_HOST static dim3 get_grid_size(long total_elements, dim3 block_size);
};

// CUDA错误检查宏
#define CHECK_CUDA_ERROR(err) (MatrixCuda<float>::check_cuda_error(err, __FILE__, __LINE__))

// 显式实例化声明
extern template class MatrixCuda<float>;
extern template class MatrixCuda<double>;

} // namespace qing

// 包含内联实现
#include "MatrixCuda.inl.cuh"