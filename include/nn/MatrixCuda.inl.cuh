/*
 * MatrixCuda.inl.cuh - MatrixCuda类的内联实现
 * 包含CUDA内核函数和模板成员函数的实现
 */

#ifndef MATRIX_CUDA_INL_CUH
#define MATRIX_CUDA_INL_CUH

#include "MatrixCuda.cuh"
#include <random>
#include <chrono>
#include <algorithm>

namespace qing {

// ============================================================================
// CUDA内核函数实现
// ============================================================================

namespace cuda_kernels {

// 矩阵加法内核
template <typename T>
__global__ void matrix_add_kernel(const T* A, const T* B, T* C, 
                                  long rows, long cols, long total_elements) {
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    long stride = blockDim.x * gridDim.x;
    
    for (long i = idx; i < total_elements; i += stride) {
        C[i] = A[i] + B[i];
    }
}

// 矩阵减法内核
template <typename T>
__global__ void matrix_sub_kernel(const T* A, const T* B, T* C,
                                  long rows, long cols, long total_elements) {
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    long stride = blockDim.x * gridDim.x;
    
    for (long i = idx; i < total_elements; i += stride) {
        C[i] = A[i] - B[i];
    }
}

// 矩阵标量乘法内核
template <typename T>
__global__ void matrix_scalar_mul_kernel(const T* A, T scalar, T* C,
                                         long total_elements) {
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    long stride = blockDim.x * gridDim.x;
    
    for (long i = idx; i < total_elements; i += stride) {
        C[i] = A[i] * scalar;
    }
}

// 矩阵标量加法内核
template <typename T>
__global__ void matrix_scalar_add_kernel(const T* A, T scalar, T* C,
                                         long total_elements) {
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    long stride = blockDim.x * gridDim.x;
    
    for (long i = idx; i < total_elements; i += stride) {
        C[i] = A[i] + scalar;
    }
}

// 矩阵转置内核
template <typename T>
__global__ void matrix_transpose_kernel(const T* A, T* C,
                                        long rows, long cols) {
    long row = blockIdx.y * blockDim.y + threadIdx.y;
    long col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row < rows && col < cols) {
        // A[row, col] -> C[col, row]
        C[col * rows + row] = A[row * cols + col];
    }
}

// 矩阵元素对应乘积内核
template <typename T>
__global__ void matrix_hadamard_kernel(const T* A, const T* B, T* C,
                                       long total_elements) {
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    long stride = blockDim.x * gridDim.x;
    
    for (long i = idx; i < total_elements; i += stride) {
        C[i] = A[i] * B[i];
    }
}

// 矩阵复制内核
template <typename T>
__global__ void matrix_copy_kernel(const T* src, T* dst, long total_elements) {
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    long stride = blockDim.x * gridDim.x;
    
    for (long i = idx; i < total_elements; i += stride) {
        dst[i] = src[i];
    }
}

} // namespace cuda_kernels

// ============================================================================
// MatrixCuda类成员函数实现
// ============================================================================

// 构造函数和析构函数
template <typename T>
CUDA_HOST MatrixCuda<T>::MatrixCuda() 
    : rows_(0), cols_(0), device_data_(nullptr), 
      device_allocated_(false), host_modified_(false), device_modified_(false) {}

template <typename T>
CUDA_HOST MatrixCuda<T>::MatrixCuda(long rows, long cols)
    : rows_(rows), cols_(cols), device_data_(nullptr),
      device_allocated_(false), host_modified_(false), device_modified_(false) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Matrix dimensions must be positive");
    }
    host_data_.resize(rows * cols, T(0));
}

template <typename T>
CUDA_HOST MatrixCuda<T>::MatrixCuda(long rows, long cols, const std::vector<T>& data)
    : rows_(rows), cols_(cols), device_data_(nullptr),
      device_allocated_(false), host_modified_(false), device_modified_(false) {
    initialize_from_vector(data);
}

template <typename T>
CUDA_HOST MatrixCuda<T>::MatrixCuda(const MatrixCuda<T>& other)
    : rows_(other.rows_), cols_(other.cols_), 
      host_data_(other.host_data_), device_data_(nullptr),
      device_allocated_(false), host_modified_(other.host_modified_), 
      device_modified_(false) {
    if (other.device_allocated_) {
        allocate_device_memory();
        copy_to_device();
    }
}

