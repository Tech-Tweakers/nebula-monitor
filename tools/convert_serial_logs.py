#!/usr/bin/env python3
"""
Script to convert direct Serial calls to silent mode compatible versions
"""

import os
import re
import glob

def convert_serial_calls(file_path):
    """Convert Serial.print calls to silent mode compatible versions"""
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    
    # Add logger include if not present
    if '#include "core/infrastructure/logger/logger.h"' not in content:
        # Find the last #include line
        lines = content.split('\n')
        last_include = -1
        for i, line in enumerate(lines):
            if line.strip().startswith('#include'):
                last_include = i
        
        if last_include != -1:
            lines.insert(last_include + 1, '#include "core/infrastructure/logger/logger.h"')
            content = '\n'.join(lines)
    
    # Replace Serial.print with Serial_print
    content = re.sub(r'\bSerial\.print\b', 'Serial_print', content)
    content = re.sub(r'\bSerial\.println\b', 'Serial_println', content)
    content = re.sub(r'\bSerial\.printf\b', 'Serial_printf', content)
    
    # Only write if changes were made
    if content != original_content:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Converted: {file_path}")
        return True
    else:
        print(f"No changes: {file_path}")
        return False

def main():
    """Convert all .cpp files in src directory"""
    
    # Find all .cpp files in src directory
    cpp_files = glob.glob('src/**/*.cpp', recursive=True)
    
    converted_count = 0
    
    for file_path in cpp_files:
        if convert_serial_calls(file_path):
            converted_count += 1
    
    print(f"\nConversion complete! {converted_count} files converted.")

if __name__ == "__main__":
    main()
