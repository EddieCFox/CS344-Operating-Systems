/**************************
** Filename: smallsh.c
** Author: Eddie Fox
** Date: November 17, 2016
**
** Description: This program is a shell written in C language. It has 3 built in commands, cd, exit, and status.
** cd and exit are fairly self explanatory. status prints out either the exit status or the terminating signal of
** the last foreground process that has been run. Beyond this, it can also execute various programs, if program 
** names and parameters are entered properly without syntactical errors. It has supports for comments, when command 
** lines begin with #. It can redirect standard input and standard outputs, and also run processes in both the foreground 
** and background. The shell can also interupt processes with a CTRL-C command from the keyboard. The shell prints error 
** messages as appropriate. The shell also prints out the process id of background processes when they begin, and prints 
** out when they are complete. Whenever a child foreground process is killed, the parent will print out the number of the 
** signal that killed it. 
*************************/

#include <stdio.h> /* Needed for things like printf, fgets, sprintf and perror. */
#include <stdlib.h> /* Needed for things such as malloc, execvp, and exit. */
#include <string.h> /* For various string operations such as strcmp and strtok. */
#include <unistd.h> /* Needed for various directory related commands, like getpid, fork, exec, and the pid_t type. */
#include <signal.h> /* Needed for almost everything to do with sigaction, including the structure, and various
					   signal set related options. */

#include <fcntl.h> /* Used for opening files in read and write modes.*/
#include <sys/stat.h> /* Provides various status information. */
#include <sys/wait.h> /* Used for waitpid(). */
#include <sys/types.h> /* provides definition for data types ssize_t and pid_t.*/

/* Here I place some variables before the main function, that will be used. */

/* The main function will have the shell contained within a while loop. timeToQuit is basically a boolean that only turns
   true (has a value of 1) when it is time for the shell to quit for good. Otherwise it will have the value of 0 (false). */
int timeToQuit = 0;

/* This holds a boolean that is false by default, but if the input getting function has nothing, becomes true, so that the main shell loop knows
to quit the current loop and move on to the next at the very beginning, as there is nothing to process if the input is blank. */
int inputWasNull = 0; 

/* For our final boolean value, we have the background variable. Background is set to 1 if the process in question is a background process, and
   0 if the process is a foreground process. By default, we initialize it to 0, because a process needs to explicitly be put in the background
   with an ampersand at the end of the command line.*/

int background = 0;

char inputLine[2048]; /* According to the assignment requirements, the shell must support command lines of up to 2048 characters, which explains the size.
					     This is a c-string that holds the command line input from the user.*/

int backgroundProcesses[250]; /* This is an array of integers that keeps track of the various processes running in the background, storing the process ID's of each background process. 
							     Technically, this can be overrun if there are over 250 background processes, but I didn't want to 
								 deal with the memory issues. It is hard to actually exceed 250 background processes on accident. 
								 I came up with this estimate by counting how many processes were running on my computer (120), and roughly 
								 doubled that to be safe.*/

int i; /* Loop control variable. Only one needed because no nested loops here. */
int statusStatus; /* Not to be confused with the various local status variables, this variable holds the integer printed if the status command
				    is used. The name might be a bit confusing, but it's funny. It holds the exit status or termination signal of the last foreground command.*/

int backgroundProcessNumber; /* Simple integer that stores the current number of background processes active in the shell.*/

/* Here I forward declare two helper functions that I will be using as part of the main function, processInput() and checkProcesses().*/

void processInput(void);
void checkProcesses(void);

