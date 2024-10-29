# risc8asm

Unofficial RISC8B Assembler for WCH PIOC eMCU

[English Version](#english) | [中文版本](#chinese)

<a name="chinese"></a>
## 中文版本

### 简介

risc8asm是一个非官方的WCH PIOC eMCU RISC8B汇编器. 因半仙看到GitHub上开发者在Linux环境下使用Wine运行官方汇编器的不便, 以及在阅读RISC8B文档时遇到的一些疑惑.    
在学习指令集的过程中, 重写了该版本.

PIOC eMCU使用RISC8B指令集, 具有以下特点:
- 2K条指令的程序空间 (ROM)
- 16位指令宽度
- 程序计数器(PC)每个地址包含一条指令

本汇编器通过参考官方文档和工具的列表文件格式重新实现, 旨在提供更便捷的开发体验.

### 编译方法

```bash
gcc -O3 -Wno-multichar -o asm53b asm53b.c
```

可选使用内置listprintf减少列表文件写入耗时：
```bash
gcc -O3 -DLITE_PRINTF -Wno-multichar -o asm53b asm53b.c
```

### 使用方法

```bash
asm53b asmfile [-f]
```

- `asmfile`: 汇编源文件, 如省略后缀名, 会自动添加`.ASM`
- `-f`: 可选参数, 生成完整ROM大小的二进制文件, 未使用部分填充NOP指令

### 主要改进

相比官方wasm53b汇编器：
1. ✓ 修复EQU标记解析时错误跳过\r符号的问题
2. ✓ 修复INCLUDE指令解析路径 (现在相对于包含他的文件目录)
3. ✓ 生成的BIN和LST文件默认输出到工作目录
4. ✓ 增加了高速的写列表文件函数
5. ✓ 加速了符号表查询

### TODO

#### 已修复
- [x] EQU标记解析
- [x] INCLUDE路径解析


#### 待增强
- [ ] 输出为C数组格式
- [ ] 去除标号数量限制
- [ ] 更详细的错误信息
- [ ] 支持条件汇编
- [ ] 支持宏定义
- [ ] 添加单元测试
- [ ] 支持Unicode源文件

### 参考资料
- WCH PIOC eMCU手册
- RISC8B指令集文档

---

<a name="english"></a>
## English Version

### Introduction

risc8asm is an unofficial assembler for WCH PIOC eMCU implementing the RISC8B instruction set. The project was initiated when I noticed developers struggling with Wine to run the official assembler on Linux, along with some confusion around the RISC8B documentation.  
This version was rewritten while studying the instruction set.

The PIOC eMCU utilizes the RISC8B instruction set with:
- 2K instructions of program space (ROM)
- 16-bit instruction width
- One instruction per program counter (PC) address

This assembler has been implemented by referencing the official documentation and list file format of the original tool, aiming to provide a more accessible development experience.

### Compilation

```bash
gcc -O3 -Wno-multichar -o asm53b asm53b.c
```

Optional built-in listprintf to reduce list file writing time:
```bash
gcc -O3 -DLITE_PRINTF -Wno-multichar -o asm53b asm53b.c
```

### Usage

```bash
asm53b asmfile [-f]
```

- `asmfile`: Assembly source file, `.ASM` extension will be added if omitted
- `-f`: Optional: generate full ROM-sized binary with unused space filled with NOP

### Improvements

Compared to the official wasm53b assembler:
1. ✓ Fixed EQU token parsing issue with \r character skipping
2. ✓ Fixed INCLUDE directive path resolution (now relative to including file's directory)
3. ✓ Generated files (BIN/LST) are placed in working directory
4. ✓ Added high-performance list file writing function
5. ✓ Accelerated symbol table lookup

### TODO

#### Fixed
- [x] EQU token parsing
- [x] INCLUDE path resolution

#### Enhancements
- [ ] C array output format
- [ ] Remove Label/EQU count limitation
- [ ] Enhanced error messages
- [ ] Conditional assembly support
- [ ] Macro definitions
- [ ] Unit testing
- [ ] Unicode source file support

### References
- WCH PIOC eMCU Manual
- RISC8B Instruction Set Documentation