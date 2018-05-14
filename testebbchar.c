/**
 * @file   testebbchar.c
 * @author Derek Molloy
 * @date   7 April 2015
 * @version 0.1
 * @brief  A Linux user space program that communicates with the ebbchar.c LKM. It passes a
 * string to the LKM and reads the response from the LKM. For this example to work the device
 * must be called /dev/ebbchar.
 * @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
*/
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM

int main(){
   int ret, fd;
   char stringToSend[BUFFER_LENGTH];
   printf("Starting device test code example...\n");
   fd = open("/dev/ebbchar", O_RDWR);             // Open the device with read/write access
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }
   printf("Instructions to Use KeyValue Storage LKM!\n"); 
   printf("To Put: Put key(int) value(string))\n");
   printf("To Get: Get key(int)\n");
   scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
   //printf("Writing message to the device [%s].\n", stringToSend);
   //ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
   //if (ret < 0){
    //  perror("Failed to write the message to the device.");
   //   return errno;
   //}
  //printf("This is what you entered = %s\n", stringToSend);
  if(stringToSend[0]=='P' && stringToSend[1]=='u' && stringToSend[2]=='t'){
  //priintf("Enter a key(int) followed by space, and then the value you'd like to put(string)\n");
  //strcpy(stringToSend, "");
  //scanf("%[^\n]%*c", stringToSend);
   write(fd, stringToSend, strlen(stringToSend)); 
}
   else{
   //printf("Enter the key of the object\n");
   //strcpy(stringToSend, "");
   //scanf("%[^\n]%*c", stringToSend);
   write(fd, stringToSend, strlen(stringToSend));
   ret = read(fd, receive, BUFFER_LENGTH);
   if(ret < 0){
   printf("There was a problem! Sorry!\n");
   }
   printf("The corresponding value is [%s]\n",receive); 	  
}
/*
   printf("Reading from the device...\n");
   ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   printf("The received message is: [%s]\n", receive);
   printf("End of the program\n");
*/
   return 0;
}
