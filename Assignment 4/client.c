/**************************
** Filename: server.c
** Author: Eddie Fox
** Date: December 3, 2016
**
** Description: This server program has pre-defined behavior based on how it has been compiled.
** If the ENCRYPT macro was defined during gcc compilation, it will communicate with the client in
** encrypt form. The server will run in the background and listen on a specific port.
** The client will provide plaintext and a key for encryption, and the server writes back cipher text
** via the same communication socket. If the DECRYPT macro is defined, it will decrypt cipher text provided by the client.
** The server provides support for 5 concurrent connections, and  creates error messages when it should, like if the key isn't
** at least as big as the plaintext, if the client or server is configured in the wrong type, or if there failed to be a connection
** through the socket.
*************************/

#include <stdio.h> /* Needed for things like printf, fgets, sprintf and perror. */
#include <stdlib.h> /* Needed for things such as malloc, execvp, and exit. */
#include <string.h> /* For various string operations such as strcmp and strtok. */

#include <unistd.h> /* Needed for various directory related commands, like getpid, fork, exec, and the pid_t type. */
#include <sys/types.h> /* provides definition for data types ssize_t and pid_t.*/
#include <sys/socket.h> /* Used for socket operations. */
#include <sys/stat.h> /* Provides various status information. */
#include <fcntl.h> /* Used for opening files in read and write modes. */

#include <netinet/in.h> /* Included for IP address macro manipulation.*/
#include <arpa/inet.h> /* Included for IP address macro manipulation. */
#include <netdb.h> /* Provides defintions for network data operations. */

#include <errno.h> /* Provides information on system error numbers. */
#include <signal.h> /* Needed for almost everything to do with sigaction, including the structure, and various signal set related options. */
#include <stdbool.h> /* Includes a macro that expands true to 1 and false to 0. Just for self-documentation purposes primarily.  */

/* The behavior of the client program differs based on if it is encrypting or decrypting. In the abscence of polymorphic object oriented behavior,
we can simulate this by how it is defined. Defining it as ENCRYPT or DECRYPT changes the server type to either 'e' for encrypt or 'd' for decrypt. */

#ifdef ENCRYPT
#define SERVERTYPE 'e'
#elif DECRYPT
#define SERVERTYPE 'd'
#endif

/* Forward declare the two functions through prototypes. The two functions being processMessage() and sendMessage(). */
void processMessage(char *port, char *messageFile, char *keyFile);
void sendMessage(char *port, char *messageBuffer, char *keyBuffer,size_t messageLength);

/* The client program processes 4 arguments. 
Argument #1: The name of the program.
Argument #2: Plaintext file
Argument #3: Key file
Argument #4: Port number to connect to. */

int main(int argc, char *argv[]) 
{
	/* If there isn't exactly 4 parameters, something is wrong. */
	if (argc != 4) 
	{
		fprintf(stderr, "Improper syntax. Try the following: Program_name plaintext_file key_file port_number"); /* Write error / ussage message.*/
		exit(1); /* Exit. */
	}

	/* Call the process message function with port number, the message file, and the key file.
	 * It is unnecessary to call the sendMessage function because the processMessage function already calls it. */

	processMessage(argv[3], argv[1], argv[2]); 
	return 0;
}

/****************************
**                 void processMessage(char *port, char *messageFile, char *keyFile) 
** Description: This function reads and processes the message and key files, extracting the content into key and message buffers 
** before attempting to connect to the server at a port via the sendMessage() function. 
****************************/

