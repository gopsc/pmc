from flask import Flask, render_template, send_from_directory,jsonify , request, Response
from flask_cors import CORS
import os
import mimetypes
import argparse
import secrets
import json
import jwt
import datetime
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding, hashes
from cryptography.hazmat.primitives.asymmetric import padding as asymmetric_padding
from base64 import b64encode, b64decode
from git import Repo
from functools import wraps

# 添加命令行参数解析
parser = argparse.ArgumentParser(description='Flask Directory Browser with RSA and AES Key Generation')
parser.add_argument('--generate-rsa', action='store_true', help='Generate RSA key pair and exit')
parser.add_argument('--generate-aes', action='store_true', help='Generate AES key and exit')
parser.add_argument('--key-size', type=int, default=2048, help='RSA key size (default: 2048)')
parser.add_argument('--aes-key-size', type=int, choices=[128, 192, 256], default=256, help='AES key size in bits (default: 256)')
parser.add_argument('--private-key', type=str, default='private_key.pem', help='Private key file name (default: private_key.pem)')
parser.add_argument('--public-key', type=str, default='public_key.pem', help='Public key file name (default: public_key.pem)')
parser.add_argument('--aes-key', type=str, default='aes_key.txt', help='AES key file name (default: aes_key.txt)')
parser.add_argument('--aes-format', type=str, choices=['hex', 'base64', 'raw'], default='hex', help='AES key format (default: hex)')
args = parser.parse_args()

# 强制使用2048位密钥，这是Web Crypto API最佳支持的大小
args.key_size = 2048

# JWT令牌验证装饰器
def require_jwt_token(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        # JWT密钥（与生成令牌时使用的密钥一致）
        jwt_secret = 'your-secret-key-change-in-production'
        
        # 从Authorization头获取令牌
        auth_header = request.headers.get('Authorization')
        
        if not auth_header:
            return jsonify({'error': '缺少Authorization头，需要JWT令牌进行身份验证'}), 401
        
        # 检查Bearer格式
        if not auth_header.startswith('Bearer '):
            return jsonify({'error': 'Authorization头格式错误，应为Bearer <token>'}), 401
        
        token = auth_header[7:]  # 去掉'Bearer '前缀
        
        try:
            # 验证JWT令牌
            payload = jwt.decode(token, jwt_secret, algorithms=['HS256'])
            
            # 检查令牌是否过期
            if 'exp' in payload:
                expiration_time = datetime.datetime.fromtimestamp(payload['exp'])
                if expiration_time < datetime.datetime.utcnow():
                    return jsonify({'error': 'JWT令牌已过期，请重新获取'}), 401
            
            # 将令牌信息存储到请求上下文中，供后续使用
            request.jwt_payload = payload
            
        except jwt.ExpiredSignatureError:
            return jsonify({'error': 'JWT令牌已过期，请重新获取'}), 401
        except jwt.InvalidTokenError as e:
            return jsonify({'error': f'无效的JWT令牌: {str(e)}'}), 401
        except Exception as e:
            return jsonify({'error': f'令牌验证失败: {str(e)}'}), 401
        
        return f(*args, **kwargs)
    
    return decorated_function

# 生成 RSA 密钥对的函数
def generate_rsa_key_pair(key_size=2048, private_key_file='private_key.pem', public_key_file='public_key.pem'):
    # 生成私钥
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=key_size,
        backend=default_backend()
    )
    
    # 生成公钥
    public_key = private_key.public_key()
    
    # 保存私钥 - 使用 PKCS#8 格式，这是 Web Crypto API 更广泛支持的格式
    with open(private_key_file, 'wb') as f:
        f.write(private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption()
        ))
    
    # 保存公钥
    with open(public_key_file, 'wb') as f:
        f.write(public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        ))
    
    print(f"RSA key pair generated successfully:")
    print(f"  Private key: {os.path.abspath(private_key_file)}")
    print(f"  Public key: {os.path.abspath(public_key_file)}")
    print(f"  Key size: {key_size} bits")

