# stn1170-tools
Tools for STN11xx devices configuration such as:
- set STN11xx device UART baudrate
- show info about STN11xx device
- maximize STN11xx device UART baudrate

### Examples of usage
Maximize baudrate `stntool -m /dev/ttyUSB3`  

Get current baudrate and device info `stntool -i /dev/ttyUSB3`  

Set baudrate `stntool -s 2000000 /dev/ttyUSB3`  
