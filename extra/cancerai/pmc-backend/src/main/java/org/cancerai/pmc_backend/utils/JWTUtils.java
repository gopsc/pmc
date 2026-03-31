package org.cancerai.pmc_backend.utils;

import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.SignatureAlgorithm;
import io.jsonwebtoken.security.Keys;

import java.nio.charset.StandardCharsets;
import java.security.Key;
import java.util.Base64;
import java.util.Date;

public class JWTUtils {
    final private static String key = "cancerai_key_qpalzm102938";
    final private static String encodedKey = Base64.getEncoder()
            .encodeToString(key.getBytes(StandardCharsets.UTF_8));
    //    set JWT kwy
    final static Key jwtKey = Keys.hmacShaKeyFor(encodedKey.getBytes());

    public static String genJWT(String name, String password) {
        final Date expirationTime = new Date(
                System.currentTimeMillis()
                        + (60 * 1000 // 1min
                        * 60 // 1h
                        * 24 // 1day
                        * 2 // 2 days
                ));

        return Jwts.builder()
                .claim("userName", name)
                .claim("user-password", password)
                .signWith(jwtKey, SignatureAlgorithm.HS256)
                .setExpiration(expirationTime)
                .compact();
    }

    public static void parseJWT(String jwt) {
        Jwts.parserBuilder()
                .setSigningKey(jwtKey)
                .build()
                .parseClaimsJws(jwt);
    }
}