# 生成 AES 密钥的函数
def generate_aes_key(key_size=256, key_file='aes_key.txt', format='hex'):
    # 生成随机 AES 密钥
    key_bytes = secrets.token_bytes(key_size // 8)
    
    # 根据格式编码密钥
    if format == 'hex':
        key_encoded = key_bytes.hex()
    elif format == 'base64':
        key_encoded = b64encode(key_bytes).decode('utf-8')
    else:  # raw
        key_encoded = key_bytes
    
    # 保存密钥到文件
    if format == 'raw':
        with open(key_file, 'wb') as f:
            f.write(key_encoded)
    else:
        with open(key_file, 'w') as f:
            f.write(key_encoded)
    
    print(f"AES key generated successfully:")
    print(f"  Key file: {os.path.abspath(key_file)}")
    print(f"  Key size: {key_size} bits")
    print(f"  Format: {format}")
    print(f"  Key: {key_encoded}")

# 如果请求生成 RSA 密钥，则生成密钥并退出
if args.generate_rsa:
    generate_rsa_key_pair(args.key_size, args.private_key, args.public_key)
    exit(0)

# 如果请求生成 AES 密钥，则生成密钥并退出
if args.generate_aes:
    generate_aes_key(args.aes_key_size, args.aes_key, args.aes_format)
    exit(0)

# 确保 AES 密钥文件存在
def ensure_aes_key():
    aes_key_file = 'aes_key.txt'
    if not os.path.exists(aes_key_file):
        generate_aes_key(256, aes_key_file, 'hex')
    
    # 读取 AES 密钥
    with open(aes_key_file, 'r') as f:
        aes_key_hex = f.read().strip()
    
    return bytes.fromhex(aes_key_hex)

def ensure_rsa_key():
    private_key_file = 'private_key.pem'
    public_key_file = 'public_key.pem'
    if not os.path.exists(private_key_file) or not os.path.exists(public_key_file):
        generate_rsa_key_pair(2048, private_key_file, public_key_file)

# 读取 RSA 公钥
def get_rsa_public_key():
    public_key_file = 'public_key.pem'
    if not os.path.exists(public_key_file):
        generate_rsa_key_pair(2048, 'private_key.pem', public_key_file)
    
    with open(public_key_file, 'rb') as f:
        public_key = serialization.load_pem_public_key(
            f.read(),
            backend=default_backend()
        )
    
    return public_key

# 初始化 AES 密钥
aes_key = ensure_aes_key()
ensure_rsa_key()
app = Flask(__name__)
CORS(app)

# 配置文件路径
CONFIG_FILE = 'config.ini'

# 默认配置
DEFAULT_CONFIG = {
    'server': {
        'port': '5000',
        'host': '0.0.0.0'
    },
    'directory': {
        'root': '.'
    }
}

# 加载配置
config = DEFAULT_CONFIG.copy()

# 简单的INI配置文件解析
if os.path.exists(CONFIG_FILE):
    with open(CONFIG_FILE, 'r') as f:
        current_section = None
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            if line.startswith('[') and line.endswith(']'):
                current_section = line[1:-1]
                if current_section not in config:
                    config[current_section] = {}
            elif '=' in line and current_section:
                key, value = line.split('=', 1)
                config[current_section][key.strip()] = value.strip()

# 获取根目录
ROOT_DIR = config['directory']['root']

# 修改为HTML根目录
HTML_ROOT_DIR = 'html'

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def serve_html(path):
    """服务html目录下的文件和目录，类似Apache2的静态文件服务"""
    # 构建真实路径
    real_path = os.path.join(HTML_ROOT_DIR, path)
    
    # 安全检查：确保不会访问到html目录之外的内容
    if not os.path.realpath(real_path).startswith(os.path.realpath(HTML_ROOT_DIR)):
        return "Access denied", 403
    
    # 检查路径是否存在
    if not os.path.exists(real_path):
        return "Path not found", 404
    
    # 如果是文件，返回文件内容
    if os.path.isfile(real_path):
        # 读取文件内容
        with open(real_path, 'rb') as f:
            file_content = f.read()
        
        # 根据文件扩展名设置Content-Type
        file_ext = os.path.splitext(real_path)[1].lower()
        if file_ext == '.html':
            return Response(file_content, mimetype='text/html')
        elif file_ext == '.js':
            return Response(file_content, mimetype='application/javascript')
        elif file_ext == '.css':
            return Response(file_content, mimetype='text/css')
        elif file_ext == '.json':
            return Response(file_content, mimetype='application/json')
        elif file_ext in ['.png', '.jpg', '.jpeg', '.gif']:
            return Response(file_content, mimetype=f'image/{file_ext[1:]}')
        else:
            return Response(file_content, mimetype='text/plain')
    
    # 如果是目录，列出目录内容
    items = []
    for item in os.listdir(real_path):
        item_path = os.path.join(real_path, item)
        item_rel_path = os.path.join(path, item)
        item_type = 'dir' if os.path.isdir(item_path) else 'file'
        items.append({
            'name': item,
            'type': item_type,
            'path': item_rel_path,
            'size': os.path.getsize(item_path) if item_type == 'file' else 0,
            'modified': os.path.getmtime(item_path)
        })
    
    # 按类型排序，目录在前，文件在后
    items.sort(key=lambda x: (x['type'] != 'dir', x['name']))
    
    # 生成父目录路径
    parent_path = os.path.dirname(path) if path != '' else ''
    
    # 读取模板文件内容
    view_template_path = os.path.join(HTML_ROOT_DIR, '__view.html')
    if not os.path.exists(view_template_path):
        return "View template not found", 500
    
    with open(view_template_path, 'r') as f:
        template_content = f.read()
    
    # 生成路径导航
    path_nav = ''
    if path != '':
        path_parts = path.split('/')
        for i in range(len(path_parts)):
            current_subpath = '/'.join(path_parts[:i+1])
            path_nav += f' / <a href="/{current_subpath}">{path_parts[i]}</a>'
    
    # 生成父目录链接
    parent_link = ''
    if parent_path:
        parent_link = f'<tr><td><span class="icon">📁</span><a href="/{parent_path}" class="dir-name">..</a></td><td></td><td></td></tr>'
    
    # 生成文件列表
    file_list = ''
    for item in items:
        if item['type'] == 'dir':
            file_list += f'<tr><td><span class="icon">📁</span><a href="/{item["path"]}" class="dir-name">{item["name"]}</a></td><td></td><td>{item["modified"]}</td></tr>'
        else:
            file_list += f'<tr><td><span class="icon">📄</span><a href="/{item["path"]}" class="file-name">{item["name"]}</a></td><td class="file-size">{item["size"]} bytes</td><td class="file-modified">{item["modified"]}</td></tr>'
    
    # 替换模板中的占位符
    rendered_html = template_content.replace('{path}', path)
    rendered_html = rendered_html.replace('{path_nav}', path_nav)
    rendered_html = rendered_html.replace('{parent_link}', parent_link)
    rendered_html = rendered_html.replace('{file_list}', file_list)
    
    return Response(rendered_html, mimetype='text/html')

@app.route('/get-encrypted-aes-key')
def get_encrypted_aes_key():
    """获取通过RSA公钥加密的AES密钥"""
    global aes_key
    
    try:
        # 确保AES密钥存在
        if aes_key is None:
            aes_key = ensure_aes_key()
        
        # 获取RSA公钥
        rsa_public_key = get_rsa_public_key()
        
        # 使用RSA公钥加密AES密钥
        encrypted_aes_key = rsa_public_key.encrypt(
            aes_key,
            asymmetric_padding.OAEP(
                mgf=asymmetric_padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )
        
        # 将加密后的AES密钥转换为base64格式返回
        encrypted_aes_key_base64 = b64encode(encrypted_aes_key).decode('utf-8')
        
        return {
            'encrypted_aes_key': encrypted_aes_key_base64,
            'aes_key_size': len(aes_key) * 8
        }
    except Exception as e:
        return {
            'error': f"Failed to get encrypted AES key: {str(e)}"
        }, 500

@app.route('/upload', methods=['POST'])
def upload_file():
    """上传通过AES加密的文件"""
    global aes_key
    
    try:
        # 检查请求中是否包含文件
        if 'file' not in request.files:
            return {'error': 'No file part in the request'}, 400
        
        file = request.files['file']
        
        # 检查文件是否有文件名
        if file.filename == '':
            return {'error': 'No file selected for uploading'}, 400
        
        # 确保AES密钥存在
        if aes_key is None:
            aes_key = ensure_aes_key()
        
        # 获取上传目录 - 改为html目录下
        upload_dir = HTML_ROOT_DIR
        
        # 读取加密文件内容
        encrypted_data = file.read()
        
        # 解密文件
        # Check data length - AES-GCM 需要 IV + GCM 标签，数据部分可以是任意长度（包括0字节）
        if len(encrypted_data) < 16 + 16:  # IV (16) + GCM 标签 (16) + 至少 0 字节数据
            return {'error': 'Invalid encrypted data'}, 400
        
        # 提取 IV（前16字节）和标签（后16字节）
        iv = encrypted_data[:16]
        tag = encrypted_data[-16:]
        encrypted_content = encrypted_data[16:-16]
        
        # 创建解密器 - 使用 AES-GCM 算法，与前端保持一致
        cipher = Cipher(algorithms.AES(aes_key), modes.GCM(iv, tag), backend=default_backend())
        decryptor = cipher.decryptor()
        
        # 解密文件内容
        decrypted_content = decryptor.update(encrypted_content) + decryptor.finalize()
        
        # 构建保存路径
        save_path = os.path.join(upload_dir, file.filename)
        
        # 安全检查：确保不会保存到根目录之外
        if not os.path.realpath(save_path).startswith(os.path.realpath(ROOT_DIR)):
            return {'error': 'Invalid file path'}, 403
        
        # 保存解密后的文件
        with open(save_path, 'wb') as f:
            f.write(decrypted_content)
        
        # 重新生成AES密钥
        generate_aes_key(256, 'aes_key.txt', 'hex')
        
        # 更新全局aes_key变量
        aes_key = ensure_aes_key()
        
        return {
            'success': True,
            'filename': file.filename,
            'message': 'File uploaded successfully. AES key has been regenerated.'
        }
    except Exception as e:
        return {
            'error': f"Failed to upload file: {str(e)}"
        }, 500

@app.route('/execute-bash', methods=['POST'])
def execute_bash():
    """执行通过AES加密的bash脚本"""
    global aes_key
    
    try:
        # 检查请求中是否包含bash脚本
        if 'script' not in request.files:
            return {'error': 'No script part in the request'}, 400
        
        script_file = request.files['script']
        
        # 确保AES密钥存在
        if aes_key is None:
            aes_key = ensure_aes_key()
        
        # 读取加密脚本内容
        encrypted_data = script_file.read()
        
        # 解密脚本
        # 检查数据长度是否足够 - AES-GCM 需要 IV + GCM 标签，数据部分可以是任意长度（包括0字节）
        if len(encrypted_data) < 16 + 16:  # IV (16) + GCM 标签 (16) + 至少 0 字节数据
            return {'error': 'Invalid encrypted data'}, 400
        
        # 提取 IV（前16字节）和标签（后16字节）
        iv = encrypted_data[:16]
        tag = encrypted_data[-16:]
        encrypted_content = encrypted_data[16:-16]
        
        # 创建解密器 - 使用 AES-GCM 算法，与前端保持一致
        cipher = Cipher(algorithms.AES(aes_key), modes.GCM(iv, tag), backend=default_backend())
        decryptor = cipher.decryptor()
        
        # 解密脚本内容
        decrypted_content = decryptor.update(encrypted_content) + decryptor.finalize()
        
        # 将解密后的脚本转换为字符串
        bash_script = decrypted_content.decode('utf-8')
        
        # 执行bash脚本
        import subprocess
        result = subprocess.run(
            bash_script,
            shell=True,
            capture_output=True,
            text=True,
            cwd=HTML_ROOT_DIR  # 在html目录下执行脚本
        )
        
        # 重新生成AES密钥
        generate_aes_key(256, 'aes_key.txt', 'hex')
        
        # 更新全局aes_key变量
        aes_key = ensure_aes_key()
        
        return {
            'success': True,
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode,
            'message': 'Bash script executed successfully. AES key has been regenerated.'
        }
    except Exception as e:
        return {
            'error': f"Failed to execute bash script: {str(e)}"
        }, 500

@app.route('/delete', methods=['POST'])
def delete_file():
    """删除通过AES加密的文件"""
    global aes_key
    
    try:
        # 检查请求中是否包含要删除的文件
        if 'file' not in request.files:
            return {'error': 'No file part in the request'}, 400
        
        file = request.files['file']
        
        # 确保AES密钥存在
        if aes_key is None:
            aes_key = ensure_aes_key()
        
        # 读取加密文件名数据
        encrypted_data = file.read()
        
        # 解密文件名
        # 检查数据长度是否足够 - AES-GCM 需要 IV + GCM 标签，数据部分可以是任意长度（包括0字节）
        if len(encrypted_data) < 16 + 16:  # IV (16) + GCM 标签 (16) + 至少 0 字节数据
            return {'error': 'Invalid encrypted data'}, 400
        
        # 提取 IV（前16字节）和标签（后16字节）
        iv = encrypted_data[:16]
        tag = encrypted_data[-16:]
        encrypted_content = encrypted_data[16:-16]
        
        # 创建解密器 - 使用 AES-GCM 算法，与前端保持一致
        cipher = Cipher(algorithms.AES(aes_key), modes.GCM(iv, tag), backend=default_backend())
        decryptor = cipher.decryptor()
        
        # 解密文件名
        decrypted_content = decryptor.update(encrypted_content) + decryptor.finalize()
        
        # 将解密后的文件名转换为字符串
        filename = decrypted_content.decode('utf-8')
        
        # 构建文件路径
        file_path = os.path.join(HTML_ROOT_DIR, filename)
        
        # 安全检查：确保不会删除html目录之外的文件
        if not os.path.realpath(file_path).startswith(os.path.realpath(HTML_ROOT_DIR)):
            return {'error': 'Invalid file path'}, 403
        
        # 检查文件是否存在
        if not os.path.exists(file_path):
            return {'error': 'File not found'}, 404
        
        # 检查是否为文件（不是目录）
        if not os.path.isfile(file_path):
            return {'error': 'Path is not a file'}, 400
        
        # 删除文件
        os.remove(file_path)
        
        # 重新生成AES密钥
        generate_aes_key(256, 'aes_key.txt', 'hex')
        
        # 更新全局aes_key变量
        aes_key = ensure_aes_key()
        
        return {
            'success': True,
            'filename': filename,
            'message': 'File deleted successfully. AES key has been regenerated.'
        }
    except Exception as e:
        return {
            'error': f"Failed to delete file: {str(e)}"
        }, 500


'''def start_subsystems(name: str):
    comm = PmcComm('http://localhost:8012')
    comm.Set_RSA_Pri_Key('private_key.pem')
    comm.Try_Get_AES_Key()
    comm.try_start(name)'''

'''
请求URL： http://localhost:9200/get_module_from_git 
请求头："Content-Type: application/json"
请求方法： POST
请求体：{"git":" https://gitcode.com/qingss0/phs "}
响应体：
{"git":" https://gitcode.com/qingss0/phs ","status":"success"}
'''
def process_git_repository(git_url):
    clone_dir = os.path.join(os.getcwd(), '..')
    os.makedirs(clone_dir, exist_ok=True)
    repo_name = git_url.split('/')[-1].replace('.git', '')
    repo_path = os.path.join(clone_dir, repo_name)
    if os.path.exists(repo_path):
        Repo(repo_path).remotes.origin.pull()
    else:
        Repo.clone_from(git_url, repo_path)
    return repo_path

'''
【分析仓库】
请求URL：http://localhost:9200/analyze_repository
请求头："Content-Type: application/json"
请求方法：POST
请求体：{"git":"https://gitcode.com/qingss0/phs"}
响应体：

{
    "git": "https://gitcode.com/qingss0/phs",
    "result": {
        "active_branch": "main",
        "branches": [
            "main"
        ],
        "description": "Unnamed repository; edit this file 'description' to name the repository.",
        "git_url": "https://gitcode.com/qingss0/phs",
        "head_commit": {
            "author": "qingss0",
            "date": "2025-12-31T16:54:23+08:00",
            "message": "update: \u66f4\u65b0\u6587\u4ef6 README.md\n\n\nSigned-off-by: qingss0 <qingss0@noreply.gitcode.com>",
            "sha": "fee9c2c2f371b7b3408593eb7ae6b52760542d64"
        },
        "is_dirty": false,
        "remotes": [
            "origin"
        ],
        "repo_name": "phs",
        "repo_path": "/bot/pmc/_init/../users/phs",
        "total_commits": 4,
        "untracked_files": []
    },
    "status": "success"
}
'''
def analyze_repository(git_url):
    clone_dir = os.path.join(os.getcwd(), '..')
    os.makedirs(clone_dir, exist_ok=True)
    repo_name = git_url.split('/')[-1].replace('.git', '')
    repo_path = os.path.join(clone_dir, repo_name)
    
    if not os.path.exists(repo_path):
        Repo.clone_from(git_url, repo_path)
    
    repo = Repo(repo_path)
    
    result = {
        'repo_name': repo_name,
        'repo_path': repo_path,
        'active_branch': repo.active_branch.name,
        'branches': [branch.name for branch in repo.branches],
        'remotes': [remote.name for remote in repo.remotes],
        'head_commit': {
            'sha': repo.head.commit.hexsha,
            'message': repo.head.commit.message.strip(),
            'author': str(repo.head.commit.author),
            'date': repo.head.commit.committed_datetime.isoformat()
        },
        'total_commits': len(list(repo.iter_commits())),
        'is_dirty': repo.is_dirty(),
        'untracked_files': repo.untracked_files,
        'description': repo.description if repo.description else '',
        'git_url': git_url
    }
    
    return result

# curl -X POST -H "Content-Type: application/json" http://localhost:9200/get_module_from_git -d '{"git":"https://gitcode.com/qingss0/chat"}'
@app.route('/get_module_from_git', methods=['POST'])
@require_jwt_token
def get_module_from_git():
    data = request.get_json()
    if not data or 'git' not in data:
        raise ValueError('Missing required parameter: git')
    ########
    git_url = data.get('git')
    if not git_url:
        raise ValueError('Parameter git cannot be empty')
    ########
    process_git_repository(git_url)

    return jsonify({'status': 'success', 'git': git_url})

@app.route('/analyze_repository', methods=['POST'])
@require_jwt_token
def analyze_repository_view():
    data = request.get_json()
    if not data or 'git' not in data:
        raise ValueError('Missing required parameter: git')
    ########
    git_url = data.get('git')
    if not git_url:
        raise ValueError('Parameter git cannot be empty')
    ########
    result = analyze_repository(git_url)

    return jsonify({'status': 'success', 'git': git_url, 'result': result})

@app.route('/get_jwt_token', methods=['POST'])
def get_jwt_token():
    """获取JWT令牌 - 要求前端传回AES加密的"i-wanna-login"数据"""
    global aes_key
    
    try:
        # 检查请求中是否包含加密数据
        if 'encrypted_data' not in request.files:
            return jsonify({
                'error': 'No encrypted data part in the request'
            }), 400
        
        encrypted_file = request.files['encrypted_data']
        
        # 确保AES密钥存在
        if aes_key is None:
            aes_key = ensure_aes_key()
        
        # 读取加密数据内容
        encrypted_data = encrypted_file.read()
        
        # 检查数据长度是否足够 - AES-GCM 需要 IV + GCM 标签，数据部分可以是任意长度（包括0字节）
        if len(encrypted_data) < 16 + 16:  # IV (16) + GCM 标签 (16) + 至少 0 字节数据
            return jsonify({
                'error': 'Invalid encrypted data'
            }), 400
        
        # 提取 IV（前16字节）和标签（后16字节）
        iv = encrypted_data[:16]
        tag = encrypted_data[-16:]
        encrypted_content = encrypted_data[16:-16]
        
        # 创建解密器 - 使用 AES-GCM 算法，与前端保持一致
        cipher = Cipher(algorithms.AES(aes_key), modes.GCM(iv, tag), backend=default_backend())
        decryptor = cipher.decryptor()
        
        # 解密数据内容
        decrypted_content = decryptor.update(encrypted_content) + decryptor.finalize()
        
        # 将解密后的数据转换为字符串
        decrypted_text = decrypted_content.decode('utf-8')
        
        # 解析JSON数据
        try:
            data = json.loads(decrypted_text)
        except json.JSONDecodeError:
            return jsonify({
                'error': 'Invalid JSON format in decrypted data'
            }), 400
        
        # 检查是否包含"i-wanna-login"字段且值为true
        if not data or data.get('i-wanna-login') != True:
            return jsonify({
                'error': 'Invalid request. Send AES encrypted {"i-wanna-login": true} to get JWT token'
            }), 400
        
        # JWT密钥（在实际应用中应该使用更安全的密钥）
        jwt_secret = 'your-secret-key-change-in-production'
        
        # 生成JWT令牌
        payload = {
            'iss': 'upd-server',
            'sub': 'user-login',
            'iat': datetime.datetime.utcnow(),
            'exp': datetime.datetime.utcnow() + datetime.timedelta(hours=24),  # 24小时过期
            'user_id': 'anonymous',
            'permissions': ['read', 'write', 'execute']
        }
        
        # 生成JWT令牌
        token = jwt.encode(payload, jwt_secret, algorithm='HS256')
        
        # 重新生成AES密钥
        generate_aes_key(256, 'aes_key.txt', 'hex')
        
        # 更新全局aes_key变量
        aes_key = ensure_aes_key()
        
        return jsonify({
            'status': 'success',
            'token': token,
            'expires_in': '24 hours',
            'token_type': 'Bearer',
            'message': 'JWT token generated successfully. AES key has been regenerated.'
        })
        
    except Exception as e:
        return jsonify({
            'error': f'Failed to generate JWT token: {str(e)}'
        }), 500

# SSH文件读取视图 - 需要系统密码
@app.route('/ssh_read_file', methods=['POST'])
def ssh_read_file():
    """通过SSH客户端获取目标机器上一个文件的内容，需要系统密码"""
    try:
        data = request.get_json()
        
        # 检查必需参数
        if not data:
            return jsonify({'error': '缺少请求数据'}), 400
        
        required_fields = ['host', 'username', 'password', 'file_path']
        for field in required_fields:
            if field not in data:
                return jsonify({'error': f'缺少必需参数: {field}'}), 400
        
        host = data['host']
        username = data['username']
        password = data['password']
        file_path = data['file_path']
        port = data.get('port', 22)  # 默认SSH端口
        
        # 导入paramiko库（如果可用）
        try:
            import paramiko
        except ImportError:
            return jsonify({
                'error': 'paramiko库未安装，请安装: pip install paramiko',
                'install_command': 'pip install paramiko'
            }), 500
        
        # 创建SSH客户端
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        
        try:
            # 连接SSH服务器
            ssh.connect(host, port=port, username=username, password=password, timeout=30)
            
            # 读取文件内容
            sftp = ssh.open_sftp()
            try:
                with sftp.open(file_path, 'r') as remote_file:
                    file_content = remote_file.read().decode('utf-8', errors='ignore')
            except UnicodeDecodeError:
                # 如果UTF-8解码失败，尝试其他编码
                with sftp.open(file_path, 'r') as remote_file:
                    file_content = remote_file.read().decode('latin-1', errors='ignore')
            
            # 获取文件信息
            file_stat = sftp.stat(file_path)
            
            return jsonify({
                'status': 'success',
                'file_path': file_path,
                'file_content': file_content,
                'file_size': file_stat.st_size,
                'last_modified': file_stat.st_mtime,
                'message': '文件读取成功'
            })
            
        except paramiko.AuthenticationException:
            return jsonify({'error': 'SSH认证失败，请检查用户名和密码'}), 401
        except paramiko.SSHException as e:
            return jsonify({'error': f'SSH连接失败: {str(e)}'}), 500
        except FileNotFoundError:
            return jsonify({'error': f'文件不存在: {file_path}'}), 404
        except PermissionError:
            return jsonify({'error': f'没有权限读取文件: {file_path}'}), 403
        except Exception as e:
            return jsonify({'error': f'读取文件失败: {str(e)}'}), 500
        finally:
            ssh.close()
            
    except Exception as e:
        return jsonify({'error': f'请求处理失败: {str(e)}'}), 500


if __name__ == '__main__':
    import configparser
    import os
    import ssl
    
    # 读取配置文件
    config = configparser.ConfigParser()
    config.read('config.ini')
    
    # 获取服务器配置
    host = config['server']['host']
    port = int(config['server']['port'])
    
    # 检查SSL证书是否存在
    cert_file = 'cert.pem'
    key_file = 'key.pem'
    
    if os.path.exists(cert_file) and os.path.exists(key_file):
        print(f"Starting upd https server on {host}:{port}...")
        # 使用Flask内置服务器启动HTTPS
        app.run(host=host, port=port, ssl_context=(cert_file, key_file), debug=False)
    else:
        print(f"SSL certificates not found at {cert_file} and {key_file}")
        print(f"Starting upd http server on {host}:{port}...")
        # 使用Flask内置服务器启动HTTP
        app.run(host=host, port=port, debug=False)
