
# Author: Eddie C. Fox
# Date: November 26, 2016

# This script creates 3 files, each containing 10 random lowercase letters with no spaces. 
# After printing the contents of the 3 files, it will generate two random numbers between 1 and 42, and print them out. 
# Finally, the script prints out the product of the two random numbers. 

# First, we import the modules we need for random numbers, and for dealing with strings.

import string # Imports the string module
import random # Imports the random module.


random.seed() # Seeds the random number generator using the system time. 

# How many files we are creating 

numberOfFiles = 3;

# How long each file will be 

fileLength = 10;

print ("\n") # Prints a newline.
print ("Now creating 3 files, with 10 random letters each.") # Prints the explanatory statement in quotes.
print ("\n") # Print another newline

# Here, we create a function that can be called to generate a string of random lower case letters of any length.
# The reason we create a function here is so that we odn't have to do a nested loop, which I feel uncomofrotable doing in python. 
# The loop will run 3 times, one for each file being created, and it will call the function each time. 

# In this function, we create an empty string, and append to it a randomly chosen letter chosen from the ascii lowercase letters.
# The function does this as many times as the length parameters indicates, so it is more flexible if we extend the program. In this case, we will pass it the parameter 10. 

def generateRandomString(length):
	return ''.join(random.choice(string.ascii_lowercase) for i in range(length))	

# Now we create the for loop described above, where we perform the operations of each file. 

for i in range(numberOfFiles):

	randomFileString = generateRandomString(fileLength) # We create a randomFileString variable to hold 10 random lower case letters
	
	# We create a variable to hold the file name, as it will be printed explicitly as such. 

	fileName = "file" + str(i+1 ) + ".txt" # Each file will be called something like file1.txt, file2.txt, file3.txt etc. we use str on the i variable to convert it to a string for concatenation.
	filePointer = open(fileName, 'w') # Create a file named identically to the fileName variable and open it in write mode. 
	filePointer.write(randomFileString) # Here we write the random file string to the file. 
	filePointer.close() # close the file after done. 

	print("File: " + fileName + " contents are: " + randomFileString) #It will print something roughly like this: File: file1 contents are: thrisodpoa
	print ("\n") # Print a newline. 


# Now that we are done with the file section, we move onto the random numbers section. We generate two random numbers between 1 and 42, and hold them in two number variables. 
# Then we print the random numbers and have a third variable for the product of the two random numbers, and print that out. 

firstRandomNumber = random.randint(1, 42) # Use the randint aspect of the random module to generate a random number between 1 and 42. 
secondRandomNumber = random.randint(1, 42) # We do the same process here that we did for the first one. 

print ("The first number is: " + str(firstRandomNumber)) # Print the first random number. Append the string converted form of the first random number.
print ("\n") # Print newline.

print ("The second number is: " + str(secondRandomNumber)) # Print out the second random number. Append the string ocverted form of the second random number. 
print ("\n")

numberProduct = firstRandomNumber * secondRandomNumber # multiply the two numbers and store it. 

print ("The product of " +  str(firstRandomNumber) + " and " + str(secondRandomNumber) + " is " + str(numberProduct)) #Print the product of the two numbers. 
print ("\n") # Print a newline. 
	
