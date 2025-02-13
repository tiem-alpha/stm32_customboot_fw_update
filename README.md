- [STM32F103 Firmware Update via UART with custom bootloader](#stm32f103-firmware-update-via-uart-with-custom-bootloader)

  - [Overview](#overview)
  - [Hardware prerequisites](#hardware-prerequisites)
  - [Software prerequisites](#software-prerequisites)
  - [Hardware setup](#hardware-setup)
  - [Software setup](#software-setup)
    - [Step 1: Download and install VSCode](#step-1-download-and-install-vscode)
    - [Step 2: Install STM32 extentions on VSCode](#step-2-install-stm32-extentions-on-vscode)
    - [Step 3: Download and install python ](#step-3-download-and-install-python)
    - [Step 4: Open Log console ](#step-4-open-log-console)
    - [Step 5: Build and flash bootloader for STM32 ](#step-5-build-and-flash-bootloader-for-stm32)
    - [Step 6: Build Application image and run python tool on PC ](#step-6-build-application-image-and-run-python-tool-on-pc)
    - [Step 7: Trigger the STM32 into bootloader mode by pressing the reset button ](#step-7-trigger-the-stm32-into-bootloader-mode-by-pressing-the-reset-button)reset button
  - [Update FW Process](#update-fw-process)

  

# STM32F103 Firmware Update via UART with custom bootloader


## Overview
This project will perform firmware updates for the STM32 via UART1 using a custom bootloader. In the project, UART1 is enabled for firmware updates, and UART2 is used for logging.

## Hardware prerequisites

1. STM32f103 kit
2. ST Link v2 
3. Female to female connecting jumper wires
6. USB to TTL module CP2102 (02 Unit)

## Software prerequisites

1. VSCode
2. MobaXTerm
3. STM32 extentions with VSCode
4. Python 3.12.0

## Hardware setup

Follow the below picture

<p align="center" style="margin-left:0em;">
<img src="./Img/HW_Setup.png" width="60%" height="60%" alt="HW_setup.png">
</p>



## Software setup

### Step 1: Download and install VSCode 

https://code.visualstudio.com/

### Step 2: Install STM32 extentions on VSCode

Please follow this ULR for more detail:</br>

https://www.youtube.com/watch?v=aLD9zggmop4

### Step 3: Download and install python 

- https://www.python.org/downloads/
- Set up environment variable
- Install requirements: open CMD and type pip install pyserial to install serial lib for python 

### Step 4: Open Log console

Open MobaXterm and start a COM session corresponding to the USB-TTL port connected to UART2.

<p align="center" style="margin-left:0em;">
<img src="./Img/flash_bootloader.png" width="60%" height="60%" alt="HW_setup.png">
</p>

- 1 Create new Session
- 2 Chose Serial session
- 3 Chose corresponding COM port connect to UART2
- 4 Select baudrate 115200


### Step 5: Build and flash bootloader for STM32

- Open STM32 Programmer

<p align="center" style="margin-left:0em;">
<img src="./Img/flash_bootloader.png" width="60%" height="60%" alt="HW_setup.png">
</p>

- Click Download to flash bootloader
- Click reset button on STM32 kit , on Moba Xterm console will be log out "Start boot loader 0.1.1"

### Step 6: Build Application image and run python tool on PC

Follow Application guide to generate Application.bin file
Copy Application.bin to python tools folder

Follow Python tool guide to start FW updater host

### Step 7: Trigger the STM32 into bootloader mode by pressing the reset button

## Update FW Process


1. Reset
The device (e.g., microcontroller) is reset.
2. Send Firmware Update Request
The bootloader sends a 1-byte value 0xAC to "wake up" the host (e.g., computer).
The bootloader waits for a response message from the host within a timeout period.
The response message has the following structure:
0xAC (start byte) + length of package + 0x01 (command NEW_FW) + new FW major + new FW minor + new FW build + high byte of new FW size + low byte of new FW size + high byte of CRC16-CCITT + low byte of CRC16-CCITT
*   `length of package`: Length of the data part (from command NEW\_FW to CRC16-CCITT)
*   The new FW size is represented by 2 bytes (uint16\_t).
*   CRC16-CCITT is calculated on `length of package` and the data part.
3. Handle Response Message
Timeout: If the bootloader does not receive a response message within the timeout period, it will retry step 2. This process repeats up to 5 times. If it still fails, the bootloader will jump to the app (if available).
Error Message Received: If the bootloader receives a response message but it contains an error (e.g., invalid format), it will also retry step 2 (up to 5 times).
Success Message Received: If the bootloader receives a valid response message, it will send an ACK byte (0x55) to confirm with the host.
4. Firmware Transfer
The host sends the new firmware data in packages.
Each package has the following structure:
0xAC (start byte) + length of package + data + CRC16-CCITT[2]
*   `length of package`: Length of the data part.
*   `data`: Firmware data (a portion of the new firmware).
*   `CRC16-CCITT[2]`: 2 bytes of CRC16-CCITT calculated on `length of package` and `data`.
The bootloader checks the validity of each package (e.g., checks CRC). If the package is valid, it will write the data to flash memory. If it is invalid, the bootloader will report an error and jump to the app.
5. Completion and Jump to App
After receiving all the firmware data, the bootloader will jump to the app to run the new application.

