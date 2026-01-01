#pragma once
#include <string>
#include <vector>
#include <stdexcept>
namespace qing {
	class Crypto_Basic {
		public:
		class CryptoExp: public std::runtime_error {
			public:
			CryptoExp(const std::string& msg): std::runtime_error(msg) {
				;
			}
		};
		protected:
		/* 字节码转十六进制字符串 */
		static std::string bytes_to_hex(const std::vector<unsigned char>& data) {
			static const char hex_chars[] = "0123456789abcdef";
			std::string hex_string;
			hex_string.reserve(data.size() * 2);
	
			for (unsigned char byte: data) {
				hex_string.push_back(hex_chars[byte >> 4]);	// 高4位
				hex_string.push_back(hex_chars[byte & 0x0F]);	// 低4位
			}

			return hex_string;
		}

		/* 十六进制字符串转字节码 */
		static std::vector<unsigned char> hex_to_bytes(const std::string &hex) {
			// 检查长度是否为偶数
			if (hex.size() % 2 != 0) {
				throw std::invalid_argument("十六进制字符串长度必须为偶数");
			}
	
			std::vector<unsigned char> bytes;
			bytes.reserve(hex.size() / 2);

			auto char_to_nibble = [](char c) -> unsigned char {
				if (c >= '0' && c <= '9') return c - '0';
				if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
				if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
				throw std::invalid_argument("无效的十六进制字符：" + std::string(1, c));
			};

			for (size_t i = 0; i < hex.size(); i += 2) {
				unsigned char high = char_to_nibble(hex[i]);
				unsigned char low = char_to_nibble(hex[i + 1]);
				bytes.push_back((high << 4) | low);
			}

			return bytes;
		}
	};
}
