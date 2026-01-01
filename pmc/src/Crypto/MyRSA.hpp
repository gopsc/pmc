#pragma once
#include "Crypto_Basic.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/core_names.h>
// 错误处理宏
#define HANDLE_SSL_ERROR() { \
	char err_buf[512]; \
	ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf)); \
	std::cerr << "SSL error: " << err_buf << std::endl; \
}
// 自定义删除器用于管理OpenSSL资源
struct EVP_PKEY_deleter {
	void operator()(EVP_PKEY* pkey) const { EVP_PKEY_free(pkey); }
};
using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, EVP_PKEY_deleter>;

struct EVP_PKEY_CTX_deleter {
	void operator()(EVP_PKEY_CTX* ctx) const { EVP_PKEY_CTX_free(ctx); }
};
using EVP_PKEY_CTX_ptr = std::unique_ptr<EVP_PKEY_CTX, EVP_PKEY_CTX_deleter>;

struct BIO_deleter {
	void operator()(BIO* bio) const { BIO_free_all(bio); }
};
using BIO_ptr = std::unique_ptr<BIO, BIO_deleter>;

namespace qing {
	class MyRSA: public qing::Crypto_Basic {
		public:
		class Generator {
			public:
			Generator() {
				key = generate_rsa_key();
			}
			void save_pri_key(const std::string filename) {
				if (!save_private_key(key.get(), filename)) {
					throw Crypto_Basic::CryptoExp("保存私钥失败。");
				}
			}
			void save_pub_key(const std::string filename) {
				if(!save_public_key(key.get(), filename)) {
					throw Crypto_Basic::CryptoExp("保存公钥失败。");
				}
			}
			private:
			EVP_PKEY_ptr key;
			// 生成 RSA 密钥对
				
			static EVP_PKEY_ptr generate_rsa_key(size_t key_bits = 2048) {
				EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr));
				if (!ctx) {
					HANDLE_SSL_ERROR();
					return nullptr;
				}
				if (EVP_PKEY_keygen_init(ctx.get()) <= 0) {
					HANDLE_SSL_ERROR();
					return nullptr;
				}
					
				// 设置 RSA 密钥长度
				OSSL_PARAM params[] = {
					OSSL_PARAM_construct_size_t(OSSL_PKEY_PARAM_RSA_BITS, &key_bits),
					OSSL_PARAM_END
				};

				if (EVP_PKEY_CTX_set_params(ctx.get(), params) <= 0) {
					HANDLE_SSL_ERROR();
					return nullptr;
				}

				EVP_PKEY* pkey = nullptr;
				if (EVP_PKEY_generate(ctx.get(), &pkey) <= 0) {
					HANDLE_SSL_ERROR();
					return nullptr;
				}

				return EVP_PKEY_ptr(pkey);
			}

			// 保存私钥到文件
			static bool save_private_key(EVP_PKEY* pkey, const std::string& filename,
					const char* password = nullptr) {
				BIO_ptr bio(BIO_new_file(filename.c_str(), "wb"));
				if (!bio) {
					std::cerr << "Failed to open file: " << filename << std::endl;
					return false;
				}

				// 选择加密算法
				const EVP_CIPHER* cipher = password ? EVP_aes_256_cbc() : nullptr;

				int ret = PEM_write_bio_PrivateKey(bio.get(), pkey, cipher,
						(unsigned char*) password,
						password ? strlen(password) : 0,
						nullptr, nullptr);

				if (ret != 1) {
					HANDLE_SSL_ERROR();
					return false;
				}

				return true;
			}