template <typename T>
CUDA_HOST MatrixCuda<T>::MatrixCuda(MatrixCuda<T>&& other) noexcept
    : rows_(other.rows_), cols_(other.cols_),
      host_data_(std::move(other.host_data_)),
      device_data_(other.device_data_),
      device_allocated_(other.device_allocated_),
      host_modified_(other.host_modified_),
      device_modified_(other.device_modified_) {
    other.rows_ = 0;
    other.cols_ = 0;
    other.device_data_ = nullptr;
    other.device_allocated_ = false;
    other.host_modified_ = false;
    other.device_modified_ = false;
}

template <typename T>
CUDA_HOST MatrixCuda<T>::~MatrixCuda() {
    free_device_memory();
}

// 赋值运算符
template <typename T>
CUDA_HOST MatrixCuda<T>& MatrixCuda<T>::operator=(const MatrixCuda<T>& other) {
    if (this != &other) {
        free_device_memory();
        
        rows_ = other.rows_;
        cols_ = other.cols_;
        host_data_ = other.host_data_;
        host_modified_ = other.host_modified_;
        device_modified_ = false;
        
        if (other.device_allocated_) {
            allocate_device_memory();
            copy_to_device();
        }
    }
    return *this;
}

template <typename T>
CUDA_HOST MatrixCuda<T>& MatrixCuda<T>::operator=(MatrixCuda<T>&& other) noexcept {
    if (this != &other) {
        free_device_memory();
        
        rows_ = other.rows_;
        cols_ = other.cols_;
        host_data_ = std::move(other.host_data_);
        device_data_ = other.device_data_;
        device_allocated_ = other.device_allocated_;
        host_modified_ = other.host_modified_;
        device_modified_ = other.device_modified_;
        
        other.rows_ = 0;
        other.cols_ = 0;
        other.device_data_ = nullptr;
        other.device_allocated_ = false;
        other.host_modified_ = false;
        other.device_modified_ = false;
    }
    return *this;
}

