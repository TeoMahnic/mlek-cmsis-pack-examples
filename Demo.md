# Out of the box demos for Alif Ensemble boards

Build and run the **Keyword spotting** and **Object detection** examples on Alif **Ensemble E7 AI/ML AppKit** and **Ensemble E7 DevKit** boards.

## Prerequisites

- [Visual Studio Code IDE](https://code.visualstudio.com/) with
    - [Arm Keil Studio Pack Extension](https://marketplace.visualstudio.com/items?itemName=Arm.keil-studio-pack)
    - [Arm CMSIS Debugger Extension](https://github.com/Open-CMSIS-Pack/vscode-cmsis-debugger/releases) (VSIX installation)

- CMSIS Packs
    - [Published Packs](https://www.keil.arm.com/packs/) specified in the example solution and project files
    - [Alif Ensemble DFP](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp)
        - clone the repo: execute `git clone https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp.git`
        - build the pack: execute `gen_pack.sh`
        - install the repo: execute `cpackget add output/AlifSemiconductor.Ensemble.<version>.pack`
        - edit the pack .pdsc file: remove the meta data from the version (needs to match the folder name)
    - [Alif Ensemble AppKit BSP](https://github.com/VladimirUmek/alif_ensemble_appkit-bsp)
        - clone the repo: execute `git clone https://github.com/VladimirUmek/alif_ensemble_appkit-bsp.git`
        - install the repo: execute `cpackget add cpackget add Keil.Ensemble_AppKit-E7_BSP.pdsc`
    - [Alif Ensemble DevKit BSP](https://github.com/VladimirUmek/alif_ensemble_devkit-bsp)
        - clone the repo: execute `git clone https://github.com/VladimirUmek/alif_ensemble_devkit-bsp.git`
        - install the repo: execute `cpackget add cpackget add Keil.Ensemble_DevKit-E7_BSP.pdsc`

- Debug Probe
    - CMSIS-DAP compliant Debug Probe with 20 pin Cortex-Debug connector (ULINKplus, ULINKpro, ...)

- Alif Boards
    - [Ensemble E7 AI/ML AppKit](https://alifsemi.com/support/kits/ai-ml-appkit-gen-2/)
        - configured to run from the M55_HP core, see [instructions](https://github.com/VladimirUmek/alif_ensemble_appkit-bsp/blob/main/setools/README.md)
    - [Ensemble E7 DevKit](https://alifsemi.com/support/kits/ensemble-devkit-gen2/)
        - configured to run from the M55_HP core, see [instructions](https://github.com/VladimirUmek/alif_ensemble_devkit-bsp/blob/main/setools/README.md)

## Target names

- Alif-AppKit-E7-HP-U55: Ensemble E7 AI/ML AppKit
- Alif-DevKit-E7-HP-U55: Ensemble E7 DevKit

## Project names

- kws: Keyword spotting
- object-detection: Object detection

## Build the demo

- Open the repo root folder in VS Code.
- Open "CMSIS - Manage Solution Settings".
- Select Active Target for desired Board.
- Select Active Project.
- Click "CMSIS - Build solution".

## Run the demo

- Select the appropriate debug configuration under "Debug and Run".
- Click "Start Debugging".
- Open a terminal window with the serial port corresponding to the board terminal with 115200 baud rate.
- Interact with the application and observe the message in the terminal window.

More information is provided in the [README.md](./README.md).