			// 保存公钥到文件
			static bool save_public_key(EVP_PKEY* pkey, const std::string& filename) {
				BIO_ptr bio(BIO_new_file(filename.c_str(), "wb"));
				if (!bio) {
					std::cerr << "Failed ot open file: " << filename << std::endl;
					return false;
				}

				int ret = PEM_write_bio_PUBKEY(bio.get(), pkey);
				if (ret != 1) {
					HANDLE_SSL_ERROR();
					return false;
				}

				return true;
			}
		};

		class Private_Key: Crypto_Basic {
		public:
			Private_Key(const std::string fullpath) {
				if (!(pri_key = load_private_key(fullpath)))
					throw Crypto_Basic::CryptoExp("读取私钥失败");
			}
			std::string Decrypt(const std::string cipher_hex) {
				std::vector<unsigned char> ciphertext = hex_to_bytes(cipher_hex);
				std::vector<unsigned char> decrypted = rsa_decrypt(pri_key.get(), ciphertext);
				if (decrypted.empty()) throw Crypto_Basic::CryptoExp("解密失败");
				return std::string(decrypted.begin(), decrypted.end());
			}
		private:
			EVP_PKEY_ptr pri_key;
			// 从文件加载私钥
			EVP_PKEY_ptr load_private_key(const std::string& filename,
					const char* password = nullptr) {
				BIO_ptr bio(BIO_new_file(filename.c_str(), "rb"));
				if (!bio) {
					std::cerr << "Failed to open file: " << filename << std::endl;
					return nullptr;
				}

				// 密码回调函数，显示转换为函数指针类型
				pem_password_cb* callback = +[] (char *buf, int size, int rwflag, void * u) {
					if (!u) return 0;
					const char* pass = (const char*)u;
					int len = strlen(pass);
					if (len > size) len = size;
					memcpy(buf, pass, len);
					return len;
				};
				pem_password_cb* cb = password ? nullptr : callback;
	
				EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio.get(), nullptr, cb, (void*)password);
				if (!pkey) {
					HANDLE_SSL_ERROR();
					return nullptr;
				}
	
				return EVP_PKEY_ptr(pkey);
			}

			// 使用私钥解密数据
			static std::vector<unsigned char> rsa_decrypt(EVP_PKEY* private_key,
					const std::vector<unsigned char>& ciphertext) {
				EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new(private_key, nullptr));
				if (!ctx) {
					HANDLE_SSL_ERROR();
					return {};
				}

				if (EVP_PKEY_decrypt_init(ctx.get()) <= 0) {
					HANDLE_SSL_ERROR();
					return {};
				}

				// 使用 OAEP 填充
				if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING) <= 0) {
					HANDLE_SSL_ERROR();
					return {};
				}

				// 获取输出缓冲区大小
				size_t outlen = 0;
				if (EVP_PKEY_decrypt(ctx.get(), nullptr, &outlen,
							ciphertext.data(), ciphertext.size()) <= 0) {
					HANDLE_SSL_ERROR();
					return {};
				}

				// 执行解密
				std::vector<unsigned char> plaintext(outlen);
				if (EVP_PKEY_decrypt(ctx.get(), plaintext.data(), &outlen,
							ciphertext.data(), ciphertext.size()) <= 0) {
					HANDLE_SSL_ERROR();
					return {};
				}

				plaintext.resize(outlen);
				return plaintext;
			}
		};


		class Public_Key: public Crypto_Basic {
		public:
			Public_Key(const std::string filename) {
				if (!(pub_key = load_public_key(filename)))
					throw Crypto_Basic::CryptoExp("读取公钥失败");
			}
			std::string Encrypt(std::string original_text) {
				std::vector<unsigned char> plaintext(original_text.begin(), original_text.end());
				std::vector<unsigned char> ciphertext = rsa_encrypt(pub_key.get(), plaintext);
				if (ciphertext.empty()) throw Crypto_Basic::CryptoExp("加密失败");
				return bytes_to_hex(ciphertext);
			}
		private:
			EVP_PKEY_ptr pub_key;

			// 从文件加载公钥
			static EVP_PKEY_ptr load_public_key(const std::string& filename) {
				BIO_ptr bio(BIO_new_file(filename.c_str(), "rb"));
				if (!bio)  {
					std::cerr << "Failed to open file: " << filename << std::endl;
					return nullptr;
				}

				EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);
				if (!pkey) {
					HANDLE_SSL_ERROR();
					return nullptr;
				}

				return EVP_PKEY_ptr(pkey);
			}

			// 使用公钥加密数据
			static std::vector<unsigned char> rsa_encrypt(EVP_PKEY* public_key,
					const std::vector<unsigned char>& plaintext) {
				EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new(public_key, nullptr));
				if (!ctx) {
					HANDLE_SSL_ERROR();
					return {};
				}

				if (EVP_PKEY_encrypt_init(ctx.get()) <= 0) {
					HANDLE_SSL_ERROR();
					return {};
				}

				// 使用 OAEP 填充
				if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING) <= 0) {
					HANDLE_SSL_ERROR();
					return {};
				}

				// 获取输出缓冲区大小
				size_t outlen =0 ;
				if (EVP_PKEY_encrypt(ctx.get(), nullptr, &outlen,
							plaintext.data(), plaintext.size()) <= 0) {
					HANDLE_SSL_ERROR();
					return {};
				}

				// 执行加密
				std::vector<unsigned char> ciphertext(outlen);
				if (EVP_PKEY_encrypt(ctx.get(), ciphertext.data(), &outlen,
							plaintext.data(), plaintext.size()) <= 0) {
					HANDLE_SSL_ERROR();
					return {};
				}

				ciphertext.resize(outlen);
				return ciphertext;
			}
		};
	};
}
