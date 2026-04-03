#!/usr/bin/env python3
"""
图片标注工具
用于手动标注图像中的目标区域并保存位置坐标（百分比格式）到txt文件
支持矩形框标注、预览、编辑和删除功能
"""

import os
import sys
import json
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, simpledialog
from PIL import Image, ImageTk, ImageDraw
import cv2
import numpy as np
from datetime import datetime
from typing import List, Optional, Tuple
import threading


class ImageAnnotationTool:
    """图片标注工具 - 支持手动矩形框标注"""
    
    def __init__(self, root):
        self.root = root
        self.root.title("图片标注工具")
        self.root.geometry("1200x800")
        
        # 数据存储
        self.current_image_path = None
        self.current_image = None
        self.image_pil = None
        self.image_tk = None
        self.zoom_factor = 1.0
        self.image_display = None
        self.annotations = []  # 存储标注：[(x1, y1, x2, y2), ...]  # 移除了label字段
        
        # 标注状态
        self.drawing = False
        self.start_x = None
        self.start_y = None
        self.current_rect = None
        self.selected_annotation = -1  # 当前选中的标注索引
        self.drag_mode = False  # 是否在拖拽模式
        self.drag_start = None  # 拖拽起始点
        
        # 缩放和平移
        self.offset_x = 0
        self.offset_y = 0
        self.pan_start_x = 0
        self.pan_start_y = 0
        self.panning = False
        
        # 创建UI
        self.setup_ui()
        
        # 绑定事件
        self.setup_bindings()
        
        # 更新状态
        self.update_status()
    
    def setup_ui(self):
        """设置用户界面"""
        # 创建主框架
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 左侧控制面板
        control_frame = ttk.Frame(main_frame, width=250)
        control_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 5))
        control_frame.pack_propagate(False)
        
        # 文件操作
        file_group = ttk.LabelFrame(control_frame, text="文件操作", padding=5)
        file_group.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Button(file_group, text="打开图片", command=self.open_image).pack(fill=tk.X, pady=2)
        ttk.Button(file_group, text="保存标注", command=self.save_annotations).pack(fill=tk.X, pady=2)
        ttk.Button(file_group, text="加载标注", command=self.load_annotations).pack(fill=tk.X, pady=2)
        
        # 标注操作
        anno_group = ttk.LabelFrame(control_frame, text="标注操作", padding=5)
        anno_group.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Button(anno_group, text="清除当前标注", command=self.clear_current_annotation).pack(fill=tk.X, pady=2)
        ttk.Button(anno_group, text="删除所有标注", command=self.clear_all_annotations).pack(fill=tk.X, pady=2)
        ttk.Button(anno_group, text="删除选中标注", command=self.delete_selected_annotation).pack(fill=tk.X, pady=2)
        
        # 标注列表
        list_group = ttk.LabelFrame(control_frame, text="标注列表", padding=5)
        list_group.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # 创建列表框和滚动条
        list_frame = ttk.Frame(list_group)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        self.annotation_listbox = tk.Listbox(list_frame, height=15)
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.annotation_listbox.yview)
        self.annotation_listbox.configure(yscrollcommand=scrollbar.set)
        
        self.annotation_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.annotation_listbox.bind('<<ListboxSelect>>', self.on_annotation_select)
        
        # 信息显示
        info_group = ttk.LabelFrame(control_frame, text="信息", padding=5)
        info_group.pack(fill=tk.X)
        
        self.info_label = ttk.Label(info_group, text="未加载图片", wraplength=220)
        self.info_label.pack(fill=tk.X, pady=2)
        
        self.status_label = ttk.Label(info_group, text="状态: 就绪", wraplength=220)
        self.status_label.pack(fill=tk.X, pady=2)
        
        # 右侧画布区域
        canvas_frame = ttk.Frame(main_frame)
        canvas_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # 工具栏
        toolbar = ttk.Frame(canvas_frame)
        toolbar.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Button(toolbar, text="放大", command=self.zoom_in).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="缩小", command=self.zoom_out).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="适应窗口", command=self.fit_to_window).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="重置视图", command=self.reset_view).pack(side=tk.LEFT, padx=2)
        
        self.draw_mode_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(toolbar, text="绘制模式", variable=self.draw_mode_var).pack(side=tk.LEFT, padx=5)
        
        # 画布
        self.canvas = tk.Canvas(canvas_frame, bg='gray', cursor='cross')
        self.canvas.pack(fill=tk.BOTH, expand=True)
    
    def setup_bindings(self):
        """设置事件绑定"""
        self.canvas.bind('<ButtonPress-1>', self.on_mouse_down)
        self.canvas.bind('<B1-Motion>', self.on_mouse_move)
        self.canvas.bind('<ButtonRelease-1>', self.on_mouse_up)
        self.canvas.bind('<Control-ButtonPress-1>', self.on_pan_start)
        self.canvas.bind('<Control-B1-Motion>', self.on_pan_move)
        self.canvas.bind('<Control-ButtonRelease-1>', self.on_pan_end)
        self.root.bind('<Delete>', lambda e: self.delete_selected_annotation())
        self.root.bind('<Control-s>', lambda e: self.save_annotations())
        self.root.bind('<Control-o>', lambda e: self.open_image())
    
    def open_image(self):
        """打开图片文件"""
        file_path = filedialog.askopenfilename(
            title="选择图片文件",
            filetypes=[
                ("图片文件", "*.jpg *.jpeg *.png *.bmp *.tiff"),
                ("所有文件", "*.*")
            ]
        )
        
        if not file_path:
            return
        
        try:
            # 加载图片
            self.current_image_path = file_path
            self.image_pil = Image.open(file_path)
            
            # 重置状态
            self.annotations = []
            self.selected_annotation = -1
            self.zoom_factor = 1.0
            self.offset_x = 0
            self.offset_y = 0
            
            # 适应窗口
            self.fit_to_window()
            
            # 更新标注列表
            self.update_annotation_list()
            
            # 更新信息
            self.update_info()
            self.update_status("图片已加载")
            
        except Exception as e:
            messagebox.showerror("错误", f"无法加载图片: {e}")
    
    def fit_to_window(self):
        """调整图片适应窗口"""
        if not self.image_pil:
            return
        
        canvas_width = self.canvas.winfo_width()
        canvas_height = self.canvas.winfo_height()
        
        if canvas_width <= 1 or canvas_height <= 1:
            canvas_width = 800
            canvas_height = 600
        
        img_width, img_height = self.image_pil.size
        
        # 计算缩放比例
        scale_x = canvas_width / img_width
        scale_y = canvas_height / img_height
        self.zoom_factor = min(scale_x, scale_y)
        
        # 计算偏移量使图片居中
        scaled_width = img_width * self.zoom_factor
        scaled_height = img_height * self.zoom_factor
        self.offset_x = (canvas_width - scaled_width) / 2
        self.offset_y = (canvas_height - scaled_height) / 2
        
        self.display_image()
    
    def reset_view(self):
        """重置视图"""
        self.zoom_factor = 1.0
        self.offset_x = 0
        self.offset_y = 0
        self.fit_to_window()
    
    def zoom_in(self):
        """放大"""
        self.zoom_factor *= 1.2
        self.display_image()
    
    def zoom_out(self):
        """缩小"""
        self.zoom_factor /= 1.2
        self.display_image()
    
    def display_image(self):
        """显示图片"""
        if not self.image_pil:
            return
        
        # 缩放图片
        img_width, img_height = self.image_pil.size
        new_width = int(img_width * self.zoom_factor)
        new_height = int(img_height * self.zoom_factor)
        
        scaled_image = self.image_pil.resize((new_width, new_height), Image.Resampling.LANCZOS)
        
        # 转换为PhotoImage
        self.image_tk = ImageTk.PhotoImage(scaled_image)
        
        # 清除画布
        self.canvas.delete("all")
        
        # 显示图片
        self.canvas.create_image(self.offset_x, self.offset_y, anchor=tk.NW, image=self.image_tk)
        
        # 绘制标注
        self.draw_annotations()
    
    def draw_annotations(self):
        """绘制所有标注"""
        if not self.image_pil:
            return
        
        img_width, img_height = self.image_pil.size
        
        for idx, (x1, y1, x2, y2) in enumerate(self.annotations):
            # 转换坐标
            canvas_x1 = x1 * self.zoom_factor + self.offset_x
            canvas_y1 = y1 * self.zoom_factor + self.offset_y
            canvas_x2 = x2 * self.zoom_factor + self.offset_x
            canvas_y2 = y2 * self.zoom_factor + self.offset_y
            
            # 确定颜色
            if idx == self.selected_annotation:
                color = "red"
                width = 3
            else:
                color = "blue"
                width = 2
            
            # 绘制矩形
            self.canvas.create_rectangle(canvas_x1, canvas_y1, canvas_x2, canvas_y2,
                                         outline=color, width=width, tags=f"rect_{idx}")
            
            # 绘制序号
            self.canvas.create_text(canvas_x1 + 5, canvas_y1 + 15,
                                    text=str(idx + 1), anchor=tk.NW,
                                    fill=color, font=("Arial", 10, "bold"),
                                    tags=f"label_{idx}")
    
    def canvas_to_image_coords(self, canvas_x, canvas_y):
        """将画布坐标转换为图片坐标"""
        img_x = (canvas_x - self.offset_x) / self.zoom_factor
        img_y = (canvas_y - self.offset_y) / self.zoom_factor
        return img_x, img_y
    
    def on_mouse_down(self, event):
        """鼠标按下事件"""
        if self.draw_mode_var.get():
            # 绘制模式
            img_x, img_y = self.canvas_to_image_coords(event.x, event.y)
            if 0 <= img_x <= self.image_pil.size[0] and 0 <= img_y <= self.image_pil.size[1]:
                self.drawing = True
                self.start_x = img_x
                self.start_y = img_y
        else:
            # 选择模式 - 查找最近的标注
            self.find_nearest_annotation(event.x, event.y)
    
    def find_nearest_annotation(self, canvas_x, canvas_y):
        """查找最近的标注"""
        if not self.annotations:
            return
        
        min_dist = float('inf')
        nearest_idx = -1
        
        for idx, (x1, y1, x2, y2) in enumerate(self.annotations):
            # 转换坐标
            cx1 = x1 * self.zoom_factor + self.offset_x
            cy1 = y1 * self.zoom_factor + self.offset_y
            cx2 = x2 * self.zoom_factor + self.offset_x
            cy2 = y2 * self.zoom_factor + self.offset_y
            
            # 计算到矩形边界的距离
            if cx1 <= canvas_x <= cx2 and cy1 <= canvas_y <= cy2:
                # 点在矩形内
                dist = 0
            else:
                # 计算到最近边的距离
                dx = max(cx1 - canvas_x, 0, canvas_x - cx2)
                dy = max(cy1 - canvas_y, 0, canvas_y - cy2)
                dist = (dx**2 + dy**2)**0.5
            
            if dist < min_dist:
                min_dist = dist
                nearest_idx = idx
        
        # 如果距离足够近，选中该标注
        if min_dist < 20:  # 20像素阈值
            self.selected_annotation = nearest_idx
            self.annotation_listbox.selection_clear(0, tk.END)
            self.annotation_listbox.selection_set(nearest_idx)
            self.annotation_listbox.see(nearest_idx)
            self.display_image()
            self.update_status(f"选中标注 #{nearest_idx + 1}")
    
    def on_mouse_move(self, event):
        """鼠标移动事件"""
        if self.drawing:
            # 绘制临时矩形
            if self.current_rect:
                self.canvas.delete(self.current_rect)
            
            img_x, img_y = self.canvas_to_image_coords(event.x, event.y)
            canvas_x1 = self.start_x * self.zoom_factor + self.offset_x
            canvas_y1 = self.start_y * self.zoom_factor + self.offset_y
            canvas_x2 = img_x * self.zoom_factor + self.offset_x
            canvas_y2 = img_y * self.zoom_factor + self.offset_y
            
            self.current_rect = self.canvas.create_rectangle(
                canvas_x1, canvas_y1, canvas_x2, canvas_y2,
                outline="green", width=2, dash=(5, 5)
            )
    
    def on_mouse_up(self, event):
        """鼠标释放事件"""
        if self.drawing:
            # 完成绘制
            img_x, img_y = self.canvas_to_image_coords(event.x, event.y)
            
            # 确保坐标正确排序
            x1 = min(self.start_x, img_x)
            y1 = min(self.start_y, img_y)
            x2 = max(self.start_x, img_x)
            y2 = max(self.start_y, img_y)
            
            # 检查矩形大小
            if x2 - x1 > 5 and y2 - y1 > 5:
                # 添加标注（只保存坐标，不保存标签）
                self.annotations.append((x1, y1, x2, y2))
                self.update_annotation_list()
                self.display_image()
                self.update_status(f"已添加标注 #{len(self.annotations)}")
            else:
                self.update_status("标注太小，已忽略")
            
            # 清理绘制状态
            self.drawing = False
            if self.current_rect:
                self.canvas.delete(self.current_rect)
                self.current_rect = None
    
    def on_pan_start(self, event):
        """开始平移"""
        self.panning = True
        self.pan_start_x = event.x
        self.pan_start_y = event.y
    
    def on_pan_move(self, event):
        """平移移动"""
        if self.panning:
            dx = event.x - self.pan_start_x
            dy = event.y - self.pan_start_y
            self.offset_x += dx
            self.offset_y += dy
            self.pan_start_x = event.x
            self.pan_start_y = event.y
            self.display_image()
    
    def on_pan_end(self, event):
        """结束平移"""
        self.panning = False
    
    def update_annotation_list(self):
        """更新标注列表"""
        self.annotation_listbox.delete(0, tk.END)
        for idx, (x1, y1, x2, y2) in enumerate(self.annotations):
            width = x2 - x1
            height = y2 - y1
            list_text = f"{idx + 1}. 矩形 ({width:.0f}x{height:.0f})"
            self.annotation_listbox.insert(tk.END, list_text)
    
    def on_annotation_select(self, event):
        """标注列表选择事件"""
        selection = self.annotation_listbox.curselection()
        if selection:
            self.selected_annotation = selection[0]
            self.display_image()
            self.update_status(f"已选中标注 #{self.selected_annotation + 1}")
    
    def delete_selected_annotation(self):
        """删除选中的标注"""
        if self.selected_annotation >= 0 and self.selected_annotation < len(self.annotations):
            del self.annotations[self.selected_annotation]
            self.selected_annotation = -1
            self.update_annotation_list()
            self.display_image()
            self.update_status("已删除选中标注")
    
    def clear_current_annotation(self):
        """清除当前标注"""
        self.selected_annotation = -1
        self.display_image()
        self.update_status("已清除选中状态")
    
    def clear_all_annotations(self):
        """删除所有标注"""
        if messagebox.askyesno("确认", "确定要删除所有标注吗？"):
            self.annotations = []
            self.selected_annotation = -1
            self.update_annotation_list()
            self.display_image()
            self.update_status("已删除所有标注")
    
    def save_annotations(self):
        """保存标注到文件（仅TXT格式，纯坐标，格式：top right bottom left）"""
        if not self.current_image_path:
            messagebox.showwarning("警告", "请先加载图片")
            return
        
        if not self.annotations:
            if not messagebox.askyesno("确认", "没有标注要保存，是否继续？"):
                return
        
        try:
            # 获取图像尺寸
            img_width, img_height = self.image_pil.size
            
            # 生成保存路径：图片文件名.txt（同目录下）
            image_dir = os.path.dirname(self.current_image_path)
            image_basename = os.path.splitext(os.path.basename(self.current_image_path))[0]
            file_path = os.path.join(image_dir, f"{image_basename}.txt")
            
            # 保存标注（纯坐标格式，top right bottom left）
            with open(file_path, 'w', encoding='utf-8') as f:
                for x1, y1, x2, y2 in self.annotations:
                    # 转换为百分比坐标（top, right, bottom, left）
                    top_pct = y1 / img_height
                    right_pct = x2 / img_width
                    bottom_pct = y2 / img_height
                    left_pct = x1 / img_width
                    
                    # 写入坐标数据（保留6位小数，空格分隔）
                    f.write(f"{top_pct:.6f} {right_pct:.6f} {bottom_pct:.6f} {left_pct:.6f}\n")
            
            self.update_status(f"已保存 {len(self.annotations)} 个标注到 {os.path.basename(file_path)}")
            messagebox.showinfo("成功", f"标注已保存到:\n{file_path}\n\n格式: top right bottom left (百分比坐标)")
            
        except Exception as e:
            messagebox.showerror("错误", f"保存失败: {e}")
    
    def load_annotations(self):
        """加载标注文件（TXT格式，纯坐标，格式：top right bottom left）"""
        if not self.current_image_path:
            messagebox.showwarning("警告", "请先加载图片")
            return
        
        # 默认加载同目录下的同名txt文件
        image_dir = os.path.dirname(self.current_image_path)
        image_basename = os.path.splitext(os.path.basename(self.current_image_path))[0]
        default_file = os.path.join(image_dir, f"{image_basename}.txt")
        
        if os.path.exists(default_file):
            file_path = default_file
        else:
            file_path = filedialog.askopenfilename(
                title="选择标注文件",
                initialdir=image_dir,
                filetypes=[
                    ("文本文件", "*.txt"),
                    ("所有文件", "*.*")
                ]
            )
            
            if not file_path:
                return
        
        try:
            self.load_txt_annotations(file_path)
            self.update_annotation_list()
            self.display_image()
            self.update_status(f"已加载 {len(self.annotations)} 个标注")
            
        except Exception as e:
            messagebox.showerror("错误", f"加载失败: {e}")
    
    def load_txt_annotations(self, file_path):
        """加载TXT格式的标注文件（纯坐标格式，top right bottom left）"""
        self.annotations = []
        img_width, img_height = self.image_pil.size
        
        with open(file_path, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue
                
                parts = line.split()
                if len(parts) == 4:
                    try:
                        top_pct = float(parts[0])
                        right_pct = float(parts[1])
                        bottom_pct = float(parts[2])
                        left_pct = float(parts[3])
                        
                        # 验证百分比坐标范围
                        if all(0 <= v <= 1 for v in [top_pct, right_pct, bottom_pct, left_pct]):
                            # 转换为绝对坐标
                            x1 = left_pct * img_width
                            y1 = top_pct * img_height
                            x2 = right_pct * img_width
                            y2 = bottom_pct * img_height
                            
                            self.annotations.append((x1, y1, x2, y2))
                        else:
                            print(f"警告: 第{line_num}行坐标超出范围 [0,1]: {line}")
                    except ValueError as e:
                        print(f"警告: 第{line_num}行格式错误: {line}, 错误: {e}")
                else:
                    print(f"警告: 第{line_num}行格式错误，需要4个坐标值: {line}")
    
    def update_info(self):
        """更新信息显示"""
        if self.current_image_path and self.image_pil:
            img_width, img_height = self.image_pil.size
            filename = os.path.basename(self.current_image_path)
            self.info_label.config(text=f"图片: {filename}\n尺寸: {img_width}x{img_height}\n标注: {len(self.annotations)}个")
        else:
            self.info_label.config(text="未加载图片")
    
    def update_status(self, message=None):
        """更新状态栏"""
        if message:
            self.status_label.config(text=f"状态: {message}")
        else:
            self.status_label.config(text="状态: 就绪")
    
    def update(self):
        """更新界面"""
        self.update_info()
        self.root.after(1000, self.update)


def main():
    """主函数"""
    root = tk.Tk()
    app = ImageAnnotationTool(root)
    app.update()
    root.mainloop()


if __name__ == "__main__":
    main()