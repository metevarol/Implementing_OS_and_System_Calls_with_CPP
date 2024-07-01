1- For Ubuntu 22.04.2, you need install:

sudo apt-get install g++ binutils libc6-dev-i386
sudo apt-get install VirtualBox grub2 xorriso

2- After that you can run this command:

make mykernel.iso


3- Then you should create new virtual machine with this ISO file on VirtualBox with name: "My Operating System".

4- After that you can run this command for open OS on VirtualBox

make run