void processMessage(char *port, char *messageFile, char *keyFile) 
{
	int error; /* Variable for holding the error. */

	/* We create a stat struct to help us determine the status of the message. */

	struct stat buf; 
	error = stat(messageFile, &buf); /*Call the stat function from the structure on the message file with error checking. */

	if (error == -1) /* If there is an error, write an error message, and then exit. */
	{
		fprintf(stderr, "Error occured with the message file. Perhaps there is no valid one.");
		exit(1);
	}

	/* st_size from the buf stat structure gives us the total size of the file in bytes. We can therefore drop the newline at the end of the message file 
	 * by setting the length equal to the total size of the file minus 1. It would knock off the last character no matter what, but here it happens to be a newline. */

	size_t messageLength = buf.st_size - 1; 

	/* Now we repeat the stuff we did above in order to determine the status of the key file, with error checking. */

	error = stat(keyFile, &buf); /* Call the stat function from the structure on the key file with error checking. */

	if (error == -1) /* If there is an error, write an error message, and then exit. */
	{
		fprintf(stderr, "Error occured with the ke file. Perhaps there is no valid one.");
		exit(1);
	}

	/* Drop the newline of the keyfile by setting the length equal to the total size of the file minus 1, knocking off the last character. */

	size_t key_len = buf.st_size - 1;

	/* Use an if statement to confirm the key length is at least as long as the plain text file. Otherwise, it will run out of characters to encrypt the plaintext message. */

	if (key_len < messageLength) /* If the key length is shorter than the message length, there is a problem. */
	{
		fprintf(stderr, "Error: Keyfile not as long as plaintext message.\n");
		exit(1);
	}

	/* Dynamically allocate character arrays for the message and key buffers. */

	char *messageBuffer = malloc(messageLength);
	char *keyBuffer = malloc(messageLength);


	/* Open message file with error checking and handling. */

	int messagefd = open(messageFile, O_RDONLY); /* Open message file in read only mode. */

	if (messagefd == -1) /* If this condition is true, an error has occured. */
	{
		fprintf(stderr, "Error opening message file");
		exit(1);
	}

	/* Open key file with error checking and handling. */

	int keyfd = open(keyFile, O_RDONLY); /* Open key file in read only mode. */
	
	if (keyfd == -1) /* If this condition is true, an error has occured. */
	{
		fprintf(stderr, "Error opening key file");
		exit(1);
	}

	/* Read message from message file into message buffer, with error checking and handling. */
	error = read(messagefd, messageBuffer, messageLength);
	if (error == -1) 
	{
		fprintf(stderr, "Error reading message file");
		exit(1);
	}

	/* Read key from key file into key buffer, with error checking and handling. */

	error = read(keyfd, keyBuffer, messageLength);

	if (error == -1) 
	{
		fprintf(stderr, "Error reading message file");
		exit(EXIT_FAILURE);
	}

	/* Close file descriptors since we are done reading them into the buffers.*/
	close(keyfd);
	close(messagefd);

	/* Now that we are done with reading the messages and keys from the file, we neeed to validate and process the 
	 * messages and keys from the buffers before we can call the sendMessage() function. */

	/* Scan the message. If any character is not either a space or between 'A' and 'Z' on the ASCII table, print a 
	 * message declaring the message to be invalid, and exit in failure. */

	for (size_t i = 0; i < messageLength; i++)
	{
		if (!(messageBuffer[i] == ' ' || (messageBuffer[i] >= 'A' && messageBuffer[i] <= 'Z'))) 
		{
			fprintf(stderr, "Invalid message character encountered. %c. Exiting due to error.\n", messageBuffer[i]); /* Write invalid message message while specifying exit. */
			printf("For reference, here is the contents of the message buffer: %s", messageBuffer); /* Print the message for reference. */
			exit(1); /* Exit in failure. */
		}

	/* We can put the key buffer validation in the same input, since by definition, the key buffer
	 * must be at least as long as the plaintext buffer (if the program has reached this far without an error induced exit). */

		if (!(keyBuffer[i] == ' ' || (keyBuffer[i] >= 'A' && keyBuffer[i] <= 'Z'))) 
		{
			fprintf(stderr, "Invalid key character %c.\n", keyBuffer[i]); /* Print the invalid character in the key.*/
			printf("For reference, here is the contents of the key buffer: %s", keyBuffer);
			exit(1);
		}
	}

	/* Now that the contents of the message and key buffers have been validated, we can call the sendMessage()
	 * function in order to communicate with the server that will encrypt / decrypt our request. */

	sendMessage(port, messageBuffer, keyBuffer, messageLength);

	/* After the sendMessage() function returns, we can free the message and key buffers, as they aren't needed any more, 
	 * and we want to prevent memory leaks. */

	free(messageBuffer);
	free(keyBuffer);
}

