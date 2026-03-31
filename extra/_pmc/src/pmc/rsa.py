from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.backends import default_backend
import os
class Rsa_Pri_Key:
    '''RSA 私钥对象'''
    def __init__(self, fullpath: str) -> None:
        self.pri_key=Rsa_Pri_Key.load_private_key(fullpath)
    def decrypt(self, ciphertext: str) -> str:
        return Rsa_Pri_Key.decrypt_hex_ciphertext(self.pri_key, ciphertext)


    @staticmethod
    def load_private_key(pem_file, password=None):
        '''
        从PEM文件加载RSA私钥

        :param pem_file: PEM格式的私钥文件路径
        :param password: 私钥密码（如果有）
        :return RSA私钥对象
        '''
        with open(pem_file, 'rb') as key_file:
            private_key = serialization.load_pem_private_key(
                key_file.read(),
                password=password.encode() if password else None,
                backend=default_backend()
            )
        return private_key

    @staticmethod
    def decrypt_hex_ciphertext(private_key, hex_ciphertext):
        '''
        解密HEX格式的RSA密文

        :param private_key: 已加载的RSA私钥
        :param hex_ciphertext: Hex字符串格式的密文
        :return: 解密后的明文
        '''
        try:
            # 将Hex字符串转换为bytes
            ciphertext = bytes.fromhex(hex_ciphertext)

            # 使用OAEP填充方案解密
            plaintext = private_key.decrypt(
                ciphertext,
                padding.OAEP(
                    # OPENSSL 没有显式指定时，这个是SHA1
                    mgf=padding.MGF1(algorithm=hashes.SHA1()),
                    algorithm=hashes.SHA1(),
                    label=None
                )
            )
            return plaintext
        except ValueError as e:
            print(f"Hex解码失败：{str(e)}")
            return None
        except Exception as e:
            print(f"解密失败：{str(e)}")
            return None


def main():
    ...

if __name__ == '__main__':
    main()
"""
# 生成RSA私钥
private_key = rsa.generate_private_key(
    public_exponent=65537,  # 通常使用65537作为公钥指数
    key_size=2048,          # 密钥长度，推荐至少2048位
)

# 将私钥序列化为PEM格式
pem_private_key = private_key.private_bytes(
    encoding=serialization.Encoding.PEM,
    format=serialization.PrivateFormat.PKCS8,
    encryption_algorithm=serialization.NoEncryption()  # 这里不加密私钥
)

# 获取对应的公钥
public_key = private_key.public_key()

# 将公钥序列化为PEM格式
pem_public_key = public_key.public_bytes(
    encoding=serialization.Encoding.PEM,
    format=serialization.PublicFormat.SubjectPublicKeyInfo
)

# 打印密钥
print("私钥:")
print(pem_private_key.decode('utf-8'))

print("公钥:")
print(pem_public_key.decode('utf-8'))

# 可选：将密钥保存到文件
with open('private_key.pem', 'wb') as f:
    f.write(pem_private_key)

with open('public_key.pem', 'wb') as f:
    f.write(pem_public_key)
"""
