# Item Reader for PSOBB

This item reader is heavily based Solybum's work.
Which maybe found here: https://github.com/Solybum/PSOBBMod-Addons
My sincere apologees, there does not appear to be documentation on the target
program's memory layout anywhere online, even after extensive googling.

This application requires ptrace permissions in order to read memory from a 
application. Which you may grant by the following command:
`sudo setcap 'CAP_SYS_PTRACE+ep' /path/to/binary/apir`

To make the application, just run make.
Note: You will need not only my utilities library, but also ncurses and linux 
capabilities libraries too.

Also note: these are early commits, I still need to do some testing to get a 
better idea on how to make this work.
