#include "nn/Matrix.hh"
namespace qing {

	template <typename T>
	Matrx<T>::Matrx(): row(0), col(0) {}

	template <typename T>
	Matrx<T>::Matrx(const long row, const long col)
	: row(row), col(col) {  /* 该构造函数用于从头开始构造 */
		arr = std::vector<T>(col * row);
	}

	template<typename T>
	Matrx<T>::Matrx(const long row, const long col, const std::vector<T>& src)
	: row(row), col(col) {  /* 该构造函数用于通过已有的vector进行构造 */
		arr = src;
	}

	template<typename T>
	const T& Matrx<T>::get(long row, long col) const {  /* 访问器 - 访问矩阵元素 */
		return arr[col * this->row + row];
	}

	template<typename T>
	T& Matrx<T>::witch(long row, long col) {  /* 访问器 - 访问矩阵元素 */
		return arr[col * this->row + row];
	}

	template<typename T>
	const std::vector<T>& Matrx<T>::get_data() const{  /* 访问器 - 用于获取原始数据 */
		return arr;
	}

	template<typename T>
	const long Matrx<T>::get_col() const {  /* 获取列数 */
		return col;
	}

	template<typename T>
	const long Matrx<T>::get_row() const {  /* 获取行数 */
		return row;
	}

	template<typename T>
	bool Matrx<T>::has_same_shape(const Matrx<T>& other) {  /* 两个矩阵是否具有相同形状 */
		return other.get_col() == this->get_col() || other.get_row() == this->get_row();
	}

	template<typename T>
	bool Matrx<T>::has_dot_shape(const Matrx<T>& other) { /* 具有矩阵乘法的形状 */
		return this->get_col() == other.get_row();
	}

	template<typename T>
	void Matrx<T>::assert_same_shape(const Matrx<T>& other) { /* 断言两个矩阵具有相同形状 */
		if (!has_same_shape(other))
			throw std::invalid_argument("The shapes of matrix are not same.");   
	}

	template<typename T>
	void Matrx<T>::assert_dot_shape(const Matrx<T>& other) { /* 断言两个矩阵具有点积前置形状 */
		if (!has_dot_shape(other)) {
			throw std::invalid_argument("The shapes of matrix can not be dot.");   
		}
	}

	template<typename T>
	Matrx<T> Matrx<T>::operator+(const Matrx<T>& other) {  /* 加法操作符重定义 - 矩阵加法 */
		assert_same_shape(other);  /* 检查两个矩阵形状是否一致 */
		std::vector<T> v0  = this->get_data();
		std::vector<T> vec = other.get_data();
		for (auto it = v0.begin(), it_or = vec.begin(); it != v0.end(); ++it, ++it_or ) {
			*it += *it_or;
		}
		return Matrx<T>{this->get_row(), this->get_col(), v0};
	}

	template<typename T>
	Matrx<T> Matrx<T>::operator+(T& item) {  /* 矩阵加法广播 */
		auto matrix = *this;
		for (auto it = arr.begin(); it != arr.end(); ++it) {
			*it += item;
		}
		return matrix;
	}

	template<typename T>
	Matrx<T> Matrx<T>::operator-(const Matrx<T>& other) {  /* 操作符重载 - 矩阵减法 */
		assert_same_shape(other);  /* 检查两个矩阵形状是否一致 */
		std::vector<T> v0 = this->get_data();
		std::vector <T> vec = other.get_data();
		for (auto it = v0.begin(), it_or = vec.begin(); it != v0.end(); ++it, ++it_or ) {
			*it -= *it_or;
		}
		return Matrx{this->get_row(), this->get_col(), v0};
	}

	template<typename T>
	Matrx<T> Matrx<T>::operator-(T& item) {  /* 操作符重载 - 矩阵减法广播 */
		auto matrix = *this;
		for (auto it = arr.begin(); it != arr.end(); ++it) {
			*it -= item;
		}
		return matrix;
	}

	template<typename T>
	const Matrx<T> Matrx<T>::operator*(const Matrx<T>& other) {  /* 操作符重载 - 矩阵乘法 */ 
		assert_dot_shape(other); /* 它们的形状是否能够点积 */
		auto matrix = Matrx<T>(this->get_row(), other.get_col());
		for (long i=0; i<this->get_row(); ++i) {/* 点积运算 */
 			for (long j=0; j<other.get_col(); ++j) {
				T sum{0};
				for (long k=0; k<this->get_col(); ++k) {
					sum += this->get(i, k) * other.get(k, j);
				}
				matrix.witch(i, j) = sum;
			}
		}
		return matrix;
	}

	template<typename T>
	Matrx<T> Matrx<T>::operator*(T& item) {  /* 操作符重载 - 矩阵乘法广播 */
		std::vector<T> vec  = this->get_data();
		for (auto it = vec.begin(); it != vec.end(); ++it) {
			*it *= item;
		}
		return Matrx<T>{this->get_row(), this->get_col(), vec};
	}

	template<typename T>
	Matrx<T> Matrx<T>::operator/(T& item) {  /* 操作符重载 - 矩阵除法广播 */
		std::vector<T> vec = this->get_data();
		for (auto it = vec.begin(); it != vec.end(); ++it) {
			*it /= item;
		}
		return Matrx<T>(this->get_row(), this->get_col(), vec);
	}

	template class Matrx<float>; /* 显式实例化 */
	template class Matrx<double>;

}