int main()
{
	/* At first, I was debating wether or not to make this a do-while loop, but I realized that since I initialized timeToQuit as 0, 
	   it would be guaranteed to execute at least once anyway. */

	while (timeToQuit == 0)
	{
		/* To kick things off, we run the two helper functions, which checks background functions that have terminated, before giving the user control, and prints
		   out the relevant information. processInput() then takes the users command line argument, and formats it in a string that the numerous conditionals in the 
		   while loop can process using strcmp. */

		checkProcesses(); 
		processInput();

		if (inputWasNull == 1)
		{
			return 0; /* If the user didn't enter anything for the input besides hitting the enter button, return and move on to the next iteration of the loop. */
		}

		else if (strcmp(inputLine, "exit") == 0) /* If they entered exit.*/
		{
			timeToQuit = 1;
			return 0; /* As timeToQuit is now 1, it will exit the while loop and end the shell properly. */
		}

		else if (strstr(inputLine, "#")) /* If the beginning of the input is a hashtag (#), it is a comment, and we ignore it. */
		{
			printf("\n You entered a comment. Ignoring. Please try again.\n");
			fflush(stdout); /* Flush for safety */
			continue; /* Skip to the next iteration of the loop. */
		}

		else if (strcmp(inputLine, "status") == 0) /* If they entered status.*/
		{
			printf("exit value %d", statusStatus); /* Here we print the exit status or terminating signal of the last foreground process.*/
		}

		/* strncmp compares two strings up to the length specified in the third parameter. In this case, we are taking cd and comparing it to the 
		   command that the user entered, but only up to the first two letters of cd, because line will have further characters that denote what 
		   directory they wish to change into. */
		else if (strncmp("cd", inputLine, strlen("cd")) == 0) 
		{
			/* This searches the third character of what the user inputted. If they entered a parameter, then there will naturally be a space after cd,
			   so we can check if they entered something by checking for this space. */

			if (inputLine[2] == ' ') 
			{
				char workingDirectory[2045]; /* This creates a c-string that can hold the working directory the user entered. Because the max length of a user input is 2048,
											    and the user already used 3 characters to type "cd ", the most characters they can use is 2045 to specify a working directory.*/
				getcwd(workingDirectory, sizeof(workingDirectory)); /* Reads the current working directory. */

				char *newDirectoryPath; /* Creates a c-string to hold the new directory path.*/
				newDirectoryPath = strstr(inputLine, " "); /* Creates a substring on the inputted line up to the next space. */

				if (newDirectoryPath) /* If the substring isn't NULL, this is condition fires off.*/
				{
					/* Because the substring entered only goes up to the first space, we need to move up one space to actually reach the start of the argument for cd. */
					newDirectoryPath += 1; 
					char *pathValue; /* Value will store the value of the newDirectoryPath. */
					
					pathValue = malloc(strlen(newDirectoryPath)); /* Dynamically allocates memory for the new directory path. */
					memcpy(pathValue, newDirectoryPath, strlen(newDirectoryPath)); /* Copy the value of newDirectorypath into pathValue, copying as many bytes as the length of newDirectoryPath.*/
					*(pathValue + strlen(newDirectoryPath)) = 0; /* Add a null terminator to the end. */
					sprintf(workingDirectory, "%s/%s", workingDirectory, pathValue); /* Combine the current working directory with the value of the new path the user specified to 
																				        create a full directory to change into. This final result is stored in workingDirectory. */
					free(pathValue); /* Free the memory used when dynamically allocating pathValue. */
				}

				chdir(workingDirectory); /* Change into the new working directory. */
			}

			else /* If this is reached, then no space was entered after cd, meaning the user just entered cd by itself.
					In this case, we just change to the home directory, as defined by the environmental variables.*/
			{
				char *homeLocation = getenv("HOME"); /* Grab the HOME environment variable using getenv.*/
				chdir(homeLocation); /* Change the current working directory to the location of the home environment variable.*/
			}
		}

		/* If we've reached this point, the input is neither NULL, a comment, or built in, meaning we need to execute some
		   other program through the shell, either in the background or the foreground. */
		else 
		{
			pid_t processID; /* Creates a pid_t type variable to hold the process id. */
			int status; /* Local status variable.*/

			char *command; /* Holds the command the user entered. */
			char *commandArguments[512]; /* Holds every argument in an array of c-strings. 512 is the max as per the assignment specifications.*/
			int numberOfArguments; /* Holds the number of arguments. */
			int redirectionInvolved = 0; /* This is a boolean variable that accounts for wether or not the process involves redirection. By default, it is set to 0.*/

			command = strtok(inputLine, " "); /* Reads the command line input up to the first space. */
			if (command == NULL)  /* If this is reached, there is no command. */
			{
				continue; /* Move on to the next iteration of the shell loop.*/
			}

			commandArguments[0] = command; /* Set the first command argument to command, which now is set to the first argument, so it works out.*/
			numberOfArguments = 1;
			commandArguments[numberOfArguments] = strtok(NULL, " "); /* Place the next argument into the next space of the array. */

			while (commandArguments[numberOfArguments] != NULL) /* While there are still arguments to place in.*/
			{
				numberOfArguments++; /* Increment the number of arguments. */
				commandArguments[numberOfArguments] = strtok(NULL, " "); /* Read in arguments into the arguments array until there are none left.*/
			}

			if (background == 1) /* If the user intended to start the process in the background.*/
			{
				if ((processID = fork()) < 0) /* If the fork went wrong, it will return a status below 0, like -1. This is what we are checking for.*/
				{
					perror("An error occured while forking."); /* Write the error message.*/
					exit(1); /* Exit.*/
				}

				if (processID == 0) /* If the process id equals 0, then this is the child process.*/
				{
					for (i = 0; i < numberOfArguments; i++) /* For every argument*/
					{
						if (strcmp(commandArguments[i], "<") == 0) /* See if one of the arguments is <, if so, we need to redirect input.*/
						{
							if (access(commandArguments[i + 1], R_OK) == -1) /* If the file involved with redirection cannot be read,*/
							{
								printf("Cannot open %s to redirect input.\n", commandArguments[i + 1]); /* Print so*/
								fflush(stdout); /*flush for safety. */
								redirectionInvolved = 1; /* and set redirection as involved through the boolean variable.*/
							}

							else /* Otherwise, the file can be read.*/
							{
								int fileDescriptor = open(commandArguments[i + 1], O_RDONLY, 0); /* Create a file descriptor and open the argument after the < symbol in Read only mode.*/
								dup2(fileDescriptor, STDIN_FILENO); /* Redirect the input via duplication. */
								close(fileDescriptor); /* Close file descriptor.*/
								redirectionInvolved = 1; /* set redirection as involved through the boolean variable. */
								execvp(command, &command); /* Execute the command. */
							}
						}

						if (strcmp(commandArguments[i], ">") == 0) /* See if one of the arguments is >, if so, we need to redirect output. */
						{
							int fileDescriptor = creat(commandArguments[i + 1], 0644); /* Creates a file to output to.*/
							dup2(fileDescriptor, STDOUT_FILENO); /* Redirects the output via duplication.*/
							close(fileDescriptor); /* Close file descriptor.*/
							redirectionInvolved = 1; /* set redirection as involved through the boolean variable. */
							execvp(command, &command); /* Execute the command. */
						}
					}

					if (redirectionInvolved == 0) /* If there is no redirection*/
					{
						execvp(command, commandArguments); /* Execute the command, using all the command arguments. */
					}

					/* This point can only be reached if there is an error. */
					printf("Some error occured.\n");
					fflush(stdout); /* Flush for safety*/
					exit(1); /* Exit with an error. */
				}

				else /* If it isn't a child, it must be a parent process.*/
				{
					int status; /* Local status variable.*/
					int process;  /* Holds the result of doing the waitpid command. */
					printf("background pid is %d\n", processID); /* Write the proccess id of the parent background process that is being started.*/
					fflush(stdout); /* Flush for safety*/
					backgroundProcesses[backgroundProcessNumber] = processID; /* Add the process ID of the newly started background process to the array of background processes.*/
					backgroundProcessNumber++; /* Increment the number of background processes after adding the process id to the array.*/
					process = waitpid(processID, &status, WNOHANG); /* Performs the waitpid command on the background process with no delay. */
					continue; /* Head to the next loop of the shell, as this process is running in the background. */
				}
			}

			else /* If this is reached, the process the user intends to execute using the shell must be a foreground one.*/
			{
				if ((processID = fork()) < 0) /* If the fork went wrong, it will return a status below 0, like -1. This is what we are checking for.*/
				{
					perror("An error occured while forking."); /* Write the error message.*/
					exit(1); /* Exit.*/
				}

				if (processID == 0) /* If the foreground process to be started is a child process.*/
				{
					/* Note, the following is an exact copy paste of the code I put above, because input and output redirection for a child
					   process works exactly the same, regardless if the child process was started in the background or foreground.*/

					for (i = 0; i < numberOfArguments; i++) /* For every argument*/
					{
						if (strcmp(commandArguments[i], "<") == 0) /* See if one of the arguments is <, if so, we need to redirect input.*/
						{
							if (access(commandArguments[i + 1], R_OK) == -1) /* If the file involved with redirection cannot be read,*/
							{
								printf("Cannot open %s to redirect input.\n", commandArguments[i + 1]); /* Print so*/
								fflush(stdout); /*flush for safety. */
								redirectionInvolved = 1; /* and set redirection as involved through the boolean variable.*/
							}

							else /* Otherwise, the file can be read.*/
							{
								int fileDescriptor = open(commandArguments[i + 1], O_RDONLY, 0); /* Create a file descriptor and open the argument after the < symbol in Read only mode.*/
								dup2(fileDescriptor, STDIN_FILENO); /* Redirect the input via duplication. */
								close(fileDescriptor); /* Close file descriptor.*/
								redirectionInvolved = 1; /* set redirection as involved through the boolean variable. */
								execvp(command, &command); /* Execute the command. */
							}
						}

						if (strcmp(commandArguments[i], ">") == 0) /* See if one of the arguments is >, if so, we need to redirect output. */
						{
							int fileDescriptor = creat(commandArguments[i + 1], 0644); /* Creates a file to output to.*/
							dup2(fileDescriptor, STDOUT_FILENO); /* Redirects the output via duplication.*/
							close(fileDescriptor); /* Close file descriptor.*/
							redirectionInvolved = 1; /* set redirection as involved through the boolean variable. */
							execvp(command, &command); /* Execute the command. */
						}
					}

					if (redirectionInvolved == 0) /* If there is no redirection*/
					{
						execvp(command, commandArguments); /* Execute the command, using all the command arguments. */
					}

					/* This point can only be reached if there is an error. */
					printf("Some error occured.\n");
					fflush(stdout); /* Flush for safety*/
					exit(1); /* Exit with an error. */
				}

				else /* If this point is reached, the process intended to start is a foreground parent process.*/
				{
					int status; /* Local status variable.*/
					waitpid(processID, &status, 0); /* Wait for the process to end.*/

					if (WIFEXITED(status)) /* If the process exited with a status*/
					{
						statusStatus = WEXITSTATUS(status); /* Set the statusStatus equal to the exit status. This will be called in the coming
															   shell loops until another foreground loop is run, at which point the statusStatus
															   will be overwritten with the new status. */
					}
				}

			} 

		} 

		signal(SIGINT, SIG_IGN); /* If an interrupt signal is found, catch and ignore it. */
	} 

	return 0; /* Do everything to exit the shell properly. */
}

