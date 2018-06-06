/**************************
** Filename: keygen.c
** Author: Eddie Fox
** Date: December 3, 2016
**
** Description: This file creates a keyfile of the specified length. It will create the key by 
** randomly generating the 26 letters of the alphabet, plus spaces. It outputs to stdout. 
*************************/

#include <stdio.h> /* Needed for things like printf, fgets, sprintf and perror. */
#include <stdlib.h> /* Needed for things such as malloc, execvp, and exit. */
#include <string.h> /* For various string operations such as strcmp and strtok. */
#include <time.h> /* Used to seed the random number generator. */

/****************************
**                                    void usageMessage(int argc, char *argv[])
** Description: This function provides a helpful usage message to clue the user in to 
** the proper syntax that should be used if the number of arguments they input is not equal to 2. 
** The two needed arguments is keygen and the length. 
****************************/
void usageMessage(int argc, char *argv[]) 
{
	if (argc != 2) /* If the number of arguments are not 2,*/
	{
		printf("Usage: keygen length , where length is the size the key should be in bytes."); /* Print the message.*/
		exit(0); /* Exit*/
	}
}

int main(int argc, char *argv[]) {

	srand(time(NULL)); /* Seeds the random number generator based on the system time. More specifically, the number of seconds since January 1, 1970.*/

	int i; /* Loop control variable. */
	int length; /* Will eventually hold the length parameter passed into the file. */

	char key[length + 1]; /* Need +1 to contain enough space for the newline. */
	char randomLetter; /* Holds a random letter (or space), will be used in a for loop to add as many letters as the specified length. */

	usageMessage(argc, argv); /* This will only execute if the number of arguments is not 2.*/


	length = atoi(argv[1]); // Converts the second argument from characters to an integer, and assigns it to the length variable. 

	for (i = 0; i < length; i++) 
	{
		randomLetter = " ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 27]; /* Needs to be modulo 27 instead of modulo 26 to factor in the extra space. */
		key[i] = randomLetter; /* Assigns the appropriate random letter to where it should be in the key array. */

	}

	key[length] = '\0'; /* Strips the newline and adds a terminating character. */

	printf("%s\n", key); /* Prints the key. */

	return 0;
}