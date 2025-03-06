# Board: AlifSemiconductor AppKit

## Board Layer for M55 High Performance Core

Device: AE722F80F55D5LS:M55_HP

This setup is configured using **Conductor Tool**, an interactive tool provided by Alif Semiconductor for device configuration.
Refer to ["Alif Conductor Tool Manual"](https://conductor.alifsemi.com/Alif_HTML_DCT_User_Help/Content/Help%20Manual.htm) for additional information.

### System Configuration

| System Component        | Setting
|:------------------------|:----------------------------------------
| Heap                    | 32 kB (configured in M55_HP linker file)
| Stack (MSP)             |  8 kB (configured in M55_HP linker file)

### STDIO mapping

**STDIO** is routed to Virtual COM port via **UART4** peripheral

### Virtual Streaming Interfaces

| Interface | Peripheral | Board connector/component | Connection
|:----------|:-----------|:--------------------------|:-----------
| Audio     | SAI3       | I2S Microphone (U13, U15) | AVH_AUDIO
| Video     | MIPI DCI   | MIPI Camera (J10, J12)    | AVH_VIDEO

### CMSIS-Driver mapping

| CMSIS-Driver        | Peripheral | Board connector/component  | Connection
|:--------------------|:-----------|:---------------------------|:---------------------
| Driver_USART4       | UART4      | PRG USB connector (J3)     | STDIN, STDOUT, STDERR
| CMSIS-Driver VIO    | GPIO       | Buttons, RGB LED, Joystick | CMSIS_VIO

### CMSIS-Driver Virtual I/O mapping

| CMSIS-Driver VIO  | Board component
|:------------------|:----------------------------
|vioBUTTON0         | SW4 Button
|vioBUTTON1         | SW5 Button
|vioJOYup           | Joystick Up
|vioJOYdown         | Joystick Down
|vioJOYleft         | Joystick Left
|vioJOYright        | Joystick Right
|vioJOYselect       | Joystick Select Button
|vioLED0            | RGB LED Red
|vioLED1            | RGB LED Green
|vioLED2            | RGB LED Blue