/****************************
**                                          void processInput(void)
** Description: This function takes the command line input from the user and takes out the newline, replacing
** it with a null terminator. I have done this numerous times in CS 261, and it is no different here. The string 
** that results from this function is later processed through multiple conditional statements within the while 
** loop of the shell, using strcmp to check for various conditions and characters in the users command line input. 
****************************/

void processInput(void)
{
	/* First, we flush the standard input and standard output streams to be safe. */
	fflush(stdout);
	fflush(stdin);

	printf(": "); /* Here we print a colon symbol, because the assignment requirements say it must be the prompt for each command line.*/

	/* Flushing standard output and input again to be safe.*/
	fflush(stdout); /* Warning, this line was not in the original code. Delete this line if the program doesn't work. Also delete this comment if the program does work with it. */
	fflush(stdin);

	inputWasNull = 0; /* We set inputWasNull back to 0, because otherwise after the first blank input, it would permanently be set to 1. */

	if (fgets(inputLine, sizeof(inputLine), stdin) != NULL) /* Run fgets on the null line*/
	{
		char *position; /* This character pointer will store the position of the new line inevitably entered after the command line in the shell.*/
		position = strchr(inputLine, '\n'); /* Set position to the newline's position. */

		*position = '\0'; /* Dereference the position pointer to change the newline to a null terminator.*/

		/* We now re-appropriate position and use strchr again, searching for & this time. If there is an & in the command line,
		   it indicates that the user intends to run the process in the background, and we can thus set the boolean variable accordingly. */
		
		position = strchr(inputLine, '&'); /* Sets position to a character pointer with the position where there is a & in the line, if any. */
		if (position != NULL) /* For it to not be NULL, there must be a & somewhere in the command line. */
		{
			*position = '\0'; /* Like before, we can dereference the pointer and change it to a null terminator in order to remove it from the commmand line.
							     That way, the & won't interfere with the conditionals that process the input. Having two null terminators right next to each 
								 other doesn't matter because the string is terminated after the first one is read. */
			background = 1; /* Background boolean is set to true, which assigns the processing of the command line to the proper conditionals. */
		}

		else /* If this is reached, there is no & at the end of the line, meaning the user intends to run the process in the foreground. */
		{
			background = 0;
		}
	}

	else /* If this is reached, the user didn't enter anything in the shell, and just hit enter. inputWasNull becomes 1 to reflect this.*/
	{
		inputWasNull = 1;
	}

	return; /* Since it is a void function, it doesn't actually need to return anything. */
}

