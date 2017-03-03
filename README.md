# udpfileserver
UDP based file server in C

UDP fileserver

This project submission contains two .c files that can be compiled to run a UDP fileserver.

It implements the following commands:
	1. ls
	2. get(filename)
	3. put(filename)
	4. exit

---server.c---
Once compiled using the makefile an executable server is created. The arguments passed are:
	1. Port Number

When executed the port number is mentioned as a command line argument.
Example: ./server 5555

The server executable does not ask for input. All commands are to be passed in the client 
executable file.

---client.c---
The compiled client.c generates an executable client that can be run by passing the following
arguments:
	1. IP address of server
	2. Port number

The executable file is run by passing the IP address of the server as the first argument and the port
number as the second argument.
Example: ./client 123.225.79.44 5555

Commands:

1. LS
When command "ls" is passed the client sends the command as it is to the server which runs the system
function that passes a command line argument that lists all files in the current directory of the server
and redirects the result to a file named "ls_values.txt". The server then reads the contents of the 
files and sends them to the client via the socket. The usage is as follows: 

ls

2. GET
This command is used to copy a file from the server to the client. The command is used as follow:

get(filename)

The client sends this command as is to the server as well. Once this command is received the 
client and server both tokenize the command to separate the command and the filename. So if 
get(foo1) is passed, they tokenize the commands to get and foo1. Both of the extracted tokens
are stored in variables that are used to open files and enter specific loops. Tokens are created
using the strtok() function and the delimiters used are "(" and ")".

The server then opens the specific file that is passed and packetizes the contents in to packets
of MAXBUF size. These packets are sent via the socket to the client where the client opens a file
on binary write mode. The filename of the copied file is of the form "copied_filename" to distinguish
between the original and the copied file.
Example: If get(foo2) is passed the resultant file in the client directory is copied_foo2

Once the transfer is done the files are closed on both sides and the next command is asked.

3. PUT
This command copies a file from the client directory to store it in the server directory. The usage
is as follows:

put(filename)

The client send the command to the server as it is. The client and server tokenize the commands to
get the filenames as mentioned above. The server opens a file named "copied_filename" to store the 
incoming packets into the server directory.

The put process returns a message that the put operation has succeeded to the client side once the 
transfer is complete.

Once completed the client asks for the next command to be executed.

4. EXIT
This command exits both client and server executions by executing exit(1). The usage is as follows:

exit

RELIABILITY

To implement reliability of packet transfer the stop and wait protocol is used. Only one packet is 
in transit at a given time. The server or client initiate the next packet only when the receipt of
the previous packet has been acknowledged. 

Once a packet is sent and received the receiver sends an ACK in form of the integer "1"

If packet loss occurs and nothing is received the receiver sends a NACK in form of the integer "0"

At the sender if no ACK or NACK is received the sender waits for 25 ms and then resends the packet

If the receiver receives a duplicate packet, i.e. if the receiver of the file receives the same packet
twice then the new packet is discarded. This takes care of the case when the ACK sent is lost and the
sender times out, thus causing the retransmission of a packet already received.
