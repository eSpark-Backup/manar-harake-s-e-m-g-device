# ManarHarake-sEMGDevice

## Branch Information

|Branch Name|Description|
|-----------|-----------|
|main|This branch currently holds all the basic required info on the readme as well as the design last fabricated and under testing by the debugging team|
|sEMG Product-v1.0.0-|This branch holds the last developed design version. The design previously developed had LCD Board, Sensor board and Control board but with all the major amplifiers and circuitries on Control Board. It was scrapped off due to the issue that signal from sEMG sensors will need some pre-amplification to allow them to reach the Control Board effectively.
|Hardware-Development|This branch contains the design under evaluation. The key points of this design include making separate Electrode boards to allow the signal from sensors to be pre-amplified so that it can effectively reach the Control board. The LCD board was removed from the design for later use|
|Firmware-Development|This branch contains source code and development info of the hardware under evaluation. It contains the test sketches developed and debugging of the board to make it work|

## 1. Firmware Upload Guide
### 1.1 Setting up the Hardware
The hardware mainly the control board expected to power up over 8.4V (Two cell LiPo / Li-Ion battery). However, due to slight issues with the battery supervisor circuit, the Voltage Divider network expected to bring voltages to safe range had to be removed. **The board is now powered up over 4V for safe operations**.

####  1.1.1 Tools required
1. Power Supply (5V / 1A)
2. JLink Debugger / Programmer
3. USB to USB Type C cable (For serial debugging)


####  1.1.2 Software required
1. [Microchip Studio](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio#Downloads)

#### 1.1.3 Step by Step configuration
1. Power up the supply and set it to 4V.
2. Connect the power supply cables to the power input pins on the PCB as can be seen in the screenshot below. Make sure the polarity is correct after observing the -ve and +ve pin identifiers
<p align="center" width="100%">
<img src="https://user-images.githubusercontent.com/76909080/195995410-549fbed7-de78-469c-aeab-4af878973c96.png"/>
</p>
3. Connect the debugger with the board as per the table below. The debugger pin snapshot as well as the PCB pin names are given in screenshots below:
<div align="center" width="100%">

|Board Pin Name|Debugger Pin Name|
|----|----|
|VCC_3V3|VTRef|
|TDI|Do not connect|
|TDO|Do not connect|
|TMS|SWDIO|
|TCK|SWCLK|
|NRST|nRESET|
|ERASE|Do not connect|
|GND|GND|
</div>
<p align="center" width="100%">
<img src="https://user-images.githubusercontent.com/76909080/195996062-b666f94c-5eff-4f73-bf63-309af3ffa576.png"/><img src="https://user-images.githubusercontent.com/76909080/195996295-b1c436bd-0b0d-452f-8828-67160a2047d9.png"/>
</p>

4. Once the connection are properly made, proceed to launch Microchip Studio and verify device connection with debugger using the **Device Programming** option in the software.
5. After successful connection between hardware and Microchip, proceed to upload the provided .hex file to the board from the **Memories** window of the **Device Programming** option.
