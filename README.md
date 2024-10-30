# mfrc522 spi driver
mfrc522 spi driver based on Linux kernel v6.6 

## 1. Key Features
- give spi interface to communicate with RFID module with MFRC522.
- give implementation of ISO/IEC 14443-3 A protocol
- give userspace interface as character device, `mfrc522dev0.0` 


## 2. Implementation 
- mfrc522_drv.c
    - spi driver
    - create class and character device for every matched device.
        - open()
            - setup the device
        - read()
            - process the sequence (reqa, anti-collision loop, select)
- MFRC522.c
    - implementaion of MFRC522 functions(communicate with PICC). 
- MFRC522.h
    - define macros for register address and commands.
    - define prototype of MFRC522 functions.
- device_list & device_list_lock
- bitmap to assign minor number


## 3. How to use
1. build with `Makefile`
2. `insmod *.ko`
3. build `test.c` and run it 

## 4. Terms of Abbreviations
|Terms|Contents|
|---|---|
|ATQA|Answer to Reqeust Type A  | 
|ATS | Answer To Select  |
|CL | Cascading Level   |
|CT | Cascading Tag  |
|NVB | Number of Valid Bits  |
|PCD | Proximity Coupling Device (“Reader”)  |
|PICC | Proximity Intergrated Circuit Card (”Card”), Tag  |
|REQA | REQuest Type A   |
|RATS | Request for Answer To Select   |
|SAK | Select ACK   |
|SL |Security Level  |
|PPS | Protocol Parameter Selection|

## 6. Reference 
- https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf
- https://github.com/libdriver/mfrc522/blob/main/src/driver_mfrc522.h#L361
- https://www.nxp.com/docs/en/application-note/AN10834.pdf
