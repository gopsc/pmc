import requests
import json


def create_process(cmd_list, url="http://localhost:5200/api/v2/create"):
    """创建任意进程
    
    Args:
        cmd_list (list): 命令和参数列表，例如 ["timeout", "/t", "1000"]
        url (str, optional): 请求URL，默认为 "http://localhost:5200/api/v2/create"
    
    Returns:
        requests.Response: 响应对象
    """
    try:
        # 发送POST请求
        response = requests.post(url, data=json.dumps(cmd_list), timeout=30)
        
        # 返回响应对象供进一步处理
        return response
    except requests.exceptions.RequestException as e:
        print(f"创建进程失败: {e}")
        return None


def create_process_with_output(cmd_list, url="http://localhost:5200/api/v2/create"):
    """创建任意进程并打印输出
    
    Args:
        cmd_list (list): 命令和参数列表，例如 ["timeout", "/t", "1000"]
        url (str, optional): 请求URL，默认为 "http://localhost:5200/api/v2/create"
    
    Returns:
        dict or str: 响应内容，JSON解析后的字典或原始文本
    """
    response = create_process(cmd_list, url)
    
    if response is None:
        return None
    
    # 打印响应状态码
    print(f"响应状态码: {response.status_code}")
    
    # 尝试解析响应内容为JSON格式并打印
    try:
        response_json = response.json()
        print(f"响应内容: {json.dumps(response_json, indent=2, ensure_ascii=False)}")
        return response_json
    except json.JSONDecodeError:
        print(f"响应内容(非JSON格式): {response.text}")
        return response.text


if __name__ == "__main__":
    # 示例用法
    # 创建一个timeout进程，延迟1000毫秒
    cmd = ["timeout", "/t", "1000"]
    
    # 使用默认URL
    print("使用默认URL:")
    create_process_with_output(cmd)
    
    # 示例：使用自定义URL（注释掉，可根据需要取消注释）
    # print("\n使用自定义URL:")
    # custom_url = "http://localhost:5200/api/v2/create"
    # create_process_with_output(cmd, custom_url)
