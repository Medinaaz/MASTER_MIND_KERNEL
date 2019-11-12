Mehmet Yakuphan Bilgic
Medina Zaganjori
Sabrina Cara

In this homework we have implemented Mastermind game using device drivers. 

To run this homework you have to execute the following commands in your terminal.
We suggest you to use Virtual Box in order not to cause problems with the kernel of your computer.

In your command line, first go to the folder where the file is. 
Then you become root by typing command "sudo su".

Afterwards you make the file by typing "make" in the command line. 
Insmod readmymind.ko  <---- will insert module into linux kernel

mknod -m 666 /dev/mastermind c 250 0 <---- makes node in mastermind with major number 250 and minor 0

echo "xxxx" > /dev/mastermind <---- writes guess input to device buffer where guess input is "xxxx"
 
cat /dev/mastermind <---- reads result from device driver to user after computing it in write

