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

## REQA & ATOA
makes PICC ready 

## Anti-collision Loop 
loop until PCD receive perfect UID

## Select & SAK
with full UID, BCC, CRC, select one PICC. get SAK from it.

## TODO 
- Cascade Level different behavior.
- test anti-collision loop that it perfectly works. 

## Reference 
- https://en.wikipedia.org/wiki/Serial_Peripheral_Interface
- https://m.blog.naver.com/ittalentdonation/221215499032
- https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf
- https://github.com/libdriver/mfrc522/blob/main/src/driver_mfrc522.h#L361
- http://www.airspayce.com/mikem/bcm2835/group__spi.html#gad25421b3a4a6ca280dfdd39c94c3279a
- https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/nfc/4928/1/ISO-IEC%2014443-3_anticollision.pdf
- https://www.nxp.com/docs/en/application-note/AN10834.pdf