// 内存管理函数
template <typename T>
CUDA_HOST void MatrixCuda<T>::allocate_device_memory() {
    if (device_allocated_ || get_total_elements() == 0) {
        return;
    }
    
    cudaError_t err = cudaMalloc(&device_data_, get_total_elements() * sizeof(T));
    CHECK_CUDA_ERROR(err);
    device_allocated_ = true;
    device_modified_ = false;
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::free_device_memory() {
    if (device_allocated_ && device_data_ != nullptr) {
        cudaError_t err = cudaFree(device_data_);
        CHECK_CUDA_ERROR(err);
        device_data_ = nullptr;
        device_allocated_ = false;
        device_modified_ = false;
    }
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::copy_to_device() {
    if (!device_allocated_) {
        allocate_device_memory();
    }
    
    if (host_modified_ && get_total_elements() > 0) {
        cudaError_t err = cudaMemcpy(device_data_, host_data_.data(), 
                                     get_total_elements() * sizeof(T),
                                     cudaMemcpyHostToDevice);
        CHECK_CUDA_ERROR(err);
        host_modified_ = false;
        device_modified_ = true;
    }
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::copy_from_device() {
    if (device_allocated_ && device_modified_ && get_total_elements() > 0) {
        cudaError_t err = cudaMemcpy(host_data_.data(), device_data_,
                                     get_total_elements() * sizeof(T),
                                     cudaMemcpyDeviceToHost);
        CHECK_CUDA_ERROR(err);
        device_modified_ = false;
        host_modified_ = true;
    }
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::sync_device_to_host() {
    copy_from_device();
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::sync_host_to_device() {
    copy_to_device();
}

// 访问器函数
template <typename T>
CUDA_HOST T MatrixCuda<T>::get_element(long row, long col) const {
    check_index(row, col);
    return host_data_[row * cols_ + col];
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::set_element(long row, long col, T value) {
    check_index(row, col);
    host_data_[row * cols_ + col] = value;
    host_modified_ = true;
}

// 形状检查函数
template <typename T>
CUDA_HOST bool MatrixCuda<T>::has_same_shape(const MatrixCuda<T>& other) const {
    return rows_ == other.rows_ && cols_ == other.cols_;
}

template <typename T>
CUDA_HOST bool MatrixCuda<T>::has_dot_shape(const MatrixCuda<T>& other) const {
    return cols_ == other.rows_;
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::assert_same_shape(const MatrixCuda<T>& other) const {
    if (!has_same_shape(other)) {
        throw std::invalid_argument("Matrices must have the same shape");
    }
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::assert_dot_shape(const MatrixCuda<T>& other) const {
    if (!has_dot_shape(other)) {
        throw std::invalid_argument("Matrices cannot be multiplied: cols of first != rows of second");
    }
}

// 矩阵运算函数
template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::add(const MatrixCuda<T>& other) const {
    assert_same_shape(other);
    
    MatrixCuda<T> result(rows_, cols_);
    
    // 确保数据在设备上
    sync_host_to_device();
    other.sync_host_to_device();
    result.allocate_device_memory();
    
    // 设置CUDA内核参数
    long total_elements = get_total_elements();
    dim3 block_size = get_block_size(total_elements);
    dim3 grid_size = get_grid_size(total_elements, block_size);
    
    // 启动内核
    cuda_kernels::matrix_add_kernel<T><<<grid_size, block_size>>>(
        device_data_, other.device_data_, result.device_data_,
        rows_, cols_, total_elements
    );
    
    CHECK_CUDA_ERROR(cudaGetLastError());
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    result.device_modified_ = true;
    result.sync_device_to_host();
    
    return result;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::subtract(const MatrixCuda<T>& other) const {
    assert_same_shape(other);
    
    MatrixCuda<T> result(rows_, cols_);
    
    // 确保数据在设备上
    sync_host_to_device();
    other.sync_host_to_device();
    result.allocate_device_memory();
    
    // 设置CUDA内核参数
    long total_elements = get_total_elements();
    dim3 block_size = get_block_size(total_elements);
    dim3 grid_size = get_grid_size(total_elements, block_size);
    
    // 启动内核
    cuda_kernels::matrix_sub_kernel<T><<<grid_size, block_size>>>(
        device_data_, other.device_data_, result.device_data_,
        rows_, cols_, total_elements
    );
    
    CHECK_CUDA_ERROR(cudaGetLastError());
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    result.device_modified_ = true;
    result.sync_device_to_host();
    
    return result;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::multiply_scalar(T scalar) const {
    MatrixCuda<T> result(rows_, cols_);
    
    // 确保数据在设备上
    sync_host_to_device();
    result.allocate_device_memory();
    
    // 设置CUDA内核参数
    long total_elements = get_total_elements();
    dim3 block_size = get_block_size(total_elements);
    dim3 grid_size = get_grid_size(total_elements, block_size);
    
    // 启动内核
    cuda_kernels::matrix_scalar_mul_kernel<T><<<grid_size, block_size>>>(
        device_data_, scalar, result.device_data_, total_elements
    );
    
    CHECK_CUDA_ERROR(cudaGetLastError());
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    result.device_modified_ = true;
    result.sync_device_to_host();
    
    return result;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::add_scalar(T scalar) const {
    MatrixCuda<T> result(rows_, cols_);
    
    // 确保数据在设备上
    sync_host_to_device();
    result.allocate_device_memory();
    
    // 设置CUDA内核参数
    long total_elements = get_total_elements();
    dim3 block_size = get_block_size(total_elements);
    dim3 grid_size = get_grid_size(total_elements, block_size);
    
    // 启动内核
    cuda_kernels::matrix_scalar_add_kernel<T><<<grid_size, block_size>>>(
        device_data_, scalar, result.device_data_, total_elements
    );
    
    CHECK_CUDA_ERROR(cudaGetLastError());
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    result.device_modified_ = true;
    result.sync_device_to_host();
    
    return result;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::transpose() const {
    MatrixCuda<T> result(cols_, rows_);
    
    // 确保数据在设备上
    sync_host_to_device();
    result.allocate_device_memory();
    
    // 设置CUDA内核参数（2D网格用于转置）
    dim3 block_size(16, 16);
    dim3 grid_size((cols_ + block_size.x - 1) / block_size.x,
                   (rows_ + block_size.y - 1) / block_size.y);
    
    // 启动转置内核
    cuda_kernels::matrix_transpose_kernel<T><<<grid_size, block_size>>>(
        device_data_, result.device_data_, rows_, cols_
    );
    
    CHECK_CUDA_ERROR(cudaGetLastError());
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    result.device_modified_ = true;
    result.sync_device_to_host();
    
    return result;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::hadamard_product(const MatrixCuda<T>& other) const {
    assert_same_shape(other);
    
    MatrixCuda<T> result(rows_, cols_);
    
    // 确保数据在设备上
    sync_host_to_device();
    other.sync_host_to_device();
    result.allocate_device_memory();
    
    // 设置CUDA内核参数
    long total_elements = get_total_elements();
    dim3 block_size = get_block_size(total_elements);
    dim3 grid_size = get_grid_size(total_elements, block_size);
    
    // 启动内核
    cuda_kernels::matrix_hadamard_kernel<T><<<grid_size, block_size>>>(
        device_data_, other.device_data_, result.device_data_, total_elements
    );
    
    CHECK_CUDA_ERROR(cudaGetLastError());
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    result.device_modified_ = true;
    result.sync_device_to_host();
    
    return result;
}

// 运算符重载
template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::operator+(const MatrixCuda<T>& other) const {
    return add(other);
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::operator+(T scalar) const {
    return add_scalar(scalar);
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::operator-(const MatrixCuda<T>& other) const {
    return subtract(other);
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::operator-(T scalar) const {
    return add_scalar(-scalar);
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::operator*(T scalar) const {
    return multiply_scalar(scalar);
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::operator/(T scalar) const {
    if (scalar == T(0)) {
        throw std::invalid_argument("Division by zero");
    }
    return multiply_scalar(T(1) / scalar);
}

// 复合赋值运算符
template <typename T>
CUDA_HOST MatrixCuda<T>& MatrixCuda<T>::operator+=(const MatrixCuda<T>& other) {
    *this = *this + other;
    return *this;
}

template <typename T>
CUDA_HOST MatrixCuda<T>& MatrixCuda<T>::operator+=(T scalar) {
    *this = *this + scalar;
    return *this;
}

template <typename T>
CUDA_HOST MatrixCuda<T>& MatrixCuda<T>::operator-=(const MatrixCuda<T>& other) {
    *this = *this - other;
    return *this;
}

template <typename T>
CUDA_HOST MatrixCuda<T>& MatrixCuda<T>::operator-=(T scalar) {
    *this = *this - scalar;
    return *this;
}

template <typename T>
CUDA_HOST MatrixCuda<T>& MatrixCuda<T>::operator*=(T scalar) {
    *this = *this * scalar;
    return *this;
}

template <typename T>
CUDA_HOST MatrixCuda<T>& MatrixCuda<T>::operator/=(T scalar) {
    *this = *this / scalar;
    return *this;
}

// 矩阵乘法（基础实现，使用朴素算法）
template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::matmul(const MatrixCuda<T>& other) const {
    assert_dot_shape(other);
    
    MatrixCuda<T> result(rows_, other.cols_);
    
    // 同步数据到主机（使用朴素算法在主机上计算）
    sync_device_to_host();
    other.sync_device_to_host();
    
    // 朴素矩阵乘法（O(n³)）
    for (long i = 0; i < rows_; ++i) {
        for (long j = 0; j < other.cols_; ++j) {
            T sum = T(0);
            for (long k = 0; k < cols_; ++k) {
                sum += get_element(i, k) * other.get_element(k, j);
            }
            result.set_element(i, j, sum);
        }
    }
    
    // 标记结果已修改
    result.host_modified_ = true;
    result.sync_host_to_device();
    
    return result;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::operator*(const MatrixCuda<T>& other) const {
    return matmul(other);
}

// 工具函数
template <typename T>
CUDA_HOST void MatrixCuda<T>::print(const std::string& name) const {
    sync_device_to_host();
    
    if (!name.empty()) {
        std::cout << name << " (" << rows_ << "x" << cols_ << "):" << std::endl;
    } else {
        std::cout << "Matrix (" << rows_ << "x" << cols_ << "):" << std::endl;
    }
    
    for (long i = 0; i < rows_; ++i) {
        std::cout << "  [";
        for (long j = 0; j < cols_; ++j) {
            std::cout << get_element(i, j);
            if (j < cols_ - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::fill(T value) {
    std::fill(host_data_.begin(), host_data_.end(), value);
    host_modified_ = true;
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::fill_random(T min, T max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dist(min, max);
    
    for (auto& val : host_data_) {
        val = dist(gen);
    }
    host_modified_ = true;
}

template <typename T>
CUDA_HOST bool MatrixCuda<T>::is_valid() const {
    return rows_ > 0 && cols_ > 0 && !host_data_.empty();
}

// 静态工厂方法
template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::zeros(long rows, long cols) {
    MatrixCuda<T> mat(rows, cols);
    mat.fill(T(0));
    return mat;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::ones(long rows, long cols) {
    MatrixCuda<T> mat(rows, cols);
    mat.fill(T(1));
    return mat;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::identity(long size) {
    MatrixCuda<T> mat(size, size);
    mat.fill(T(0));
    for (long i = 0; i < size; ++i) {
        mat.set_element(i, i, T(1));
    }
    return mat;
}

template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::random(long rows, long cols, T min, T max) {
    MatrixCuda<T> mat(rows, cols);
    mat.fill_random(min, max);
    return mat;
}

// 从现有Matrx类转换
template <typename T>
CUDA_HOST MatrixCuda<T> MatrixCuda<T>::from_matrx(const class Matrx<T>& mat) {
    // 注意：这里假设Matrx类有get_row(), get_col(), get_data()方法
    long rows = mat.get_row();
    long cols = mat.get_col();
    const std::vector<T>& data = mat.get_data();
    
    return MatrixCuda<T>(rows, cols, data);
}

template <typename T>
CUDA_HOST class Matrx<T> MatrixCuda<T>::to_matrx() const {
    sync_device_to_host();
    
    // 注意：这里假设Matrx类有相应的构造函数
    // 实际实现可能需要根据Matrx类的具体接口调整
    return class Matrx<T>(rows_, cols_, host_data_);
}

// 私有辅助函数
template <typename T>
CUDA_HOST void MatrixCuda<T>::check_index(long row, long col) const {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
        throw std::out_of_range("Matrix index out of range");
    }
}

template <typename T>
CUDA_HOST void MatrixCuda<T>::initialize_from_vector(const std::vector<T>& data) {
    if (data.size() != static_cast<size_t>(rows_ * cols_)) {
        throw std::invalid_argument("Data size does not match matrix dimensions");
    }
    host_data_ = data;
    host_modified_ = true;
}

// CUDA错误检查
template <typename T>
CUDA_HOST void MatrixCuda<T>::check_cuda_error(cudaError_t error, const char* file, int line) {
    if (error != cudaSuccess) {
        std::cerr << "CUDA error at " << file << ":" << line << ": "
                  << cudaGetErrorString(error) << std::endl;
        throw std::runtime_error("CUDA error occurred");
    }
}

// 计算线程块和网格大小
template <typename T>
CUDA_HOST dim3 MatrixCuda<T>::get_block_size(long total_elements) {
    // 使用256个线程的块（CUDA的典型值）
    const long max_threads_per_block = 256;
    long threads = std::min(total_elements, max_threads_per_block);
    return dim3(threads, 1, 1);
}

template <typename T>
CUDA_HOST dim3 MatrixCuda<T>::get_grid_size(long total_elements, dim3 block_size) {
    // 计算需要的块数
    long blocks = (total_elements + block_size.x - 1) / block_size.x;
    // 限制最大块数（避免过多块）
    const long max_blocks = 65535;
    blocks = std::min(blocks, max_blocks);
    return dim3(blocks, 1, 1);
}

// 显式实例化
template class MatrixCuda<float>;
template class MatrixCuda<double>;

} // namespace qing

#endif // MATRIX_CUDA_INL_CUH