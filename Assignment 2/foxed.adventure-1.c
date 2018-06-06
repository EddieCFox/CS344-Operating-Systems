/**************************
** Filename: foxed.adventure.c
** Author: Eddie Fox
** Date: October 29, 2016
**
** Description: This program generates 7 rooms from a possible pool of 10 names, and gives
** each room a start, middle, or end type. Each room is connected to 3-6 others rooms. 
** The program generates a directory, and a room file for each room. The program is a text
** based game, and when the game begins, it reads room data from each of the rooms, and stores
** the information into structures. Each turn, the user is given their location and a list of
** connections their room has, and they try to find the end room. After finding the end room,
** a message shows that they have won, and shows how many steps it took to get to the end
** and their path to victory.
*************************/ 

#include <unistd.h> /* Needed for getpid(), which allows us to add the process id to the directory name. */
#include <stdio.h> /* Needed for the file library functions such as fgets(), fopen(), and fclose() */
#include <sys/stat.h> /* Needed for stats information of the file. */
#include <string.h> /* For various string operations such as strcpy. */
#include <stdlib.h> /* For rand and srand, in order to randomize room names, connections, etc. */
#include <time.h> /* Used to seed the randomizer based on the current time since January 1, 1970 */

/* Below I define some constants based on the assignment requirements, such as the minimum and maximum
 * number of connections, and the number of rooms. This isn't strictly necessary, as we could simply 
 * hard code the numbers, but this allows some flexibility in design for the future, so it is probably
 * good practice. */

#define NUMBER_OF_ROOMS 7 /* Each game has 7 rooms. */
#define NUMBER_OF_NAMES 10 /* The names of the 7 rooms are selected out of 10 possible names. */
#define MAX_CONNECTIONS 6 /* Each room has a maximum of 6 connections, which would connect it to each of the other 6 rooms, as a room can't be connected to itself.*/
#define NAME_BUFFER 14 /* This is probably the least intuitive of the constant definitions. 14 is the longest length of any of the 10 names, plus 1 space for the '\0' null
			* terminator character. The longest length is 13, shared between Seashore City and Castro Valley. */

/* Below, I create an array of constant character pointers to the 10 possible names that a room can have. 
 * It makes sense to make this array const because it should be impossible to change the list of names.
 * The assignment specifications say to hard code the names. I based my possible room names based off of
 * possible locations in the Rijon region in Pokemon Brown and Pokemon Prism, which are romhacks. Coincidentally,
 * there are 10 main locations, so it works out well. I suppose cities aren't really rooms, but if we think of a room as a location
 * within a larger structure, it works out, as a city is a smaller location within the bigger structure of a region. */

 char *roomNames[NUMBER_OF_NAMES] = {"Seashore City", "Merson City", "Hayward City", "Owsauri City", "Jaeru City", "Moraga Town", "Botan City", "Castro Valley", "Eagulou City", "Rijon League"};

/* Before we can create the room structure, we enumerate a custom variable type called roomType, with the three possible room types,
 * START_ROOM, END_ROOM, and MID_ROOM, as specified byt he assignment requirements. Enumeration makes sense for self-documenting purposes,
 * but also because there are only 3 possible values for the roomType variable. */

enum roomType
{
	START_ROOM,
	END_ROOM,
	MID_ROOM
};

/* We now create a room structure with name, type, connection, and number of connection attributes. Name is an array of characters, with 
 * length capped by the NAME_BUFFER constant, whose purpose is explained above. Type is from the enumerated type, and finally we have an
 * array of integers representing the connections to other rooms, capped by the max connections. I was debating between an array of integers
 * and an array of room structures, but I didn't want to add additional complexity. */

struct room
{
	char name[NAME_BUFFER]; /* Decided to not make this constant so it can be modified. Only the master list of names needs to be unmodifiable. */
				/* Length capped by the NAME_BUFFER constant above. */
	char type[100]; /* By definition has the values START_ROOM, END_ROOM, or MID_ROOM.*/ 
	int connections[MAX_CONNECTIONS]; /* While we could hard code it as "int connections[6]", I decided to use MAX_CONNECTIONS definition for future flexibility.*/
	int numberOfConnections; /* This stores the number of connections the room actually has. */
};

