package org.cancerai.pmc_backend.utils;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class EncryptUtils {
    public static String SHA256Encrypt(String str) {
        try {
            // 获取SHA-256算法实例
            MessageDigest digest = MessageDigest.getInstance("SHA-256");

            // 计算哈希值（指定UTF-8编码避免平台差异）
            byte[] hashBytes = digest.digest(str.getBytes(StandardCharsets.UTF_8));

            // 将字节数组转换为十六进制字符串
            StringBuilder hexString = new StringBuilder();
            for (byte b : hashBytes) {
                // 将每个字节转换为两位十六进制数，不足两位补0
                String hex = String.format("%02x", 0xFF & b);
                hexString.append(hex);
            }
            return hexString.toString();

        } catch (NoSuchAlgorithmException e) {
            // SHA-256是Java标准算法，理论上不会抛出此异常
            throw new RuntimeException("SHA-256算法不支持", e);
        }
    }
}
