"""
动态特征码配置文件
可以在这里自定义签名生成的参数
"""

# 签名生成配置
SIGNATURE_CONFIG = {
    # 随机填充数据的大小（字节）
    'padding_size': 256,
    
    # 签名哈希长度（必须 <= 64）
    'hash_length': 16,
    
    # 是否启用动态特征码
    'enabled': True,
    
    # 签名类型：'hash'、'timestamp'、'random'、'combined'
    'signature_type': 'combined',
    
    # 是否注入随机填充
    'inject_padding': True,
    
    # 是否生成C++接口函数
    'generate_cpp_interface': True,
    
    # 日志级别：'debug', 'info', 'warning', 'error'
    'log_level': 'info',
}

# 生成的文件配置
OUTPUT_CONFIG = {
    # 头文件输出目录（相对于项目根目录）
    'header_dir': 'Source/BuildSignature',
    
    # 头文件名称
    'header_file': 'BuildSignature.h',
    
    # C++源文件名称
    'cpp_file': 'RandomPadding.h',
    
    # 是否创建.cpp源文件（而不仅是头文件）
    'create_cpp_source': False,
}

# 编译目标配置
BUILD_CONFIG = {
    # 目标平台
    'platform': 'x64',
    
    # 构建配置
    'configuration': 'Release',
    
    # 编译命令
    'msbuild_path': 'msbuild',
    
    # 项目文件
    'project_file': 'Osiris.sln',
}
