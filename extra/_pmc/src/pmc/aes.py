from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding
class MyAES:
    def __init__(self, key: str, iv: str):
        self.key: bytes = bytes.fromhex(key)
        self.iv: bytes = bytes.fromhex(iv)
    def decrypt(self, ciphertext: str):
        cipher: bytes = bytes.fromhex(ciphertext)
        plain: bytes = MyAES.aes_decrypt(cipher, self.key, self.iv)
        try:
            return plain.decode('utf-8')
        except UnicodeDecodeError as e:
            # 返回原始字节的十六进制表示作为备选
            return plain.hex()
    def encrypt(self, original_text: str):
        plaintext: byte = original_text.encode('utf-8')
        ciphertext: byte = MyAES.aes_encrypt(plaintext, self.key, self.iv)
        return ciphertext.hex()
    @staticmethod
    def aes_decrypt(ciphertext: bytes, key: bytes, iv: bytes) -> bytes:
        """
        使用AES-256-CBC算法解密数据

        参数
        - ciphertext: 待解密的密文字节串
        - key: 256位（32字节）的密钥字节串
        - iv: 初始化向量字节串（必须为16字节）

        返回：
        - 解密后的明文字节串
        """

        # 验证输入参数长度
        if len(key) != 32:
            raise ValueError('AES-256密钥必须是32字节长度')
        if len(iv) != 16:
            raise ValueError('AES-CBC的IV必须是16字节长度')

        # 创建AES-CBC解密器
        cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
        decryptor = cipher.decryptor()

        try:
            # 执行解密并移除PKCS#7填充
            plaintext = decryptor.update(ciphertext) + decryptor.finalize()
            return plaintext
        except Exception as e:
            print(f"解密失败：{e}")
            return b''

    @staticmethod
    def aes_encrypt(plaintext: bytes, key: bytes, iv: bytes) -> bytes:
        """
        AES-256-CBC 加密

        参数
            plaintext: 要加密的密文
            key: 32字节的密钥
            iv: 16字节的初始化向量
        返回：
            加密后的密文
        """
        # 检查密钥和IV的长度
        if len(key) != 32:
            raise ValueError("密钥必须是32字节（AES-256）")
        if len(iv) != 16:
            raise ValueError("IV必须是16字节")

        # 创建加密器
        cipher = Cipher(
            algorithms.AES(key),
            modes.CBC(iv),
            backend=default_backend()
        )
        encryptor = cipher.encryptor()

        # 添加PKCS7填充
        padder = padding.PKCS7(128).padder()
        padded_data = padder.update(plaintext) + padder.finalize()

        # 加密数据
        ciphertext = encryptor.update(padded_data) + encryptor.finalize()

        return ciphertext
