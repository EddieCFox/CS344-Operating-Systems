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
#include <sys/wait.h> /* Used for waitpid(). */

#include <netinet/in.h> /* Included for IP address macro manipulation.*/
#include <arpa/inet.h> /* Included for IP address macro manipulation. */

#include <errno.h> /* Provides information on system error numbers. */
#include <signal.h> /* Needed for almost everything to do with sigaction, including the structure, and various signal set related options. */
#include <stdbool.h> /* Includes a macro that expands true to 1 and false to 0. Just for self-documentation purposes primarily.  */


/* The behavior of the server program differs based on if it is encrypting or decrypting. In the abscence of polymorphic object oriented behavior,
   we can simulate this by how it is defined. Defining it as ENCRYPT or DECRYPT affects two key things. First, it changes the server type to either 
   'e' for encrypt or 'd' for decrypt. The second thing it does is change the CRYPT function. The first parameter represents the message, while the second
   one is the key. In OTP, if we are encrypting, we add the two values together, while if decrypting, we subtract them from each other. The functions are defined
   to reflect this. */

/* I don't actually know if I can comment on preprocessor define macros, but I'm paranoid that if I do, they will be added every time, so I'm going to not to be safe.
Another note is that I am casting each of the parameters to an int, as they are originally passed in as char pointer buffers, and we need to perform math on them. */

#ifdef ENCRYPT

#define SERVERTYPE 'e'
#define CRYPT(a, b) (a) = (int)( (int)(a) + (int)(b) );

#elif DECRYPT
#define SERVERTYPE 'd'
#define CRYPT(a, b) (a) = (int)( (int)(a) - (int)(b) );

#endif 

/* Here we forward declare the function prototypes, so if the functions reference each other, they won't be confused
   as to the meaning of other functions that have yet to be declared.*/

void exitServer(int a);
void endingChild(int signalNumber);

void OTP(size_t messageLength, char *keyBuffer, char*messageBuffer);

void setup(int portNumber);

void serverLoop(int socketfd);
void cleanup(int clientsocketfd, char *keyBuffer, char *messageBuffer);

int main(int argc, char *argv[]) 
{
	int portNumber; /* Variable to hold the port number. */

	if (argc != 2) /* The server only takes the name of the program and the number of the port to listen on. If there isn't 2 arguments, the syntax is wrong, and it exits as a failure.*/
	{
		fprintf(stderr, "Improper syntax.\n"); /* Notify user of improper syntax.*/
		exit(1); /* Exit as a failure.*/
	}

	signal(SIGINT, exitServer); /* Signal handler for interrupts that calls the exitServer function. */
	signal(SIGCHLD, endingChild); /* Signal handle for child signals that calls the endingChild function.*/

	portNumber = atoi(argv[1]); /* Processes the first argument, and converts it from string to integer, then assigns the integer to the port number variable. */
	setup(portNumber); /* Runs the setup function, which takes care of the setup, then runs serverLoop at the end. */
}

/****************************
**                                void exitServer(int a)
** Description: This function is called if a interrupt signal is recieved. Tries to wait for the 
** child process to terminate, but if it does not, it will need to terminate it. 
****************************/

// Wait for children when receiving SIGINT.

void exitServer(int a) 
{
	int childInfo; /* Variable to hold the information of the child. */
	pid_t pid; /* Holds the process id of the child, probably 0. */
	while ((pid = waitpid(0, &childInfo, 0)) != -1)
	{
		if (errno == ECHILD)
		{
			exit(0);
		}

		else 
		{
			fprintf(stderr, "Child did not exit successfully. Killing soon.");
			exit(1);
		}
	}	
}
#pragma clang diagnostic pop

/****************************
**                               void endingChild(int signalNumber)
** Description: This function is called if a child signal is recieved.  Reaps children as they die, 
** whenever the server recieves sigchild.
****************************/

void endingChild(int signalNumber) {
	int childInfo; /* Variable to hold child info.*/
	pid_t pid = waitpid(0, &childInfo, 0); /* Wait for child to exit. */
	if (pid == -1) /* If child not exited.*/
	{
		fprintf(stderr, "Killing child soon after SIGHCHLD recieved by server."); /* Prints the error message described here.*/
		exit(1); /* Exit with failure. */
	}
}

/****************************
**                              void setup(int portNumber)
** Description: Does all the network setup with binding and listening to sockets and ports.
** At the end, it calls the main socket function. 
****************************/

