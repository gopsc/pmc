#!/usr/bin/env python3
"""
扩展技能：人脸位置检测工具
用于检测图像中的人脸并保存人脸位置坐标（百分比格式）到txt文件
放置在 ~/.aibox/skills/face_position_detector.py
"""

import os
import sys
import json
import argparse
import cv2
import face_recognition
import numpy as np
from datetime import datetime
from typing import List, Optional


class FacePositionDetector:
    """人脸位置检测工具（检测位置坐标并转换为百分比）"""
    
    def __init__(self):
        # 不再使用.aibox文件夹，使用当前目录
        pass
    
    def _ensure_dirs(self, dir_path: str):
        """确保目录存在"""
        os.makedirs(dir_path, exist_ok=True)
    
    def detect_faces(self, image_path: str, save_coords: bool = True, output_file: Optional[str] = None) -> str:
        """
        检测图像中的人脸并保存位置坐标（百分比格式）到txt文件
        """
        try:
            if not os.path.exists(image_path):
                return f"❌ 错误：文件不存在 - {image_path}"
            
            # 使用OpenCV获取图像尺寸
            img_cv = cv2.imread(image_path)
            if img_cv is None:
                return f"❌ 错误：无法读取图像文件 - {image_path}"
            
            img_height, img_width = img_cv.shape[:2]
            
            # 使用face_recognition检测人脸
            image = face_recognition.load_image_file(image_path)
            face_locations = face_recognition.face_locations(image)
            
            if not face_locations:
                return "📭 未检测到人脸"
            
            results = []
            saved_coords = []
            
            for i, (top, right, bottom, left) in enumerate(face_locations):
                # 转换为百分比坐标
                top_pct = top / img_height
                right_pct = right / img_width
                bottom_pct = bottom / img_height
                left_pct = left / img_width
                
                face_data = {
                    "index": i + 1,
                    "position_absolute": {"top": top, "right": right, "bottom": bottom, "left": left},
                    "position_percentage": {"top": top_pct, "right": right_pct, "bottom": bottom_pct, "left": left_pct},
                    "size_absolute": {"width": right - left, "height": bottom - top},
                    "size_percentage": {"width": (right - left) / img_width, "height": (bottom - top) / img_height},
                    "image_dimensions": {"width": img_width, "height": img_height}
                }
                
                # 保存人脸坐标到txt文件（百分比格式）
                if save_coords:
                    try:
                        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                        
                        # 确定输出文件名
                        if output_file:
                            coords_filename = output_file
                        else:
                            base_name = os.path.splitext(os.path.basename(image_path))[0]
                            coords_filename = f"{base_name}_faces_{timestamp}.txt"
                        
                        # 使用当前目录作为保存路径
                        coords_filepath = coords_filename
                        
                        # 写入百分比坐标数据（保留6位小数）
                        with open(coords_filepath, 'a') as f:
                            # 格式：top right bottom left (百分比，空格分隔)
                            f.write(f"{top_pct:.6f} {right_pct:.6f} {bottom_pct:.6f} {left_pct:.6f}\n")
                        
                        saved_coords.append(coords_filename)
                        
                        face_data["coords_file"] = coords_filename
                        face_data["coords_path"] = coords_filepath
                    except Exception as e:
                        face_data["save_error"] = str(e)
                
                results.append(face_data)
            
            # 构建结果字符串
            result_str = f"✅ 检测到 {len(results)} 张人脸\n"
            result_str += f"📏 图像尺寸: {img_width}x{img_height}\n\n"
            
            for face in results:
                result_str += f"👤 人脸 #{face['index']}:\n"
                result_str += f"  绝对坐标: top={face['position_absolute']['top']}, right={face['position_absolute']['right']}, "
                result_str += f"bottom={face['position_absolute']['bottom']}, left={face['position_absolute']['left']}\n"
                result_str += f"  百分比坐标: top={face['position_percentage']['top']:.3f}, right={face['position_percentage']['right']:.3f}, "
                result_str += f"bottom={face['position_percentage']['bottom']:.3f}, left={face['position_percentage']['left']:.3f}\n"
                result_str += f"  绝对尺寸: {face['size_absolute']['width']}x{face['size_absolute']['height']}\n"
                result_str += f"  相对尺寸: {face['size_percentage']['width']:.3f}x{face['size_percentage']['height']:.3f}"
                
                if 'coords_file' in face:
                    result_str += f"\n  坐标已保存到: {face['coords_file']}"
                
                result_str += "\n\n"
            
            if saved_coords:
                # 显示所有保存的坐标文件
                unique_files = list(set(saved_coords))
                result_str += f"📂 人脸坐标（百分比格式）已保存到当前目录:\n"
                for filename in unique_files:
                    result_str += f"  📄 {filename}\n"
            
            return result_str
            
        except Exception as e:
            return f"❌ 检测失败: {e}"


def main():
    """简化后的主函数 - 仅人脸位置检测（百分比坐标）"""
    parser = argparse.ArgumentParser(description="人脸位置检测工具（检测坐标并转换为百分比）")
    subparsers = parser.add_subparsers(dest="command", help="可用命令")
    
    # detect命令
    detect_parser = subparsers.add_parser("detect", help="检测图像中的人脸并保存百分比坐标")
    detect_parser.add_argument("image_path", help="图片文件路径")
    detect_parser.add_argument("--no-save", action="store_true", help="不保存坐标")
    detect_parser.add_argument("--output", help="指定输出文件名（默认自动生成）")
    
    # 工具描述命令
    desc_parser = subparsers.add_parser("description", help="显示工具描述")
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        sys.exit(1)
    
    detector = FacePositionDetector()
    
    if args.command == "description":
        desc = """人脸位置检测工具（百分比坐标）

功能：
- 检测图像中的人脸
- 将人脸位置坐标转换为百分比格式
- 保存百分比坐标到txt文件（格式：top right bottom left，均为0-1之间的浮点数）

坐标说明：
- top: 人脸顶部位置（占图像高度的比例，0=顶部，1=底部）
- right: 人脸右侧位置（占图像宽度的比例，0=左侧，1=右侧）
- bottom: 人脸底部位置（占图像高度的比例，0=顶部，1=底部）
- left: 人脸左侧位置（占图像宽度的比例，0=左侧，1=右侧）

目录结构：
- 人脸坐标保存位置: 当前目录

使用示例：
  facerec.py detect image.jpg              # 检测人脸并保存百分比坐标到当前目录
  facerec.py detect image.jpg --no-save    # 检测但不保存坐标
  facerec.py detect image.jpg --output my_coords.txt  # 指定输出文件名"""
        print(desc)
    
    elif args.command == "detect":
        save_coords = not args.no_save
        result = detector.detect_faces(args.image_path, save_coords, args.output)
        print(result)


if __name__ == "__main__":
    main()