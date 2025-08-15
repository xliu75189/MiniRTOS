This software is to illustrate the concepts of Real-Time Operating System (RTOS).
This MiniRTOS program is designed to use Array and Bit Map to implement Task List 
to speed up task search time. Therefore, the task priority is limited 0-31. It allows
same priority has more than one tasks. The same priority tasks are arranged with link list. 
For most applications, few tasks need at same priority. Therefore the same priority task 
link list should be short, and its search time and variant should be acceptable.

# Sub-directories in this project:
```
|
+---Application     - Test files for MiniRTOS
|
+---bsp             - Board specific files for MiniRTOS
|
+---CMSIS           - CMSIS (Cortex Microcontroller Software Interface Standard)
|
+---ek-tm4c123gxl   - support code for EK-TM4C123GXL (TivaC Launchpad) board 
|
|
+---MiniRTOS        -  MiniRTOS sources and selected ports
|
......................projects.............................
|

+---tm4c123-qxk-keil
|       ...
|       MiniRTOS.uvprojx - KEIL project for TM4C123 (TivaC LaunchPad)
|