struct room roomsList[NUMBER_OF_ROOMS]; /* This creates an array of room structs, up to the total number of rooms as defined above. */

/* Here I place function prototypes for functions that will be used in main. */

void swapStrings(char *array[], int x, int y); /* Swaps string array elements. */
void swapInts(int array[], int x, int y); /* Swaps integer array elements. */
void createRoomFile(struct room r, int roomNumber); /* Creates room files from room structures. */
void readType(struct room *r, int roomNumber); /* Reads the type of the room from the file. */
void readConnections(struct room *r, int roomNumber);

int main(void)
{
	srand(time(NULL)); /* Seed the randomizer with the time since January 1, 1970, for standardized unique results. */


	/* First, I create the variables I will need for the game. */

	int pathTaken[1000]; /* 1000 steps to solve a 7 room maze should be enough for most, and I doubt the graders will waste time trying to exhaust this very generous limit. */
	int i, j, k; /* Loop control variables */
	int numberOfUsedRooms; /* This variable assists in creating connections between rooms, and keeps track of how many rooms have yet to be connected with the current room. */
	int processID; /* This variable stores the process id so we can make a directory including the process id. */
	char directoryName[100]; /* Used to store the directory name. */
	char inputBuffer[100];
	char *newline;

	/* This section of the code makes the directory. */

	processID = getpid(); /* Uses the getpid () function to determine the process id of the process, as we are making the directory now. */
	sprintf(directoryName, "foxed.rooms.%d", processID); /* Uses sprintf to add the process id to the assignment specified format and store it in directory name. */
	if (mkdir(directoryName, 0755) == -1) /* This attempts to call mkdir using the directory Name. The permissions 0755 allow me all permissions, and everyone else read and execute permissions, but not write. */
	{
		perror("Error. Could not create directory."); /* If there is an error, it will equal -1, and use perror to print this error message, then exit the entire program with a code of 1 */
		exit(1); /* The program needs to exit because it is dependent on being able to create the directory. */
	}

	chdir(directoryName); /* chdir used to change the working directory of the program. */

	/* This section of the code assigns a name and type to each of the 7 rooms.
	 * I was trying to think of how I could uniquely assign names without re-using any.
	 * In a for loop, I decide to create a random number from 0 to 9, corresponding to the indexes of the roomNames array.*/

	for (i = 0; i < NUMBER_OF_ROOMS; i++) /* Perform this procedure to assign a name to each room. */
	{
		int randomNumber = ((rand() % NUMBER_OF_NAMES));
		char *chosenName; /* create a variable to store the name that will eventually be chosen for the room. */

		swapStrings(roomNames, randomNumber, (NUMBER_OF_NAMES - i - 1)); /* Swap the element of the randomized number with numberOfNames - i - 1. This ensures
										  * randomized names are not reused. */
		chosenName = roomNames[NUMBER_OF_NAMES - i - 1]; /* After the above swapping, we store the chosen name into, well, the chosenName variable. */
		strcpy(roomsList[i].name, chosenName); /* Copies the chosen name into the name field of the room structure. */
		strcpy(roomsList[i].type, "MID_ROOM");  /* Initially, we are setting the type of each room to MID_ROOM, and will manually change 2 of the rooms to START_ROOM and END_ROOM types.	*/
	}

	strcpy(roomsList[0].type, "START_ROOM");  /* Set the first room in the list of rooms to have the START_ROOM type. */
	strcpy(roomsList[NUMBER_OF_ROOMS - 1].type, "END_ROOM");  /* Set the final room in the list of rooms to have the END_ROOM type. */

	/* In this section, we create the connections for each room. For each room, we create an array of unused room numbers, and keep a count of how many rooms haven't been used.
	 * For each room, the array will give the numbers of all the unused rooms. Then, up until the number of connections, elements from the unused array are randomly connected to the main room. */

	for (i = 0; i < NUMBER_OF_ROOMS; i++)
	{
		int unusedRooms[NUMBER_OF_ROOMS]; /* create an array to hold all the rooms that the current room hasn't connected to. */
		int numberOfUnusedRooms = 0; /* Set the initial number of unused rooms to 0, until we calculate the total for the given room. */

		for (j = 0; j < NUMBER_OF_ROOMS; j++) /* Using a nested loop, we fill the unusedRooms array with every room number but the current room. */
		{
			if (j != i)
			{
				unusedRooms[numberOfUnusedRooms] = j; /* Goes through every room and adds it to the array unless it is the current room. */
				numberOfUnusedRooms++; /* Increment the count of used rooms by 1. */
			}
		}

		/* Now that the array of unused rooms for each room has been filled, we exit the nested loop and back into the initial loop. Here we determine the number of connections and
		 * randomly assign room numbers as connections from the unUusedRooms array. */

		int numberOfConnections = ((rand() % 4) + 3); /* Sets the number of connections to a random number between 3 and 6. */
		roomsList[i].numberOfConnections = numberOfConnections; /* Sets the number of connections in the structure of the given room to the randomly determined number. */

		/* We now create a second loop using j again (set to 0) in order to populate the connections of a room, randomly from the pool of unused rooms that a particular room has. */

		for (j = 0; j < roomsList[i].numberOfConnections; j++)
		{
			int randomNumber = rand() % numberOfUnusedRooms; /* This will generate a number between 0 and the number of rooms (besides the current room), which corresponds perfectly with
									* the indexes of the unusedRooms array. */
			roomsList[i].connections[j] = unusedRooms[randomNumber]; /* Populate the connections array with a random element from the unusedRooms array. */
			swapInts(unusedRooms, randomNumber, numberOfUnusedRooms - 1); /* Swap between the random number and the last index that hasn't been inolved in the swaps yet.
											   * This helps to prevent duplicate connections. */
			numberOfUnusedRooms--; /* Since a connection has been made, decrease the number of unused rooms by one, as a connection makes a room used. */
		}
	}

	/* We now add a second for loop in order to ensure that the connections are two way. For example, if Seashore City is connected to Hayward City, we must also
	 * ensure that Hayward City is connected to Seashore City. We actually need to do a triple nested loop in order to accomplish this. */

	int connected = 0; /* This is a boolean variable that is 0 if there isn't a 2 way connection, and 1 if there is. By default, we keep it at 0. */
	int adjacentRoom; /* This variable will be used to represent the room next to the current room that we are checking for the 2 way connection. */


	for (i = 0; i < NUMBER_OF_ROOMS; i++) /* For each room. */
	{
		for (j = 0; j < roomsList[i].numberOfConnections; j++) /* For each connection in each room. */
		{
			adjacentRoom = roomsList[i].connections[j]; /* Check each connection to each room. */
			for (k = 0; k < roomsList[adjacentRoom].numberOfConnections; k++) /* For each adjacent room, check all of their connections. */
			{
				if (roomsList[adjacentRoom].connections[k] == i) /* If the adjacent room has a connection to the original room, set the connected variable to true. */
				{
					connected = 1;
				}
			}

			if (connected == 0) /* If the adjacent room has no connection to the original room, we add one. */
			{
				roomsList[adjacentRoom].connections[roomsList[adjacentRoom].numberOfConnections] = i; /* Form a connection between the adjacent room and the original room. */
				roomsList[adjacentRoom].numberOfConnections++; /* Increase the number of connections for the adjacent room by 1. */
			}
		}
	}

	for (i = 0; i < NUMBER_OF_ROOMS; i++) /* For each room, create a room file. */
	{
		createRoomFile(roomsList[i], i + 1);
	}

	int currentRoom; /* Holds the index of the current room. */
	int nextRoom; /* Holds the index of the next room. */
	int endRoom; /* Holds the index of the final room. */

	for (i = 0; i < NUMBER_OF_ROOMS; i++)
	{
		readType(&roomsList[i], i + 1); /* For each room, read the type from the file. */

		if (strcmp(roomsList[i].type, "START_ROOM") == 0) /* If the read room is the starting room, set the starting room as the current room. */
		{
			currentRoom = i;
		}

		if (strcmp(roomsList[i].type, "END_ROOM") == 0) /* When the end room is found, set that as the end room. */
		{
			endRoom = i;
		}

		readConnections(&roomsList[i], i + 1); /* Read the connections of the room. */
	}

	int stepCount = 0; /* Will record how many steps are taken on the journey. */

	for (;;)
	{
		printf("CURRENT LOCATION: %s\n", roomsList[currentRoom].name); /* Print current location then the current rooms name, and new line. */
		printf("POSSIBLE CONNECTIONS: "); /* Write "POSSIBLE CONNECTIONS: */
		for (i = 0; i < roomsList[currentRoom].numberOfConnections; i++)
		{
			printf("%s", roomsList[roomsList[currentRoom].connections[i]].name); /* For each connection, write the name of the connection. */
			if (i == roomsList[currentRoom].numberOfConnections - 1) /* If it is the last connection, add a period, then a new line. */
			{
				printf(".\n");
			}

			else /* If it isn't the last connection, add a period, then a space. */
			{
				printf(", ");
			}
		}

		printf("WHERE TO? >"); /* Prints the where to prompt, with the cursor right outside the arrow key. */
		fflush(stdout); /* Flush the standard output to properly read the input from the user. */

		if (fgets(inputBuffer, 100, stdin) == NULL) /* Read input from user. */
		{
			printf("\n");
			exit(0);
		}

		newline = strchr(inputBuffer, '\n');
		*newline = '0'; /* Replace newline from read string with null terminator. */
		printf("\n"); /* Print new line anyway to space out the text. */

		for (i = 0; i < NUMBER_OF_ROOMS; i++)
		{
			if (strcmp(inputBuffer, roomsList[i].name) == 0) /* Searches each room's name structures for a string matching the name of the connection. */
			{
				nextRoom = i; /* If matching room found, set the nextRoom equal to the room that corresponds to the name of the connection. */
			}
		}

		for (i = 0; i < NUMBER_OF_ROOMS; i++)
		{
			if (strcmp(roomsList[roomsList[currentRoom].connections[i]].name, inputBuffer))
			{
				nextRoom = -1; /* If the strings of any room aren't equal to the string from the input buffer, set the next room as invalid.*/
			}
		}

		if (nextRoom == -1)
		{
			printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
		}

		else
		{
			currentRoom = nextRoom;
			pathTaken[stepCount] = currentRoom;
			stepCount++;

			if (currentRoom == endRoom)
			{
				printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
				printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", stepCount);

				for (i = 0; i < stepCount; i++)
				{
					printf("%s\n", roomsList[pathTaken[i]].name);
				}

				exit(0); /* Exit with a code of 0 if successful.*/
			}
		}
	}
}

