# stn1170-tools
Tools for STN11xx devices configuration such as:
- set STN11xx device UART baudrate
- autodetect current UART baudrate
- show info about STN11xx device
- maximize STN11xx device UART baudrate

### Examples of usage
Maximize baudrate `stntool -m /dev/ttyUSB3`  

Detect current baudrate `stntool -d /dev/ttyUSB3`  

Set baudrate `stntool -s 2000000 /dev/ttyUSB3`  
