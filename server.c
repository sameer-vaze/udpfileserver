#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#define MAXBUF 100


int main(int argc, char * argv[])
{
	int sock, i, a, rep, x, m, att, file_size, nbytes;						//initialize various variables,structures and pointers used
	struct sockaddr_in sin, remote;
	unsigned int remote_length;
	char *tok;
	char buffer[MAXBUF];
	FILE *fp;
	FILE *fvp;
	FILE *filep;
	char *buffer_last;
	char res[MAXBUF];
	char res_a[4];
	char res_b[10];
	char *ls_value;
	char *temp, *last;
	char filename[] = "copied_";
	struct stat st;
	struct timespec ti;
	ti.tv_sec = 0;
	ti.tv_nsec = 25000000L;
	
	if (argc != 2)															//check if number of arguments passed is sufficient
	{
		printf("Not enough command line arguments");
		exit(1);
	}

	bzero(&sin, sizeof(sin));												//clear structure
	sin.sin_family = AF_INET;												//set family to internet(AF_INET)
	sin.sin_port = htons(atoi(argv[1]));									//set port number
	sin.sin_addr.s_addr = INADDR_ANY;										//set address to be selected by socket API
	
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)							//start socket of internet family with datagram type 
	{
		printf("Error opening socket\n");									//display failure message and exit
		exit(1);
	}

	if((bind(sock, (struct sockaddr *)&sin, sizeof(sin))) < 0)				//bind the socket with address set above
	{
		printf("Unable to bind socket");									//display failure message and exit
		exit(1);
	}

	remote_length = sizeof(remote);											//store length of remote address 

	while(1)
	{
		bzero(buffer, sizeof(buffer));																	//clear buffer
		nbytes = recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length);		//receive command (ls, get(filename), put(filemame), exit)
		printf("Client message is: %s\n", buffer);
		char msg[] = "Received message";
		nbytes = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);			//send acknowledgement "Recieved message"
		
		i = 0;

		tok = strtok(buffer, "()");							//tokenize command string using delimiters "(" and ")" to seperate command(get or put) and filename
		while (tok != NULL)											
		{
			if(i == 0)
			{
				strcpy(res_a, tok);							//store command in character array res_a
			}
			else
			{
				strcpy(res_b, tok);							//store filename in character array res_b
			}
			tok = strtok (NULL, "()");
			i++;
		}
		
		if (strcmp(res_a, "get") == 0)						//if command is "get" this loop is entered
		{
			//printf("This is get\n");
			
			if((fp = fopen(res_b, "rb")) != NULL)			//open file in binary read mode
			{
				printf("File %s does exist in current location\n", res_b);	//notify that file does exist
				stat(res_b, &st);
				file_size = st.st_size;
				printf("file size is %d\n", st.st_size);
			}
			else
			{
				printf("File %s doesn't exist in current location\n", res_b);	//notify that file doesn't exist
				exit(1);									//exit execution
			}
			
			if(file_size > MAXBUF)						//calculate number of loops required to send complete file based on buffer size(100)
			{
				rep = (file_size/MAXBUF) + 1;
			}
			else
			{
				rep = 1;
			}
			
			sprintf(msg, "%d", file_size);				//convert file size from integer to character array
			nbytes = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);			//send file size to client

			int j = 1;
			for(j=1;j<= rep;j++)						//loop rep number of times to send entire file
			{
				int att = 1;
				if (j != rep)							//while the packet to be sent is not the last one
				{
					fread(buffer, MAXBUF, 1, fp);		//read from file a chunk of buffer size MAXBUF(100)
					for (int l = 0; l < MAXBUF;l++)		//store buffer elements in case of a need to send packet again
					{
						res[l] = buffer[l];
					}
					//printf("%s\n", buffer);
					//printf("%d\n", strlen(buffer));
				}
				else
				{
					buffer_last = (char *) malloc(file_size%MAXBUF);	//if packet is the last one assign buffer size as the modulus of file size and buffer size
					fread(buffer, file_size%MAXBUF, 1, fp);				//read last few bytes 
					for (int c = 0; c < file_size%MAXBUF ; c++)
					{
						buffer_last[c] = buffer[c];
					}
					//printf("%d\n", strlen(buffer));
				}
				while(1)								//loop for implementing retransmission in case of lost packets or acknowledgement
				{
					//printf("Attempt: %d\n", att);
					if (j != rep)						 
					{
						sendto(sock, res, MAXBUF, 0, (struct sockaddr *) &remote, remote_length);   //send a packet as big as buffer size
					}
					else
					{
						sendto(sock, buffer_last, MAXBUF, 0, (struct sockaddr *) &remote, remote_length);	//send last packet of remaining size
					}
					if ((recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length)) > 0)	//check if acknowledgement is received
					{
						if (atoi(buffer))		//convert ascii to integer
						{
							break;				//if ACK received leave while(1) loop
						}
						else
						{
							continue;			//if NACK received continue execution of loop
						}
					}
					else 						//if no acknowledgement is recieved
					{
						nanosleep(&ti, NULL);	//wait till time out of 25 ms 
						//att++;
					}
				}
				bzero(buffer,sizeof(buffer));	//clear buffer	
			}
			fclose(fp);							//close file 
			bzero(res_a,sizeof(res_a));			//clear variables res_a and res_b to be reused
			bzero(res_b,sizeof(res_b));
		}
		else if (strcmp(res_a, "put") == 0)		//if command is put enter this loop
		{
			//printf("This is put\n");
			recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length);			//receive file size from client
			file_size = atoi(buffer);						//convert from ascii to integer
			rep = (file_size/MAXBUF) + 1;					//calculate number of packets to be recieved
			int rem = file_size%MAXBUF;						//calculate size of last packet to be received
			strcat(filename, res_b);						//add "copied_" to front of name of file to be received
			//printf("%s\n", filename);
			//printf("%d\n", file_size);
			filep = fopen(filename, "wb");					//open "copied_filename" in binary write mode
			
			for(int j=1;j<=rep;j++) 						//loop for rep number of packets
			{
				while(1)
				{	
					int xa = recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length);	//receive packet and store size of packet 
					//printf("%d\n", xa);
					if (xa > 0)						//if packet size is greater than zero, i.e. a packet is recieved
					{
						sprintf(msg, "%d", "1");	//convert interger "1" to ascii
						sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);		//send ACK in form of integer "1"
						if (j != rep)						//while packet being received isn't last packet
						{
							if(strcmp(buffer, res) == 0)		//if packet received is same as previously received packet, i.e. duplicate
							{
								bzero(buffer, sizeof(buffer));	//clear packet and break from while loop
								break;
							}
							
							for (int l = 0; l < MAXBUF;l++)		//copy contents of buffer to check next iteration for duplicate packets in case of acknowledgement loss
							{
								res[l] = buffer[l];
							}
							//printf("%d\n", sizeof(buffer));
							fwrite(buffer, MAXBUF, 1, filep);	//write buffer to file 
							bzero(buffer, sizeof(buffer));		//clear buffer
						}
						else
						{
							last = (char *) malloc(rem);		//last packet size is allocated dynamically
							if(strcmp(buffer, last) == 0)		//compare for duplicate
							{
								break;
							}
							
							for (int c = 0; c < file_size%MAXBUF ; c++)		//copy contents to check for duplicates 
							{
								last[c] = buffer[c];
							}
							fwrite(buffer, rem, 1, filep);		//write last packet to file
							bzero(buffer, sizeof(buffer));		//clear buffer
							fclose(filep);						//close file
						}
						break;
					}
					else 								//send NACK in form of integer "0"
					{
						sprintf(msg, "%d", "0");
						sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);
					}
				}
			}
			bzero(res_a,sizeof(res_a));					//clear res_a and res_b for reuse
			bzero(res_b,sizeof(res_b));
		}
		else if (strcmp(res_a, "ls") == 0)				//if command is ls enter this loop
		{
			//printf("This is ls\n");
			system("ls -A >ls_values.txt");				//run command line argument that redirects values to a txt file
			stat("ls_values.txt", &st);					//read filesize of txt file
			file_size = st.st_size;
			sprintf(msg, "%d", file_size);				//send file size to client
			nbytes = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);
			if(file_size > MAXBUF)						//calculate number of packets to be sent
			{
				rep = (file_size/MAXBUF) + 1;
			}
			else
			{
				rep = 1;
			}
			fvp = fopen("ls_values.txt","rb");			//open txt file is binary read mode
			temp = (char *) malloc(file_size);			
			fread(temp, file_size, 1, fvp);				//store contents to a temporary character array
			
			int j = 1;
			for(j=1;j<= rep;j++)						//loop to send contents in form of packets
			{
				if (j != rep)
				{
					for (int m = 0; m < MAXBUF; m++)	//create packet of MAXBUF(100) size
					{
						buffer[m] = temp[(MAXBUF*(j-1)) + m];
					}
					printf("%s\n", buffer);
					sendto(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, remote_length); //send packet
				}
				else
				{
					buffer_last = (char *) malloc(file_size%MAXBUF);	//create packet of remainder size for last packet to be sent
					for (int m = 0; m < file_size%MAXBUF; m++)
					{
						buffer_last[m] = temp[(MAXBUF*(j-1)) + m];
					}
					printf("%s\n", buffer_last);
					sendto(sock, buffer_last, file_size%MAXBUF, 0, (struct sockaddr *) &remote, remote_length);		//send last packet
				}
	  			bzero(buffer,sizeof(buffer));				//clear buffer
			}
			fclose(fvp);									//close txt file
			bzero(res_a,sizeof(res_a));						//clear res_a and res_b for reuse
			bzero(res_b,sizeof(res_b));
		}
		else if (strcmp(res_a, "exit") == 0)				//if command is exit enter this loop
		{
			printf("Exiting server side\n");
			close(sock);									//close socket
			exit(1);										//exit gracefully
		}
		else 
		{
			printf("Unknown command: %s\n", buffer);		//default case for unknown command
		}
	}
}