/* The swapStrings function applies the basic swap scenario to arrays of c-string elements, using the 
 * classic Temp = A, A = B, B = Temp formula. */

void swapStrings(char *array[], int x, int y)
{
	char *temp; /* Create a temporary character pointer.  */
	temp = array[x]; /* Assign temp the value of array element X */
	array[x] = array[y];  /* Set array element x to the value of array element y */
	array[y] = temp; /* Give array element y the value that array x used to have by assigning it the value of temp. */
	return;
}

void swapInts(int array[], int x, int y) /* Here, we perform the same procedure that we did in the above swapStrings function, just applying to ints instead of char pointers. */
{
	int temp; 
	temp = array[x];
	array[x] = array[y];
	array[y] = temp;
	return;
}

void createRoomFile(struct room r, int roomNumber) /* This function writes the room files that are used by the program. */
{
	FILE *filePointer; /* Holds a pointer to the file. */
	int i; /* Standard loop control variable. */
	char roomName[2]; /* Holds a single digit number plus null terminator. */
	sprintf(roomName, "%d", roomNumber); /* Takes the room number passed in by the parameter and converts it into a string, then stores the result in the roomName char array. */
	
	filePointer = fopen(roomName, "w"); /* Creates a file name */
	if (filePointer == NULL) /* filePointer will be null in the case of an error. */
	{
		perror("Error: could not create room file."); /* Creates error message if couldn't create file. */
		exit(1); /* Exit with a code of 1. */
	}

	fprintf(filePointer, "ROOM NAME: %s\n", r.name); /* Access the passed in room structures name field and print that to the file. */
	
	for (i = 0; i < r.numberOfConnections; i++)
	{
		fprintf(filePointer, "CONNECTION %d: ", i + 1); /* For each connection, we are printing it to the file, but starting at i + 1 because 
								 * we are starting at connection #1, but i starts at 0. */
		if (roomsList[r.connections[i]].name == NULL) /* If there is no name for the connection, just continue on. */
		{
			continue;
		}
		
		fprintf(filePointer, "%s\n", roomsList[r.connections[i]].name); /* Print the name of the connection, then add a newline.*/
	}
	
	fprintf(filePointer, "ROOM TYPE: %s\n", r.type); /* Print the room type of the room structure. */
	fclose(filePointer); /* Close the file pointer. */
	return;
}

