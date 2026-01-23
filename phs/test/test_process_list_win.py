import requests
import json

def test_create_process():
    """测试访问phs web服务的create接口"""
    # 设置请求URL
    url = "http://localhost:5200/api/v2/list"
    
    # 发送POST请求
    response = requests.get(url)
    
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
    # 执行测试函数
    test_create_process()