1. Firstly, copy the VPE directory that in the to directory to SD Card for source pattern. That means the SD card will have VPE directory.
2. Run the demo code. For target project with TV output, Plug in the TV cable. 

Memory size note:
1. Default memory size is 64MB (0x4000000).
2. If we want to change to 32MB please modify the file "wb_init.s" and find 0x4000000 then change to 0x2000000