void readType(struct room *r, int roomNumber) /* Reads the room type from the room file. */
{
	FILE *filePointer; /* creates file pointer. */
	int i; /* standard loop control variable. */
	char roomName[2]; /* Holds a single digit number plus null terminator. */
	char readBuffer[100]; /* Holds what is read from the file for use by the main program. */
	char *newline; /* Holds the location of the newlines in the strings so that they can be converted to null terminators. */
	
	sprintf(roomName, "%d", roomNumber);
	filePointer = fopen(roomName, "r"); /* Opens the room file in read mode. */
	if (filePointer == NULL)
	{
		perror("Error: could not read room file."); /* Creates error message if couldn't read room file. */
		exit(1);
	}

	while (fgets(readBuffer, 100, filePointer) != NULL) /* While characters from the file can still be read into the buffer. */
		if (strncmp(readBuffer, "ROOM NAME", 9) == 0) /* If Room name is found in the file. */
		{
			strcpy(r->name, readBuffer + 11); /* Skips ROOM NAME and reads the name of the room. */
			newline = strchr(r->name, '\n');
			*newline = '\0'; /* This line and the above one replace newline with a null character. */ 		
		}
		
		else if (strncmp(readBuffer, "ROOM TYPE",9) == 0) /* Else if room type is found in the file. */
		{
			strcpy(r->type, readBuffer + 11); /* Skips ROOM TYPE and reads the type of the room into the structure. */
			newline = strchr(r->type, '\n');
			*newline = '\0'; /* This line and the above one replace newline with a null character. */
		}
}

