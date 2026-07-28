# STM32 BootLoader DEMO

这是一个基于 STM32 HAL 库开发的简易 Bootloader DEMO。

本项目主要用于学习嵌入式系统中 Bootloader 的基本工作流程，包括：

- 固件下载
- Flash 擦除与写入
- Application（以下简称 App） 程序跳转
- 中断向量表重定向
- Bootloader 与 App 的地址分离

---

## 📌 功能特点

- 支持 USART + YMODEM 协议下载用户固件
- 支持 Flash 擦除与固件写入
- 支持 App 程序合法性检查
  - MSP 栈顶地址检查
  - Reset_Handler 地址检查
  - Thumb 状态检查
- 支持 Bootloader 跳转至 App
- 支持 APP 向量表重定位
- 基于 STM32 HAL 库实现

---


## 🛠️ 硬件与环境
* **MCU**: STM32F407ZGT6
- **IDE**: Keil MDK
- **Configuration Tool**: STM32CubeMX
- **MCU Framework**: STM32Cube_FW_F4_V1.28.3

## 📂 目录结构
* `Core/`: STM32CUBEMX生成的外设初始化代码以及main.c
* `Drivers/`: STM32 HAL 驱动文件
* `User/`: BootLoader 核心跳转与协议逻辑


### 🏗️ Flash 地址规划

| 区域 | 起始地址 | 终止地址 | 默认大小 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| **Bootloader** | `0x08000000` | `0x08007FFF` | 32 KB | 引导加载程序，负责固件升级与程序跳转 |
| **App** | `0x08008000` | `0x080FFFFF` | 992 KB | 主应用程序（App）运行区域 |

## 🛠️ 使用方法与配置

### 1. 串口通信配置
* **波特率**: `115200`
* **数据位**: `8`
* **停止位**: `1`
* **校验位**: `无 (None)`
* **流控制**: `无 (None)`

### 2. App工程调整
App 工程需要将 Flash 起始地址配置为 0x08008000，并同步修改中断向量表偏移。如图所示:

### 2. Bootloader 触发逻辑
1. 将板子**上电**或按下**复位键**。
2. 上电/复位后，**长按 按键 2 秒以上**（按键IO口为`PA0` ，高电平有效）。
3. 触发检测成功后，系统将进入 **Bootloader 主菜单**，可通过串口终端（如 SecureCRT / Tera Term）进行交互。
4. 若上电未检测到按键按下（或按键时长不足），系统将默认直接跳转并运行 `0x08008000` 地址处的 **App** 主程序。
