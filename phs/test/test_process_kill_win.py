import requests
import json
import argparse


def test_kill_process(pid):
    """测试访问phs web服务的kill接口"""
    # 设置请求URL
    url = "http://localhost:5200/api/v2/kill"
    
    # 构造请求消息体
    payload = {
        "pid": pid
    }
    
    # 发送POST请求
    response = requests.post(url, json=payload)
    
    # 打印响应状态码
    print(f"响应状态码: {response.status_code}")
    
    # 尝试解析响应内容为JSON格式并打印
    try:
        response_json = response.json()
        print(f"响应内容: {json.dumps(response_json, indent=2, ensure_ascii=False)}")
    except json.JSONDecodeError:
        print(f"响应内容(非JSON格式): {response.text}")
    
    # 返回响应对象供进一步处理
    return response


if __name__ == "__main__":
    # 创建命令行参数解析器
    parser = argparse.ArgumentParser(description="测试访问phs web服务的kill接口")
    
    # 添加pid参数
    parser.add_argument("pid", type=int, help="要杀死的进程ID")
    
    # 解析命令行参数
    args = parser.parse_args()
    
    # 执行测试函数，传入命令行参数中的pid
    test_kill_process(args.pid)