/****************************
**                                                      void checkProcesses(void)
** Description: This function runs through the array of background processes in order to check if background processes have ended
** after each loop of the shell, before giving control back to the user. If a process has exited, it is printed as such. 

DEBUG NOTES: It is possible that I need to make status a pointer. Try this if the shell doesn't work properly. If you change this, change
&status in the waitpid function to status.
****************************/

void checkProcesses(void)
{
	int status; /* Local status variable for the function. */

	for (i = 0; i < backgroundProcessNumber; i++) /* This for loop runs through the array of background process id's and checks if they have terminated between loops. */
	{
		/* Here we use &status instead of just status to fit the parameter requirements for waitpid. WNOHANG causes the waitpid to return information on the process
		   immediately, without waiting for it to terminate. Otherwise, waitpid would be blocked until status information is avaliable. If there is no information,
		   waitpid() will return 0 in this case. Because we are checking for background processes that have exited, we create a condition statement to check for status
		   that are greater than 0. */

		if (waitpid(backgroundProcesses[i], &status, WNOHANG) > 0)
		{
			/* If the background process is terminated by an unhandled exception, WTERMSIG holds the numer of the signal that terminated the process.
			We print out this number.*/
			if (WTERMSIG(status))
			{
				printf("background pid %d is done: terminated by signal %d", backgroundProcesses[i], WTERMSIG(status));
				fflush(stdout); /* Flusing to be safe.*/
			}
			
			/* If the process exited normally, WIFEXITED will default to a true value instead of 0. Therefore, this conditional is
			   only executed if the processes has actually exited. */
			if (WIFEXITED(status)) 
			{
				/* If the background process exited, print out as such, with the process id and the exit status code. */
				printf("background pid %d is done: exit value %d\n", backgroundProcesses[i], WEXITSTATUS(status)); 
				fflush(stdout); /* Flushing stdout with every print statement as reccomended in the hints.*/
			}	
		}
	}

	return; /* Return nothing because void function. */
}