void setup(int portNumber) {
	int socketfd; /* Variable for the socket file descriptor. */
	int error; /* Variable for any error that might occur. */

	struct sockaddr_in serverAddress; /* Server address structure. */

	/* Open socket using TCP and IP protocols. */

	socketfd = socket(AF_INET, SOCK_STREAM, 0); /* When using IP protocol, you use a zero as the last parameter.*/

	if (socketfd < 0) /* Testing this conditional will test for if the socket successfully managed to open. If not, it prints an error message and exits. */
	{
		fprintf(stderr, "Error connecting to the socket.");
		exit(2);
	}
	
	serverAddress.sin_family = AF_INET; /* Create a network-capable socket */
	serverAddress.sin_port = htons(portNumber); /* Convert from host to network byte order and store the port number.*/
	serverAddress.sin_addr.s_addr = INADDR_ANY; /* Specifies that any address is allowed for this process. */

	error = bind(socketfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)); /* Attempts to bind server to the socket, while also testing for errors. */

	if (error < 0) /* If there was an error, print according message and exit in failure.*/
	{
		fprintf(stderr, "Failed to bind server to the socket.");
		exit(2);
	}

	error = listen(socketfd, 5); /* Specifies the max number of concurrent ports to be 5. */

	if (error < 0) /* If there was an error, write an error message and exit in failure. */
	{
		fprintf(stderr, "Failed to listen on the port.");
		exit(2);
	}

	/* Here we call the serverLoop, after all the network set up is complete. It wouldn't be proper to 
	include it in the setup function, since it is the actual execution after setup.*/

	serverLoop(socketfd);
}

/****************************
**                           void serverLoop(int socketfd)
** Description: Runs pretty much everything in the program after setup is complete. 
****************************/
void serverLoop(int socketfd) 
{
	int newsocketfd; /* Holds the file descriptor of the new socket. */
	int error; /* Holds errors. */
	struct sockaddr_in clientAddress; /* Creates structure for client address.*/
	socklen_t clilent = sizeof(clientAddress); /* Holds the size of the address for formal structure purposes. */
	char client_type = 1; /* Sets client type. */
	char server_type = SERVERTYPE; /* Server type is set to encrypt or decrypt accordingly based on macro definition at time of compilation. */
	char *messageBuffer, *keyBuffer; /* Creates character array buffers to hold both the plaintext and the key. */
	size_t messageLength; /* Creates a variable to hold the length of the message. */

	while (true) /* True works because of stdbool.h, which extends the true macro to 1. */
	{
		
		newsocketfd = accept(socketfd, (struct sockaddr *) &clientAddress, &clilent); /* Accepts new connections from clients.*/

		if (newsocketfd < 0) /* If the accept function failed, and no new clients were accepted, print a message saying as much. */
		{
			fprintf(stderr, "Failed to accept connection.\n");
		}

		pid_t pid = fork(); /* Fork the parent server process accordingly as connections are made. */

		if (pid == 0) /* If pid is 0, then it is a child, and we check for errors and proceed with the encryption process. */
		{  
			error = read(newsocketfd, &client_type, sizeof(char)); /* Reads from the socket a single character that gives us the client type, while checking for errors.*/
			if (error < 0) /* If there is an error, the server failed to read from the socket, so write a message indicating so, and exit. */
			{
				fprintf(stderr, "Failed to read client type to the socket.\n");
				cleanup(newsocketfd, NULL, NULL); /* Call cleanup function to close the socket, clear filedescriptors, and free memory. */
				exit(1);
			}

			error = write(newsocketfd, &server_type, sizeof(char)); /* Attempt to write server type to the socket while checking for errors. */
			if (error < 0) 
			{
				fprintf(stderr, "Failed to write program type to socket.\n"); /* If there is an error, the server failed to write its type to the socket, so write a message indicating so, and exit. */
				cleanup(newsocketfd, NULL, NULL); /* Call cleanup function to close the socket, clear filedescriptors, and free memory. */
				exit(1);
			}

			/* At this point, we compare the server type to the client type. They will only match if an encrypt client is connecting to an encrypt server or a 
			   decrypt client is connecting to a decrypt server. I f not, reject the connection. */

			if (client_type != server_type) 
			{
				fprintf(stderr, "Rejecting connection. Wrong type of client.\n"); /* print error message.*/
				shutdown(newsocketfd, 2); /* Shutdown and close socket. */
				close(newsocketfd);
				exit(2);
			}

			error = read(newsocketfd, &messageLength, sizeof(size_t)); /* Read the length of the message while checking for errors. */
			if (error < 0) 
			{
				fprintf(stderr, "Failed to read message length from the socket."); /* If there is an error, the server failed to read the message length from the socket. */
				cleanup(newsocketfd, NULL, NULL); /* Clear file descriptors, close socket, and free any memory. */
				exit(2);
			}

			/* We declared the message buffer and key buffer above, but now we dynamically allocate space for them, up to the size of the message length. */

			messageBuffer = malloc(messageLength);
			keyBuffer = malloc(messageLength);

			error = read(newsocketfd, messageBuffer, messageLength); /* Read the message while checking for errors.*/

			if (error < 0) /* If there is an error, the server failed to read the message from the socket. */
			{
				fprintf(stderr, "Failed to read the message from the socket. Unknown error.\n");
				cleanup(newsocketfd, keyBuffer, messageBuffer); /* Clear file descriptors, close socket, and free any memory. */
				exit(2);
			}

			error = read(newsocketfd, keyBuffer, messageLength); /* Read the key while checking for errors. */

			if (error < 0) /* If there is an error, the server failed to read the key from the socket.*/
			{
				fprintf(stderr, "Failed to read from socket");
				cleanup(newsocketfd, keyBuffer, messageBuffer); /* Clear file descriptors, close socket, and free any memory. */
				exit(2);
			}
			
			/* Now we call the actual OTP (One Time Pad) function do the actual encryption / decryption, and write the result back to the message buffer. */

			OTP(messageLength, keyBuffer, messageBuffer);

			error = write(newsocketfd, messageBuffer, messageLength); /* After storing the result in the messageBuffer, we write the response back to the client while error checking.*/
			
			if (error < 0) /* If there was an error, the server somehow failed to write back to the socket.*/
			{
				fprintf(stderr, "Failed writing to socket."); 
			}
			
			/* If the program has reached this point, it has done it's job, so we call clean up to clear file descriptors, close the socket, and free memory. 
			   The porgram can then exit successfully. */

			cleanup(newsocketfd, keyBuffer, messageBuffer); /* Call cleanup.*/
			exit(EXIT_SUCCESS); /* Exit successfully, with an exit code of 0 unlike all these other exit codes. */
		}

		/* If it isn't a child, we continue to the next iteration of the serverLoop, accepting another connection from clients. */
		
		else 
		{
			continue; /* Continue to the next iteration of the loop. */
		}
	}
}


