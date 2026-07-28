# STM32 Bootloader DEMO

这是一个基于 STM32 HAL 库开发的简易 Bootloader DEMO。

本项目主要用于学习嵌入式系统中 Bootloader 的基本工作流程，包括：

- 固件下载
- Flash 擦除与写入
- Application（以下简称 App） 程序跳转

---


## ✨ 功能特点

- **固件可靠下载**
  - 支持 **USART + DMA + 双缓冲区（Double Buffer）** 接收机制，实现串口数据接收与 Flash 编程操作并行处理，提高固件升级过程中的数据传输效率和稳定性
  - 基于 **YMODEM** 协议完成用户固件传输

- **Flash 擦写与编程**
  - 支持 Flash 多扇区擦除与数据写入

- **App 程序合法性检查**
  - MSP 栈顶地址合法性检查
  - Reset_Handler 入口地址检查
  - Thumb 状态标志位检查

- **安全跳转与异常提示**
  - 支持安全跳转至用户 App 运行
  - 支持 App 向量表重定位（VTOR）
  - 若 App 校验失败，则进入异常提示流程（红灯闪烁）

- **模块化设计**
  - 基于 STM32 HAL 库实现，Bootloader 核心逻辑与 HAL 初始化代码分离

---


## 🛠️ 硬件与环境
- **MCU**: STM32F407ZGT6
- **开发工具**: Keil MDK，STM32CubeMX
- **HAL VERSION**: STM32Cube_FW_F4_V1.28.3

## 📂 目录结构
* `Core/`: STM32CubeMX 生成的外设初始化代码以及main.c
* `Drivers/`: STM32 HAL 驱动文件
* `User/`: Bootloader 核心跳转与协议逻辑


### 🏗️ Flash 地址规划

| 区域 | 起始地址 | 终止地址 | 默认大小 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| **Bootloader** | `0x08000000` | `0x08007FFF` | 32 KB | 引导加载程序，负责固件升级与程序跳转 |
| **App** | `0x08008000` | `0x080FFFFF` | 992 KB | 主应用程序（App）运行区域 |

## 🛠️ 使用方法

### 1. 串口通信配置
* **波特率**: `115200`
* **数据位**: `8`
* **停止位**: `1`
* **校验位**: `无 (None)`
* **流控制**: `无 (None)`

### 2. App工程调整
App 工程需要将 Flash 起始地址配置为 0x08008000，并同步修改中断向量表偏移。如图所示:
<img width="300" height="300" alt="2026-07-28_181112" src="https://github.com/user-attachments/assets/33e9fb19-1554-4007-ae57-d4a25b21cc90" /><img width="300" height="300" alt="图片" src="https://github.com/user-attachments/assets/4b814f7b-5374-43ba-9ee1-542eac183b74" />

### 2. Bootloader 触发逻辑
1. 将板子**上电**或按下**复位键**。
2. 上电/复位后，**长按 按键 2 秒以上**（按键IO口为`PA0` ，高电平有效）。
3. 触发检测成功后，系统将进入 **Bootloader 主菜单**，可通过串口终端（如 SecureCRT / Tera Term）进行交互。
4. 若上电未检测到按键按下（或按键时长不足），系统将默认直接跳转并运行 `0x08008000` 地址处的 **App** 主程序。
