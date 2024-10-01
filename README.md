# mfrc522
test mfrc522 spi driver

## Terms of Abbreviations
ATQA : Answer to Reqeust Type A   
ATS : Answer To Select  
CL : Cascading Level   
CT : Cascading Tag  
NVB : Number of Valid Bits  
PCD : Proximity Coupling Device (“Reader”)  
PICC : Proximity Intergrated Circuit Card (”Card”), Tag  
REQA : REQuest Type A   
RATS : Request for Answer To Select   
SAK : Select ACK   
SL : Security Level  
PPS : Protocol Parameter Selection


## MFRC522 드라이버 Features
- 디바이스 트리 오버레이
- Supports ISO/IEC 14443 A/MIFARE and NTAG
    - ISO/IEC 14443-1,2,3,4
        - 카드의 물리적 사양(1), 라디오 규격(2), 초기화 및 충돌방지(3), 전송 프로토콜(4)

## ISO/IEC 14443-3 A
- PICC Activation Sequence
    - send REQA, receive ATQA
- Anti-collision Loop
    - ErrorReg
        - ErrorReg[3] = CollErr; a collision detected, value is “1”.
    - CollReg
        - CollReg[7] = ValueAfterCol
            - if ValueAfterCol=0, 충돌 후에는 모든 수신된 비트 초기화
        - CollReg[5] = CollPosNotValid
            - if CollPosNotValid=1, 충돌X.
            - if CollPosNotVaild=0, 충돌O, CollPos 활성화
        - CollReg[4:0] = CollPos
    - BitFramingReg
        - BitFramingReg[4:6] = RxAlign
            - 첫번째 비트를 FIFO의 몇번째 포지션에 놓아야 할지 지정.
        - BitFramingReg[0:2] = TxLastBit
            - 전송할 마지막 바이트에서 몇번째 비트까지가 valid 한지 지정.
- Select PICC  
  - with full UID, BCC, CRC, select one PICC. get SAK from it.
  - Select
      - NVB = 0x70; Calculate CRC_A; 
  - 3 byte response
      - first byte = SAK value
        - SAK 레이아웃에 따라 이후 작업진행  
      - second, third byte = CRC_A value


## Reference 
- https://m.blog.naver.com/ittalentdonation/221215499032
- https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf
- https://github.com/libdriver/mfrc522/blob/main/src/driver_mfrc522.h#L361
- http://www.airspayce.com/mikem/bcm2835/group__spi.html#gad25421b3a4a6ca280dfdd39c94c3279a
- https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/nfc/4928/1/ISO-IEC%2014443-3_anticollision.pdf
- https://www.nxp.com/docs/en/application-note/AN10834.pdf
