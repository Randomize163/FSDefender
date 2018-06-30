# FSDefender
In this project we implemented standalone Anti-Ransomware Solution to protect Windows users 
against Ransomware attacks and make their private files safe.

In our opinion, Anti-Ransomware Solution should always go together with files backup, 
so even in case of successful ransomware attack it could garantie zero file loss. 
For this reason our solution will include not only Ransomware detection and neutralization, 
but also Cloud storage for private user files.

Out solution consists of two interacting one with another modules (Kernel + User):
1) Kernel module, which works as a sniffer of file system current activity
2) User Module does processing and monitoring, backups files and kills detected threats
