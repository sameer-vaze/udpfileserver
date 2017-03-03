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
	int sock, x;						//initialize variables, pointers and structures
	struct sockaddr_in remote;
	unsigned int remote_length;
	int nbytes;
	int file_size;
	char buffer[MAXBUF];
	char file_buffer[2000];
	char ls_buffer[MAXBUF];
	char *tok;
	char res_a[4], res_b[10], res[MAXBUF];
	char *final_res;
	char *last, *result;
	char *tem, *temp, *buffer_last;
	int i, rep;
	char filename[] = "copied_";
	FILE *filep, *fp;
	struct stat st;
	struct timespec ti;
	ti.tv_sec = 0;
	ti.tv_nsec = 25000000L;
	
	if (argc < 3)					//check if number of command line arguments is sufficient
	{
		printf("Not enough command line arguments");
		exit(1);
	}

	bzero(&remote, sizeof(remote));
	remote.sin_family = AF_INET;				//set family attributes os remote(in this case the server)
	remote.sin_port = htons(atoi(argv[2]));		//set port number
	remote.sin_addr.s_addr = inet_addr(argv[1]);		//set address as the one passed in the command line arguments

	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)		//open socket to transmit in udp i.e. datagram (SOCK_DGRAM)
	{
		printf("Error opening socket\n");				//notify error in opening socket
		exit(1);
	}

	remote_length = sizeof(remote);						//set length of server address

	while(1)
	{
		char msg[] = "";
		printf("Enter message to send:\n\n Options are:\n\t1.ls\n\t2.get(filename)\n\t3.put(filename\n\t4.exit\n\n");
		scanf("%s", msg);								//read command
		tem = (char *) malloc(strlen(msg));
		strcpy(tem, msg);								//copy to a temporary array
		nbytes = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);			//send contents to server
		//printf("These are the contents of msg: %s\n", msg);
		bzero(buffer, sizeof(buffer));					//clear buffer
		
		i = 0;
		tok = strtok(tem, "()");						//tokenize string to extract command (get,put,ls or exit) and filename
		while (tok != NULL)
		{
			if(i == 0)
			{
				strcpy(res_a, tok);
				//printf("i is %d and res_a is %s\n", i, res_a);
			}
			else
			{
				strcpy(res_b, tok);
				//printf("i is %d and res_b is %s\n", i, res_b);
			}
			tok = strtok (NULL, "()");
			i++;
		}

		nbytes = recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length);		//receive server message notifying receipt of command
		printf("Server message is: %s\n", buffer);
		
		if(strcmp(res_a, "get") == 0)						//if command is get enter this loop
		{
			//printf("This is get on client side\n");
			recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length);			//receive filesize from server
			file_size = atoi(buffer);																//convert ascii to integer
			rep = (file_size/MAXBUF) + 1;															//calculate number of packets
			int rem = file_size%MAXBUF;																//calculate size of last packet
			strcat(filename, res_b);
			//printf("%s\n", filename);
			//printf("%d\n", file_size);
			filep = fopen(filename, "wb");							//open file in binary write mode
			
			for(int j=1;j<=rep;j++) 								//loop to receive all packets
			{
				while(1)
				{	
					int xa = recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length);		//recieve packet and store size
					//printf("%d\n", xa);
					if (xa > 0)							//if packet size is greater than zero, i.e. a packet is received
					{
						sprintf(msg, "%d", "1");		//send ACK in form of integer "1"
						sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);
						if (j != rep)
						{
							if(strcmp(buffer, res) == 0)		//check duplicate packet as res contains previously recieved packet
							{
								bzero(buffer, sizeof(buffer));	//clear buffer
								break;							//break from while(1) to recieve next packet
							}
							
							for (int l = 0; l < MAXBUF;l++)		//store packet to check for duplicates in next iteration
							{
								res[l] = buffer[l];
							}
							//printf("%d\n", sizeof(buffer));
							fwrite(buffer, MAXBUF, 1, filep);	//write packet to file
							bzero(buffer, sizeof(buffer));		//clear buffer
						}
						else
						{
							last = (char *) malloc(rem);
							if(strcmp(buffer, last) == 0)		//compare last packet for duplicate
							{
								break;
							}
							
							for (int c = 0; c < file_size%MAXBUF ; c++)
							{
								last[c] = buffer[c];			//store packet 
							}
							fwrite(buffer, rem, 1, filep);		//write last packet to file
							bzero(buffer, sizeof(buffer));		//clear buffer
							fclose(filep);						//close file
						}
						break;									//leave while loop
					}
					else
					{
						sprintf(msg, "%d", "0");				//send NACK in form of integer "0"
						sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);
					}
				}
			}
			bzero(res_a,sizeof(res_a));							//clear res_a and res_b for reuse
			bzero(res_b,sizeof(res_b));
		}
		else if (strcmp(res_a, "put") == 0)						//if command is put enter this loop
		{
			//printf("This is put\n");
			
			if((fp = fopen(res_b, "rb")) != NULL)				//open file in binary read mode
			{
				printf("File %s does exist in current location\n", res_b);		//notify that file exists
				stat(res_b, &st);												//read file size
				file_size = st.st_size;
				//printf("file size is %d\n", st.st_size);
			}
			else
			{
				printf("File %s doesn't exist in current location\n", res_b);		//notify that file doesn't exist in current location
				exit(1);															//exit execution
			}
			
			if(file_size > MAXBUF)													//calculate number of packets to be sent
			{
				rep = (file_size/MAXBUF) + 1;
			}
			else
			{
				rep = 1;
			}
			
			sprintf(msg, "%d", file_size);
			nbytes = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, remote_length);			//send filesize to server

			int j = 1;
			for(j=1;j<= rep;j++)											//loop to send packets
			{
				int att = 1;
				if (j != rep)
				{
					fread(buffer, MAXBUF, 1, fp);							//read file for a packet size of MAXBUF(100)
					for (int l = 0; l < MAXBUF;l++)
					{
						res[l] = buffer[l];									//store for retransmission in case of packet loss
					}
					//printf("%s\n", buffer);
					//printf("%d\n", strlen(buffer));
				}
				else
				{
					buffer_last = (char *) malloc(file_size%MAXBUF);
					fread(buffer, file_size%MAXBUF, 1, fp);					//read last packet
					for (int c = 0; c < file_size%MAXBUF ; c++)
					{
						buffer_last[c] = buffer[c];							//store for retransmission
					}
					//printf("%d\n", strlen(buffer));
				}
				while(1)													//while loop that sends until ACK is not recieved
				{
					//printf("Attempt: %d\n", att);
					if (j != rep)
					{
						sendto(sock, res, MAXBUF, 0, (struct sockaddr *) &remote, remote_length);			//send packets that are of size of buffer
					}
					else
					{
						sendto(sock, buffer_last, MAXBUF, 0, (struct sockaddr *) &remote, remote_length);	//send last packet that is of remainder length
					}
					if ((recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length)) > 0)	//check if ACK or NACK is recived
					{
						if (atoi(buffer))							//if ACK i.e. integer "1" break from while
						{
							break;
						}
						else										//else continue execution
						{
							continue;
						}
					}
					else
					{
						nanosleep(&ti, NULL);						//wait till time out of 25 ms 
						att++;
					}
				}
				bzero(buffer,sizeof(buffer));	
			}
			fclose(fp);												//close file
			printf("File %s successfuly stored in server\n", res_b);	//notify successful put operation
			bzero(res_a,sizeof(res_a));								//clear res_a and res_b for reuse
			bzero(res_b,sizeof(res_b));
		}
		else if (strcmp(res_a, "ls") == 0)							//if command is ls enter this loop
		{
			// printf("This is ls\n");
			recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length);			//receive filesize of txt containing ls results
			file_size = atoi(buffer);								//convert from ascii to integer
			rep = (file_size/MAXBUF) + 1;							//calculate number of packets to be recived
			int rem = file_size%MAXBUF;								//calculte last packet size
			int j = 1;
			for(int j=1;j<=rep;j++) 								//loop to recive all packets
			{
				int xa = recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *) &remote, &remote_length);		//receive packet
				//printf("%d\n", xa);
				if (j != rep)
				{
					strcpy(res, buffer);
					printf("%s\n", res);							//display packet received
					bzero(buffer, sizeof(buffer));					//clear buffer
				}
				else
				{
					last = (char *) malloc(rem);					//last packet 
					strcpy(last, buffer);
					bzero(buffer, sizeof(buffer));
					printf("%s\n", last);							//display last packet
				}
			}
			bzero(res_a,sizeof(res_a));								//clear res_a and res_b for reuse
			bzero(res_b,sizeof(res_b));
		}
		else if(strcmp(res_a, "exit") == 0)							//if command is exit enter this loop
		{
			printf("Exiting client side\n");
			close(sock);											//close socket
			exit(1);												//exit gracefully
		}
	}
}