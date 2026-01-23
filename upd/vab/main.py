import sys
import requests
import argparse
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, 
    QWidget, QLineEdit, QPushButton, QTextEdit, QLabel, QScrollArea
)
from PySide6.QtWebEngineWidgets import QWebEngineView
from PySide6.QtCore import Qt, QUrl
from PySide6.QtGui import QFont
from bs4 import BeautifulSoup
import base64



class VABrowser(QMainWindow):
    """VA代理专用浏览器"""
    
    def __init__(self, proxy_url="http://localhost:9200"):
        super().__init__()
        self.proxy_url = proxy_url
        # 添加显示模式常量
        self.VIEW_HTML = 0
        self.VIEW_RAW = 1
        # 默认显示模式为HTML
        self.current_view = self.VIEW_HTML
        self.initUI()
    
    def initUI(self):
        """初始化用户界面"""
        self.setWindowTitle("VA Proxy Browser")
        self.setGeometry(100, 100, 1200, 800)
        
        # 创建主布局
        central_widget = QWidget()
        main_layout = QVBoxLayout(central_widget)
        
        # 创建代理设置区域
        proxy_layout = QHBoxLayout()
        
        self.proxy_label = QLabel("Proxy URL:")
        proxy_layout.addWidget(self.proxy_label)
        
        self.proxy_bar = QLineEdit()
        self.proxy_bar.setText(self.proxy_url)
        self.proxy_bar.setPlaceholderText("输入代理URL，例如：http://localhost:7203")
        proxy_layout.addWidget(self.proxy_bar)
        
        self.set_proxy_btn = QPushButton("Set Proxy")
        self.set_proxy_btn.clicked.connect(self.set_proxy)
        proxy_layout.addWidget(self.set_proxy_btn)
        
        main_layout.addLayout(proxy_layout)
        
        # 创建地址栏区域
        url_layout = QHBoxLayout()
        
        self.url_label = QLabel("URL:")
        url_layout.addWidget(self.url_label)
        
        self.url_bar = QLineEdit()
        self.url_bar.returnPressed.connect(self.load_url)
        self.url_bar.setPlaceholderText("输入URL，例如：http://qsont.xyz")
        url_layout.addWidget(self.url_bar)
        
        self.go_btn = QPushButton("Go")
        self.go_btn.clicked.connect(self.load_url)
        url_layout.addWidget(self.go_btn)
        
        self.reload_btn = QPushButton("Reload")
        self.reload_btn.clicked.connect(self.reload)
        url_layout.addWidget(self.reload_btn)
        
        # 添加显示模式选择
        self.view_mode_label = QLabel("显示模式：")
        url_layout.addWidget(self.view_mode_label)
        
        self.view_mode_combo = QPushButton("查看原始文本")
        self.view_mode_combo.clicked.connect(self.toggle_view_mode)
        url_layout.addWidget(self.view_mode_combo)
        
        main_layout.addLayout(url_layout)
        
        # 创建状态栏
        self.status_bar = QLabel("就绪")
        main_layout.addWidget(self.status_bar)
        
        # 创建Web浏览器显示区域
        self.web_view = QWebEngineView()
        
        # 创建原始文本显示区域
        self.text_edit = QTextEdit()
        self.text_edit.setReadOnly(True)
        self.text_edit.setFont(QFont("Courier New", 10))
        self.text_edit.setVisible(False)
        
        # 将两个控件添加到布局中
        main_layout.addWidget(self.web_view, 1)
        main_layout.addWidget(self.text_edit, 1)
        
        self.setCentralWidget(central_widget)
        
        # 不加载默认页面，等待用户输入URL
        self.status_bar.setText("就绪，请输入URL访问网站")
    
    def get_resource_via_proxy(self, resource_url):
        """通过VA代理获取资源"""
        try:
            # 通过VA代理发送请求
            proxy_data = {
                "url": resource_url,
                "method": "GET",
                "headers": {
                    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
                    "Accept-Language": "zh-CN,zh;q=0.9,en;q=0.8"
                }
            }
            
            response = requests.post(
                self.proxy_url,
                json=proxy_data,
                timeout=30
            )
            
            # 返回资源内容和MIME类型
            return response.content, response.headers.get('Content-Type', 'text/plain')
        except Exception as e:
            print(f"Error loading resource {resource_url}: {e}")
            return None, None
    
    def load_url(self, url=None):
        """通过VA代理加载指定URL，根据显示模式显示原始文本或网页"""
        if not url:
            url = self.url_bar.text()
        
        # 确保URL格式正确
        if not url.startswith(('http://', 'https://')):
            url = f"http://{url}"
        
        self.status_bar.setText(f"正在加载: {url}")
        
        try:
            # 通过VA代理发送请求，设置接受中文编码
            proxy_data = {
                "url": url,
                "method": "GET",
                "headers": {
                    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
                    "Accept-Language": "zh-CN,zh;q=0.9,en;q=0.8"
                }
            }
            
            response = requests.post(
                self.proxy_url,
                json=proxy_data,
                timeout=30
            )
            
            # 确保响应内容使用正确的编码
            response.encoding = response.apparent_encoding
            
            # 获取响应内容
            response_text = response.text
            
            if self.current_view == self.VIEW_RAW:
                # 显示原始HTTP响应
                raw_response = f"状态码: {response.status_code}\n"
                raw_response += f"URL: {response.url}\n"
                raw_response += f"编码: {response.encoding}\n"
                raw_response += "\n响应头:\n"
                for key, value in response.headers.items():
                    raw_response += f"{key}: {value}\n"
                raw_response += "\n响应内容:\n"
                raw_response += response_text
                
                self.text_edit.setText(raw_response)
                self.status_bar.setText(f"加载完成: {url} (原始文本模式)")
            else:
                # HTML模式，处理并显示网页
                # 使用BeautifulSoup解析HTML
                soup = BeautifulSoup(response_text, 'html.parser')
                
                # 处理所有图片标签
                for img_tag in soup.find_all('img'):
                    img_src = img_tag.get('src')
                    if img_src:
                        # 构造完整的图片URL
                        if not img_src.startswith(('http://', 'https://')):
                            from urllib.parse import urljoin
                            img_url = urljoin(url, img_src)
                        else:
                            img_url = img_src
                        
                        # 通过VA代理获取图片资源
                        img_content, img_mime = self.get_resource_via_proxy(img_url)
                        if img_content and img_mime:
                            # 将图片转换为base64格式
                            img_base64 = base64.b64encode(img_content).decode('utf-8')
                            # 替换图片URL为base64数据URL
                            img_tag['src'] = f"data:{img_mime};base64,{img_base64}"
                
                # 处理所有CSS链接标签
                for link_tag in soup.find_all('link', rel='stylesheet'):
                    css_href = link_tag.get('href')
                    if css_href:
                        # 构造完整的CSS URL
                        if not css_href.startswith(('http://', 'https://')):
                            from urllib.parse import urljoin
                            css_url = urljoin(url, css_href)
                        else:
                            css_url = css_href
                        
                        # 通过VA代理获取CSS资源
                        css_content, css_mime = self.get_resource_via_proxy(css_url)
                        if css_content and css_mime:
                            # 创建style标签，将CSS内容嵌入到HTML中
                            style_tag = soup.new_tag('style')
                            style_tag.string = css_content.decode('utf-8')
                            soup.head.append(style_tag)
                            # 移除原来的link标签
                            link_tag.extract()
                
                # 处理所有JavaScript脚本标签
                for script_tag in soup.find_all('script', src=True):
                    js_src = script_tag.get('src')
                    if js_src:
                        # 构造完整的JavaScript URL
                        if not js_src.startswith(('http://', 'https://')):
                            from urllib.parse import urljoin
                            js_url = urljoin(url, js_src)
                        else:
                            js_url = js_src
                        
                        # 通过VA代理获取JavaScript资源
                        js_content, js_mime = self.get_resource_via_proxy(js_url)
                        if js_content and js_mime:
                            # 创建script标签，将JavaScript内容嵌入到HTML中
                            new_script_tag = soup.new_tag('script')
                            new_script_tag.string = js_content.decode('utf-8')
                            # 替换原来的script标签
                            script_tag.replace_with(new_script_tag)
                
                # 将处理后的HTML内容转换为字符串
                processed_html = str(soup)
                
                # 将HTML内容加载到Web视图中
                self.web_view.setHtml(processed_html, baseUrl=url)
                self.status_bar.setText(f"加载完成: {url} (网页模式)")
            
        except Exception as e:
            # 显示错误信息
            error_msg = f"错误: {str(e)}\n"
            if self.current_view == self.VIEW_RAW:
                self.text_edit.setText(error_msg)
            else:
                error_html = f"<html><body><h1>加载错误</h1><p>{str(e)}</p></body></html>"
                self.web_view.setHtml(error_html)
            self.status_bar.setText(f"加载失败: {url}")
    
    def reload(self):
        """刷新当前页面"""
        self.load_url()
    
    def toggle_view_mode(self):
        """切换显示模式：HTML网页或原始文本"""
        if self.current_view == self.VIEW_HTML:
            # 切换到原始文本模式
            self.current_view = self.VIEW_RAW
            self.view_mode_combo.setText("查看网页")
            self.web_view.setVisible(False)
            self.text_edit.setVisible(True)
            # 重新加载当前URL以显示原始文本
            self.load_url()
        else:
            # 切换到HTML模式
            self.current_view = self.VIEW_HTML
            self.view_mode_combo.setText("查看原始文本")
            self.web_view.setVisible(True)
            self.text_edit.setVisible(False)
            # 重新加载当前URL以显示网页
            self.load_url()
    
    def set_proxy(self):
        """设置代理URL"""
        new_proxy_url = self.proxy_bar.text().strip()
        if new_proxy_url:
            self.proxy_url = new_proxy_url
            self.status_bar.setText(f"代理已更新: {self.proxy_url}")

if __name__ == "__main__":
    # 解析命令行参数
    parser = argparse.ArgumentParser(description='VA Proxy Browser')
    parser.add_argument('--proxy-url', type=str, default='http://localhost:7203', 
                        help='设置代理URL，例如：http://localhost:7203')
    args = parser.parse_args()
    
    app = QApplication(sys.argv)
    browser = VABrowser(proxy_url=args.proxy_url)
    browser.show()
    sys.exit(app.exec())
