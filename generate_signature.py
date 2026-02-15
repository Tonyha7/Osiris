#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
动态特征码生成脚本
每次编译时生成唯一的特征码，确保每个DLL都有不同的标识
"""

import os
import sys
import hashlib
import time
import random
import datetime

try:
    from signature_config import SIGNATURE_CONFIG, OUTPUT_CONFIG, BUILD_CONFIG
except ImportError:
    # 使用默认配置
    SIGNATURE_CONFIG = {
        'padding_size': 256,
        'hash_length': 16,
        'enabled': True,
        'signature_type': 'combined',
        'inject_padding': True,
        'generate_cpp_interface': True,
        'log_level': 'info',
    }
    OUTPUT_CONFIG = {
        'header_dir': 'Source/BuildSignature',
        'header_file': 'BuildSignature.h',
        'cpp_file': 'RandomPadding.h',
        'create_cpp_source': False,
    }
    BUILD_CONFIG = {
        'platform': 'x64',
        'configuration': 'Release',
        'msbuild_path': 'msbuild',
        'project_file': 'Osiris.sln',
    }

def generate_dynamic_signature():
    """生成动态特征码"""
    timestamp = int(time.time() * 1000)  # 毫秒级时间戳
    random_value = random.randint(100000, 999999)
    
    # 组合生成签名
    signature_source = f"{timestamp}_{random_value}_{datetime.datetime.now().isoformat()}"
    hash_length = min(SIGNATURE_CONFIG.get('hash_length', 16), 64)
    signature_hash = hashlib.sha256(signature_source.encode()).hexdigest()[:hash_length].upper()
    
    return {
        'timestamp': timestamp,
        'random_value': random_value,
        'hash': signature_hash,
        'datetime': datetime.datetime.now().isoformat()
    }

def create_signature_header(output_path, signature):
    """创建包含动态特征码的头文件"""
    header_content = f"""#pragma once

/**
 * 动态编译特征码
 * 每次编译时自动生成，确保每个DLL都有唯一标识
 */

#define OSIRIS_SIGNATURE_TIMESTAMP {signature['timestamp']}L
#define OSIRIS_SIGNATURE_RANDOM {signature['random_value']}
#define OSIRIS_SIGNATURE_HASH "{signature['hash']}"
#define OSIRIS_SIGNATURE_DATETIME "{signature['datetime']}"

// 组合签名值
#define OSIRIS_BUILD_SIGNATURE "{signature['hash']}_{signature['timestamp']}"

namespace osiris {{
    constexpr const char* BUILD_SIGNATURE = OSIRIS_SIGNATURE_HASH;
    constexpr long long BUILD_TIMESTAMP = OSIRIS_SIGNATURE_TIMESTAMP;
    constexpr int BUILD_RANDOM = OSIRIS_SIGNATURE_RANDOM;
}}
"""
    
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    return True

def create_signature_cpp(output_path, signature):
    """创建包含动态特征码的C++源文件"""
    cpp_content = f"""/**
 * 动态编译特征码源文件
 * 生成时间: {signature['datetime']}
 */

namespace osiris {{
    // 编译特征码
    const char* g_build_signature = OSIRIS_SIGNATURE_HASH;
    const long long g_build_timestamp = {signature['timestamp']}LL;
    const int g_build_random = {signature['random_value']};
    
    // 获取编译签名信息
    const char* get_build_signature() {{
        return g_build_signature;
    }}
    
    long long get_build_timestamp() {{
        return g_build_timestamp;
    }}
    
    int get_build_random() {{
        return g_build_random;
    }}
}}
"""
    
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(cpp_content)
    
    return True

def inject_random_padding(output_path, size=None):
    """注入随机填充数据，增加DLL差异性"""
    if size is None:
        size = SIGNATURE_CONFIG.get('padding_size', 256)
    
    random_data = bytearray(random.getrandbits(8) for _ in range(size))
    
    header_content = f"""#pragma once

/**
 * 随机填充数据
 * 用于增加编译输出的差异性
 */

// 随机填充常量
constexpr unsigned char RANDOM_PADDING[{size}] = {{
"""
    
    # 配置字节
    bytes_per_line = 16
    for i in range(0, len(random_data), bytes_per_line):
        chunk = random_data[i:i+bytes_per_line]
        hex_values = ', '.join(f'0x{b:02x}' for b in chunk)
        header_content += f"    {hex_values},\n"
    
    header_content = header_content.rstrip(',\n') + "\n};\n"
    
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    return True

def main():
    """主函数"""
    # 检查是否启用了动态特征码
    if not SIGNATURE_CONFIG.get('enabled', True):
        print("[*] 动态特征码功能已禁用")
        return 0
    
    # 获取脚本所在目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 生成目录路径
    source_dir = os.path.join(script_dir, 'Source')
    signature_dir = os.path.join(source_dir, OUTPUT_CONFIG.get('header_dir', 'BuildSignature').split('/')[-1])
    
    print("[*] 生成动态特征码...")
    
    # 生成签名
    signature = generate_dynamic_signature()
    
    print(f"    [+] 签名Hash: {signature['hash']}")
    print(f"    [+] 时间戳: {signature['timestamp']}")
    print(f"    [+] 随机值: {signature['random_value']}")
    print(f"    [+] 日期时间: {signature['datetime']}")
    
    # 创建签名头文件
    header_filename = OUTPUT_CONFIG.get('header_file', 'BuildSignature.h')
    signature_h_path = os.path.join(signature_dir, header_filename)
    if create_signature_header(signature_h_path, signature):
        print(f"[+] 创建签名头文件: {signature_h_path}")
    
    # 创建随机填充文件
    if SIGNATURE_CONFIG.get('inject_padding', True):
        padding_filename = OUTPUT_CONFIG.get('cpp_file', 'RandomPadding.h')
        padding_h_path = os.path.join(signature_dir, padding_filename)
        if inject_random_padding(padding_h_path):
            print(f"[+] 创建随机填充文件: {padding_h_path}")
    
    print("[*] 动态特征码注入完成！")
    print(f"[*] 每次编译将生成不同的DLL文件特征码\n")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
