import urllib.request
import argparse
import sys

def openfile(filename):
    try:
        with open(filename, "r", encoding='utf-8') as  f:
            content = f.read()
        print(f"✅ 文件读取成功，内容长度: {len(content)} 字符")
        return content
    except FileNotFoundError as e:
        print(f"❌ 错误：指定的文件不存在 -> {filename}")
        raise e
    except PermissionError as e:
        print(f"❌ 错误：没有权限读取文件 -> {filename}")
        raise e
    except Exception as e:
        print(f"❌ 错误：读取文件失败 -> {str(e)}")
        raise e

def httppost(url: str, msg: str) -> str:
    try:
        # 核心转换：字符串 → 字节流（原生库必须要求）
        post_body_bytes = msg.encode("utf-8")
        # 发送POST请求，timeout=10秒防止卡死
        response = urllib.request.urlopen(url, data=post_body_bytes, timeout=2)
        
        # 解析响应结果
        if response.getcode() == 200:
            resp_content = response.read().decode("utf-8")
            print("\n🎉 请求上传成功！HTTP状态码: 200")
            print(f"🎉 服务端响应内容:\n{resp_content}")
        else:
            resp_content = f'---{str(response.getcode())}---'
            print(f"\n⚠️ 请求上传完成，但服务端返回异常状态码: {response.getcode()}")
            print(f"⚠️ 服务端响应内容: {resp_content}")

        return resp_content

    except urllib.error.URLError as e:
        print(f"\n❌ 请求失败：网络错误/连接超时/端口未开放 -> {str(e)}")
        raise e
    except urllib.error.HTTPError as e:
        print(f"\n❌ 请求失败：HTTP错误 -> 状态码: {e.code}, 详情: {e.read().decode('utf-8')}")
        raise e 
    except Exception as e:
        print(f"\n❌ 请求失败：未知错误 -> {str(e)}")
        raise e

def upload_file(file: str, host: str, port: int) -> str:
    """抽象的URL访问函数，用于上传文件到指定服务器
    
    Args:
        file: 待上传的文件路径
        host: 目标服务器IP
        port: 目标服务器端口
    """
    try:
        # 读取文件内容
        msg = openfile(file)
        # 构造URL
        url = f"http://{host}:{port}/api/v1/mybot-sample"
        # 发送POST请求
        resp = httppost(url, msg)
    except Exception as e:
        print(f"❌ 错误：上传文件失败 -> {str(e)}")
        resp = str(e)
    return resp

def upload_file_and_train(file: str, host: str, port: int) -> str:
    """抽象的URL访问函数，用于上传文件到指定服务器
    
    Args:
        file: 待上传的文件路径
        host: 目标服务器IP
        port: 目标服务器端口
    """
    try:
        # 读取文件内容
        msg = openfile(file)
        # 构造URL
        url = f"http://{host}:{port}/api/v1/mybot-nn"
        # 发送POST请求
        resp = httppost(url, msg)
    except Exception as e:
        print(f"❌ 错误：上传文件失败 -> {str(e)}")
        resp = str(e)
    return resp


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="python 远程上传文本内容")
    parser.add_argument("file", type=str, help="待上传的文件")
    parser.add_argument('--port', type=int, default=9203, help="目标服务器监听端口，默认9203")
    parser.add_argument("--host", type=str, default="127.0.0.1", help="目标服务器的IP，默认本地")
    args =parser.parse_args()
    resp = upload_file(args.file, args.host, args.port)
    #resp = upload_file_and_train(args.file, args.host, args.port)
    print(resp)
