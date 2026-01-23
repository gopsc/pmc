// JWT令牌助手函数 - 用于在其他页面中使用存储在cookie中的JWT令牌
class JwtHelper {
    // 获取JWT令牌值
    static getToken() {
        const cookies = document.cookie.split(';');
        for (let cookie of cookies) {
            cookie = cookie.trim();
            if (cookie.startsWith('jwt_token_value=')) {
                return decodeURIComponent(cookie.substring('jwt_token_value='.length));
            }
        }
        return null;
    }
    
    // 获取完整的令牌数据
    static getTokenData() {
        const cookies = document.cookie.split(';');
        for (let cookie of cookies) {
            cookie = cookie.trim();
            if (cookie.startsWith('jwt_token=')) {
                try {
                    return JSON.parse(decodeURIComponent(cookie.substring('jwt_token='.length)));
                } catch (e) {
                    console.error('解析令牌数据失败:', e);
                    return null;
                }
            }
        }
        return null;
    }
    
    // 检查是否有有效令牌
    static hasValidToken() {
        return this.getToken() !== null;
    }
    
    // 获取令牌类型
    static getTokenType() {
        const cookies = document.cookie.split(';');
        for (let cookie of cookies) {
            cookie = cookie.trim();
            if (cookie.startsWith('jwt_token_type=')) {
                return decodeURIComponent(cookie.substring('jwt_token_type='.length));
            }
        }
        return null;
    }
    
    // 获取过期时间
    static getExpiresIn() {
        const cookies = document.cookie.split(';');
        for (let cookie of cookies) {
            cookie = cookie.trim();
            if (cookie.startsWith('jwt_expires_in=')) {
                return decodeURIComponent(cookie.substring('jwt_expires_in='.length));
            }
        }
        return null;
    }
    
    // 清除令牌
    static clearToken() {
        const pastDate = new Date(0).toUTCString();
        const cookies = ['jwt_token', 'jwt_token_value', 'jwt_token_type', 'jwt_expires_in'];
        
        cookies.forEach(cookieName => {
            document.cookie = `${cookieName}=; expires=${pastDate}; path=/`;
        });
        
        console.log('JWT令牌已清除');
    }
    
    // 在API请求中自动添加JWT令牌到Authorization头
    static async fetchWithAuth(url, options = {}) {
        const token = this.getToken();
        if (!token) {
            throw new Error('未找到JWT令牌，请先登录');
        }
        
        const authOptions = {
            ...options,
            headers: {
                ...options.headers,
                'Authorization': `Bearer ${token}`
            }
        };
        
        return await fetch(url, authOptions);
    }
    
    // 显示令牌状态信息（用于调试）
    static showTokenStatus() {
        const token = this.getToken();
        const tokenData = this.getTokenData();
        
        if (token) {
            console.log('JWT令牌状态:');
            console.log('  - 令牌值:', token.substring(0, 20) + '...');
            console.log('  - 令牌类型:', this.getTokenType());
            console.log('  - 过期时间:', this.getExpiresIn());
            console.log('  - 完整数据:', tokenData);
        } else {
            console.log('未检测到JWT令牌');
        }
    }
}

// 全局函数，方便其他页面使用
window.JwtHelper = JwtHelper;

// 自动检查令牌状态（可选）
if (typeof window !== 'undefined') {
    window.addEventListener('load', () => {
        if (JwtHelper.hasValidToken()) {
            console.log('检测到有效的JWT令牌，可在API请求中使用');
        }
    });
}