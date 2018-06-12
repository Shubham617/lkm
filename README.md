# lkm
Linux Kernel Module that acts as a plug-and-play key-value store for the Linux Kernel. 

A lot of help was taken from Derek Molly's articles on LKMs, especially for LKM and character device driver setup.

The LKM has been developed on Ubuntu 16.04.4 LTS and makes use of the internal kernel hashtable to put and get data via character
device driver


ebbchar.c - This source file is the heart of the LKM. It initializes the LKM, sets up the read and write via character device driver
and manages the puts and gets from the kernel hashtable

testebbchar.c - This is a test program for interacting with the key-value store LKM.