void sendMessage(char *port, char *messageBuffer, char *keyBuffer, size_t messageLength) 
{
	int portNumber = atoi(port); /* Convert the port char parameter from a string to an integer, then assign the value to the portNumber variable. */
	int socketfd; /* Holds the file descriptor for the socket. */
	int error; /* Holds errors. */

	struct sockaddr_in serverAddress; /* Create sockaddr struct for server Address. */
	
	/* Set the client and server types appropriately. */

	char client_type = SERVERTYPE; 
	char server_type = 2;

	/* Open the socket with error checking and handling. */
	socketfd = socket(AF_INET, SOCK_STREAM, 0); /* With IP protocols, the last digit is 0. */

	if (socketfd < 0) 
	{
		fprintf(stderr, "Error connecting to the socket. ");
		exit(2); /* Network errors are given the exit code of 2. */
	}

	//bzero((char*) &serverAddress, sizeof(serverAddress));

	/*  Process information and put it in the address structure. */
	inet_pton(AF_INET, port, &(serverAddress.sin_addr));

	serverAddress.sin_family = AF_INET; /* Set  the TCP and IP protocol. */
	
	/* Transform byte order of the data to network. */
	serverAddress.sin_port = htons(portNumber);

	/* Attempt to connect to the server through the port, with error checking and handling. */

	error = connect(socketfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)); /* Connect to the socket. */
	if (error < 0)  /* This error indicates the client couldn't connect to the socket. */
	{
		fprintf(stderr, "Couldn't connect client to the socket");
		exit(2);
	}

	/* Attempt to write client type to the socket. */
	error = write(socketfd, &client_type, sizeof(char));
	if (error < 0) 
	{
		fprintf(stderr, "Error writing client type to socket");
		exit(2);
	}

	/* Attempt to read the server type from the socket. */

	error = read(socketfd, &server_type, sizeof(char));
	if (error < 0) 
	{
		fprintf(stderr, "Error reading server type from socket");
		exit(2);
	}

	/* Reject connection if the types of the server and the client don't match. */

	if (server_type != client_type) 
	{
		fprintf(stderr, "Server and client types do not match. Connection rejected.\n"); /* Write message. */
		shutdown(socketfd, 2); /* Shut down connection to socket. */
		close(socketfd); /* Close socket file descriptor. */
		exit(2);
	}

	/* Attempt to write message length to socket, with error checking and handling. */

	error = write(socketfd, &messageLength, sizeof(size_t));
	if (error < 0) 
	{
		fprintf(stderr, "Error writing message length to the socket"); /* */
		exit(2);
	}

	/* Attempt to send message to the server by writing the message buffer to the socket. */
	error = write(socketfd, messageBuffer, messageLength);
	if (error < 0) 
	{
		fprintf(stderr, "Error writing message to the socket");
		exit(2);
	}

	/* Attempt to send key to the server by writing the key buffer to the socket. */
	error = write(socketfd, keyBuffer, messageLength);
	if (error < 0) 
	{
		fprintf(stderr, "Error writing key to the socket");
		exit(2);
	}

	/* Eventually, after the server is done with everything, it will eventually provide a response. Either the plaintext or encrypted
	 * version of the message in the buffer. The server will write it to the socket, so here we read the message buffer from the socket, then write it to STDOUT. */

	error = read(socketfd, messageBuffer, messageLength); /* Attempt to read message buffer from socket with error checking. */
	if (error < 0) 
	{
		fprintf(stderr, "Error reading server response from socket");
		exit(2);
	}

	/* Write the newly read response message buffer to STDOUT. */

	write(STDOUT_FILENO, messageBuffer, messageLength);
	write(STDOUT_FILENO, "\n", 1);
	close(socketfd); /* Close the socket connection to the server, then initiate clean up at the end of the processMessage() function. */
}