/****************************
**                         void OTP(size_t messageLength, char *keyBuffer, char *messageBuffer) 
** Description: Performs the actual encryption / decryption after all the network stuff and error checking is said and done. 
****************************/

void OTP(size_t messageLength, char *keyBuffer, char *messageBuffer) 
{
	/* For the entire message length, and key buffer length, replace space with [.  [ is the ASCII code right after Z. 
	 * We then adjust the ASCII codes by subtracting from the ASCII code for 'A' from everything, such that A = 0 and 
	 * space = 26, while Z is 25. This adjusts the values properly to apply the algorithm of the function. */

	for (size_t i = 0; i < messageLength; i++)  
	{
		if (messageBuffer[i] == ' ') 
		{
			messageBuffer[i] = '[';
		}

		if (keyBuffer[i] == ' ') 
		{
			keyBuffer[i] = '[';
		}
		messageBuffer[i] = messageBuffer[i] - 'A';
		keyBuffer[i] = keyBuffer[i] - 'A';
	}

	/* Now we apply the actual algorithm, using a custom CRYPT function based on the macro definition at compile time. 
	 * In case of negative modularization, it adds 27 to wrap around the alphabet. */

	for (size_t i = 0; i < messageLength; i++) 
	{
		CRYPT(messageBuffer[i], keyBuffer[i])
			if (messageBuffer[i] < 0) {
				messageBuffer[i] = 27 + messageBuffer[i];
			}
			else /* If the particular message buffer letter is non-negative, and thus doesn't require adjustment, you can just modulo 27 it. */
			{
				messageBuffer[i] %= 27;
			}
	}
	
	/* Since we are now done with the algorithm, we transform all the numbers in the message and key buffer back to ASCII code by adding the ASCII value of
	 * 'A', then replacing all '[' with ' '. (Left brackets with spaces). */

	for (size_t i = 0; i < messageLength; i++) 
	{
		messageBuffer[i] = messageBuffer[i] + 'A';
		keyBuffer[i] = keyBuffer[i] + 'A';

		if (messageBuffer[i] == '[') 
		{
			messageBuffer[i] = ' ';
		}

		if (keyBuffer[i] == '[') 
		{
			keyBuffer[i] = ' ';
		}
	}
}

/****************************
**                        void cleanup(int clientsocketfd, char *keyBuffer, char *messageBuffer) 
** Description: Closes the socket connection to the client, closes file descriptors, and deallocates the dynamic memory 
** of the message and key buffer if they were not empty when this function was called. 
****************************/
void cleanup(int clientsocketfd, char *keyBuffer, char *messageBuffer) 
{
	int error; /* Variable to hold an errors. */

	if (messageBuffer != NULL) 
	{
		free(messageBuffer); /* Free message buffer if it wasn't empty.*/
	}

	if (keyBuffer != NULL) 
	{
		free(keyBuffer); /* Frees key buffer if it wasn't empty. */
	}

	error = shutdown(clientsocketfd, 2); /* Shuts down the client socket connectionw hile checking for errors. */

	if (error == -1)  /* If there was an error, indicate that there was a problem shutting down the client socket. */
	{
		fprintf(stderr, "Error occured while attempting to close the client socket file descriptor.");
	}

	close(clientsocketfd); /* close client socket after closing the client socket file descriptor.*/
}