void readConnections(struct room *r, int roomNumber) /* Reads the connections from a room file. */
{
	FILE *filePointer; /* Creates file pointer. */
	int i; /* Standard loop control variable. */
	int connectionIndex = 0; /* Holds the index of the read connection. */
	int roomIndex; /* holds the room index for read connection names. */
	char roomName[2]; /* Holds a single digit number plus null terminator. */
	char readBuffer[100]; /* Holds what is read from the file for use by the main program. */
	char *newline; /* Holds the location of the newlines in the strings so that they can be converted to null terminators. */

	sprintf(roomName, "%d", roomNumber); 
	filePointer = fopen(roomName, "r"); /* Opens room file in read mode. */
	
	if (filePointer == NULL)
	{
		perror("Error: could not read room file."); /* Creates error message if couldn't read room file. */
		exit(1);
	}

	while (fgets(readBuffer, 100, filePointer) != NULL) /* While characters from the file can still be read into the buffer. */ 
		{
			if (strncmp(readBuffer, "CONNECTION", 9) == 0) /* If connection is found as a string in the file. */
			{
				char connectionName[100]; /* Allocates space to hold the read connection string from the buffer. */
				strcpy(connectionName, readBuffer + 14); /* Skips "CONNECTION: " */
				newline = strchr(connectionName, '\n');
				*newline = '\0'; /* Replaces the newline in the read string with the null terminator. */

				for (i = 0; i < NUMBER_OF_ROOMS; i++)
				{
					if (strcmp(connectionName, roomsList[i].name) == 0) /* Searches each room's name structures for a string matching the name of the connection. */
					{
						roomIndex = i; /* If matching room found, set the roomIndex equal to the room that corresponds to the name of the connection. */
					}
				}
				r->connections[connectionIndex] = roomIndex; /*Sets the connection of the room to the index corresponding to the room corresponding to the read name of the connection from the file. */
				connectionIndex++; /* Increments connectionIndex by 1 in order to read and assign other connections from the file. */
			}
		}

	fclose(filePointer); /* Closes the file after use. */
}