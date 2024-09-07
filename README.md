# mfrc522
mfrc522 spi driver test

## Terms of Abbreviations
PCD : Proximity Coupling Device (“Reader”)
PICC : Proximity Intergrated Circuit Card (”Card”), Tag
REQA : REQuest Type A 
ATQA : Answer to Reqeust Type A 
RATS : Request for Answer To Select 
ATS : Answer To Select 
SAK : Select ACK 
SL : Security Level
CL : Cascading Level 
CT : Cascading Tag
NVB : Number of Valid Bits

## REQA & ATOA
- makes PICC ready 

## Anti-collision Loop 
- loop until PCD receive perfect UID

## Select & SAK
- with full UID, BCC, CRC, select one PICC. get SAK from it. 
