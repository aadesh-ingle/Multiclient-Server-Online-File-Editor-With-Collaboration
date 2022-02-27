#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#define SIZE 1024
#define PORT 8888

// Recieve and print file sent by the Server.
// Used by the similarity function.
void recieve_and_print(int sockfd)
{
	int n;
	char buffer[1024];
	read(sockfd, buffer, sizeof(buffer));
	n = atoi(buffer);
	bzero(buffer, 1024);
	snprintf(buffer, sizeof(buffer), "ok");
	write(sockfd, buffer, sizeof(buffer));
	bzero(buffer, 1024);
	while (1)
	{
		if (n <= 0)
		{
			break;
			return;
		}
		read(sockfd, buffer, sizeof(buffer));
		// fputs(buffer, fp);
		printf("%s", buffer);
		n--;
		bzero(buffer, 1024);
		snprintf(buffer, sizeof(buffer), "ok");
		write(sockfd, buffer, sizeof(buffer));
		bzero(buffer, 1024);
	}
	return;
}
// Recieve and Copy file sent by the Server.
// first we get line number for the while loop ending.
void write_file(FILE *fp, int sockfd)
{
	int n;
	char buffer[1024];
	read(sockfd, buffer, sizeof(buffer));
	n = atoi(buffer);
	bzero(buffer, 1024);
	snprintf(buffer, sizeof(buffer), "ok");
	write(sockfd, buffer, sizeof(buffer));
	bzero(buffer, 1024);
	while (1)
	{
		if (n <= 0)
		{
			break;
			return;
		}
		read(sockfd, buffer, sizeof(buffer));
		fputs(buffer, fp);
		// printf("%s\n",buffer);
		n--;
		bzero(buffer, 1024);
		snprintf(buffer, sizeof(buffer), "ok");
		write(sockfd, buffer, sizeof(buffer));
		bzero(buffer, 1024);
	}
	fseek(fp, 0, SEEK_SET);
	return;
}
// Count number of lines in file
int cline(FILE *fp)
{
	int count = 0;
	char c;
	// count newline characters so we knwo number of lines
	// return count of number of lines
	if (fp == NULL)
	{
		printf("Could not open file\n");
		return 0;
	}
	for (c = getc(fp); c != EOF; c = getc(fp))
		if (c == '\n')
			count = count + 1;
	fseek(fp, 0, SEEK_SET);
	return count;
}
// Send file line by line to the Server
void send_file(FILE *fp, int sockfd)
{
	int n;
	char data[SIZE] = {0};
	while (fgets(data, SIZE, fp) != NULL)
	{
		if (write(sockfd, data, sizeof(data)) == -1)
		{
			perror("Error in sending file.");
			exit(1);
		}
		bzero(data, SIZE);
		read(sockfd, data, sizeof(data));
		bzero(data, SIZE);
	}
	fseek(fp, 0, SEEK_SET);
}
int main()
{

	int clientSocket, ret, clientSocket1;
	struct sockaddr_in serverAddr;
	char buffer[1024];
	int fd[2];
	if (pipe(fd) == -1)
	{
		return 1;
	}
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	clientSocket1 = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0)
	{
		printf("Error in connection.\n");
		exit(1);
	}
	if (clientSocket1 < 0)
	{
		printf("Error in connection.\n");
		exit(1);
	}
	printf("Client Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	pid_t childpid;
	int id1 = shmget(IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
	int *count = shmat(id1, 0, 0);
	*count = 0;
	if ((childpid = fork()) != 0)
	{
		ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
		if (ret < 0)
		{
			printf("Error in connection.\n");
			exit(1);
		}
		else
		{
			read(clientSocket, buffer, sizeof(buffer));
			// printf("%s\n buffer printed", buffer);
			if (strncmp("FAIL", buffer, 4) == 0)
			{
				printf("Failure to connect\n");
				bzero(buffer, sizeof(buffer));
				return 0;
			}
			else if (strncmp("ECHO", buffer, 4) == 0)
			{
				printf("Connected to Server.\n");
				bzero(buffer, sizeof(buffer));
			}
		}
		while (1)
		{
			int connfd = clientSocket;
			int n, mif, dqc, ilc;
			char buffer[1024], c, d, msg[1024];
			bzero(buffer, SIZE);
			bzero(msg, SIZE);
			printf("Client: \t");
			n = 0;
			mif = 0;
			dqc = 0;
			ilc = 0;
			bzero(buffer, SIZE);
			// if(read(fd[0], buffer, 1024) != 0)
			// {
			// 	bzero(buffer, SIZE);
			// 	while ((buffer[n++] = getchar()) != '\n')
			// 		;
			// 	write(fd[1], buffer, 1024);
			// 	bzero(buffer, SIZE);
			// 	continue;
			// }
			// bzero(buffer, SIZE);
			while ((buffer[n++] = getchar()) != '\n')
			{
				if (n == 7)
				{
					if (strncmp("/insert", buffer, 7) == 0)
					{

						do
						{
							ilc = 0;
							d = getchar();
							if (d == '"')
							{
								dqc++;
								continue;
							}
							if (dqc == 1)
								msg[mif++] = d;
							else
								buffer[n++] = d;
							if (dqc == 0 && d == '\n')
								break;
						} while (dqc != 2);
						if (dqc == 0)
							break;
					}
				}
				ilc++;
			}
			// printf("%d",ilc);
			if (*count == 1)
			{
				if (strncmp("yes", buffer, 3) == 0 || strncmp("Yes", buffer, 3) == 0 )
				{
					*count = 0;
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "yes");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
				}
				else if (strncmp("no", buffer, 2) == 0 || strncmp("No", buffer, 2) == 0 )
				{
					*count = 0;
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "no");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
				}
				else
				{
					printf("Type Yes or No only.\n");
				}
			}
			else
			{
				if (strncmp("/invite", buffer, 7) == 0 && buffer[7] == ' ')
				{
					char fn1[SIZE], com[100], cln1[SIZE], permi[SIZE];
					int f2 = 0, n = 0, invf = 0;
					sscanf(buffer, "%s %s %s %s", com, fn1, cln1, permi);
					for (int i = 0; i < strlen(buffer); i++)
					{
						c = buffer[i];
						if (c == ' ')
						{
							f2++;
							if (buffer[i + 1] == ' ' || buffer[i + 1] == '\t')
							{
								invf++;
								continue;
							}
						}
					}
					if (f2 != 3)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					for (int i = 0; i < strlen(cln1); i++)
					{
						c = cln1[i];
						if (c >= '0' && c <= '9')
							;
						else
						{
							printf("Invalid Command.\n");
							continue;
						}
					}
					if (strcmp(permi, "V") != 0 && strcmp(permi, "E") != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "invite");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s", cln1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s", fn1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s", permi);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					printf("%s", buffer);
					bzero(buffer, SIZE);
				}
				else if (strncmp("/insert", buffer, 7) == 0 && buffer[7] == ' ')
				{
					if (ilc != 1)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (dqc != 2)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (mif == 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					char fn1[SIZE], com[100], ln1[SIZE];
					bzero(com, 100);
					bzero(ln1, SIZE);
					bzero(fn1, SIZE);
					int f2 = 0, n = 0, invf = 0, number1 = 0, f3 = 0, flag = 0, digit;
					sscanf(buffer, "%s %s %s", com, fn1, ln1);
					for (int i = 0; i < strlen(buffer); i++)
					{
						c = buffer[i];
						if (c == ' ')
						{
							f2++;
							if (buffer[i + 1] == ' ' || buffer[i + 1] == '\t')
							{
								invf++;
								continue;
							}
						}
					}
					if (f2 > 3)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					invf = 0;
					printf("%s\n", ln1);
					for (int i = 0; i < strlen(ln1); i++)
					{
						c = ln1[i];
						if (c >= '0' && c <= '9')
						{
							if (ln1[i - 1] == '-')
								flag = 1;
							f3++;
							digit = c - '0';
							number1 = number1 * 10 + digit;
						}
						else if (i > 0 || c != '-')
						{
							invf++;
						}
						if (c == '-' && !(ln1[i + 1] >= '0' && ln1[i + 1] <= '9'))
							invf++;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (f3 == 0)
					{
						number1 = 999999999;
					}
					else if (flag == 1)
					{
						number1 = number1 * -1;
						// printf("\n%d\n",number1);
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "insert");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s", fn1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%d", number1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s\n", msg);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					recieve_and_print(connfd);
				}
				else if (strncmp("/upload", buffer, 7) == 0 && buffer[7] == ' ')
				{
					char fn1[SIZE], com[100];
					int f2 = 0, n = 0, invf = 0;
					sscanf(buffer, "%s %s", com, fn1);
					for (int i = 0; i < strlen(buffer); i++)
					{
						c = buffer[i];
						if (c == ' ')
						{
							f2++;
							if (buffer[i + 1] == ' ' || buffer[i + 1] == '\t')
							{
								invf++;
								continue;
							}
						}
					}
					if (f2 != 1)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					FILE *fp = fopen(fn1, "r");
					if (fp == NULL)
					{
						printf("Could not open file %s\n", fn1);
						continue;
					}
					// printf("%s",fn1);
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "upload");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s", fn1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					printf("Upload Succesfull\n");
					int z = cline(fp);
					bzero(buffer, 1024);
					snprintf(buffer, sizeof(buffer), "%d", z);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					send_file(fp, connfd);
					fclose(fp);
				}
				else if (strncmp("/download", buffer, 9) == 0 && buffer[9] == ' ')
				{
					char fn1[SIZE], com[100];
					int f2 = 0, n = 0, invf = 0;
					sscanf(buffer, "%s %s", com, fn1);
					for (int i = 0; i < strlen(buffer); i++)
					{
						c = buffer[i];
						if (c == ' ')
						{
							f2++;
							if (buffer[i + 1] == ' ' || buffer[i + 1] == '\t')
							{
								invf++;
								continue;
							}
						}
					}
					if (f2 != 1)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "download");
					write(connfd, buffer, sizeof(buffer));
					// printf("%s\n",buffer);
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s", fn1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					char fname1[SIZE];
					bzero(fname1, SIZE);
					int mpid = getpid();
					snprintf(fname1, sizeof(fname1), "%d_%s", mpid, fn1);
					printf("Downloaded Successfully.And stored as : \"%s\".\n", fname1);
					FILE *fp = fopen(fname1, "w");
					if (fp == NULL)
					{
						printf("Could not open file %s\n", fn1);
						continue;
					}
					write_file(fp, connfd);
					fclose(fp);
					bzero(buffer, SIZE);
				}
				else if (strncmp("/exit", buffer, 5) == 0 && buffer[5] == '\n')
				{
					printf("Disconnected from server.\n");
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "exit");
					write(connfd, buffer, sizeof(buffer));
					close(connfd);
					exit(1);
				}
				else if (strncmp("/users", buffer, 6) == 0 && buffer[6] == '\n')
				{
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "users");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					int val = atoi(buffer);
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "ok");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					int i;
					for (i = 0; i < val; i++)
					{
						bzero(buffer, SIZE);
						read(connfd, buffer, sizeof(buffer));
						printf("%s", buffer);
						bzero(buffer, SIZE);
						snprintf(buffer, sizeof(buffer), "ok");
						write(connfd, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
					}
					bzero(buffer, SIZE);
				}
				else if (strncmp("/files", buffer, 6) == 0 && buffer[6] == '\n')
				{
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "files");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					int val = atoi(buffer);
					if (val == 0)
						printf("No files present\n");
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "ok");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					int i, j;
					for (i = 0; i < val; i++)
					{
						bzero(buffer, SIZE);
						read(connfd, buffer, sizeof(buffer));
						printf("%s", buffer);
						bzero(buffer, SIZE);
						snprintf(buffer, sizeof(buffer), "ok");
						write(connfd, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						read(connfd, buffer, sizeof(buffer));
						printf("%s", buffer);
						bzero(buffer, SIZE);
						snprintf(buffer, sizeof(buffer), "ok");
						write(connfd, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						read(connfd, buffer, sizeof(buffer));
						int val2 = atoi(buffer);
						bzero(buffer, SIZE);
						if (val2 == 0)
							printf("Collaborators : None");
						else
							printf("Collaborators : \t");
						snprintf(buffer, sizeof(buffer), "ok");
						write(connfd, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						// printf("%d  %d\n",val,val2);
						for (j = 0; j < val2; j++)
						{
							bzero(buffer, SIZE);
							read(connfd, buffer, sizeof(buffer));
							printf("%s", buffer);
							bzero(buffer, SIZE);
							snprintf(buffer, sizeof(buffer), "ok");
							write(connfd, buffer, sizeof(buffer));
							bzero(buffer, SIZE);
						}
					}
					printf("\n");
					bzero(buffer, SIZE);
				}
				else if (strncmp("/read", buffer, 5) == 0 && buffer[5] == ' ')
				{
					char fn1[SIZE], com[100], ln1[SIZE], ln2[SIZE];
					bzero(ln2, SIZE);
					bzero(ln1, SIZE);
					bzero(fn1, SIZE);
					int f2 = 0, invf = 0, digit, number1 = 0, number2 = 0, flag = 0, f3 = 0, f4 = 0;
					sscanf(buffer, "%s %s %s %s", com, fn1, ln1, ln2);
					for (int i = 0; i < strlen(buffer); i++)
					{
						c = buffer[i];
						if (c == ' ')
						{
							f2++;
							if (buffer[i + 1] == ' ' || buffer[i + 1] == '\t')
							{
								invf++;
								continue;
							}
						}
					}
					if (f2 > 3)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					invf = 0;
					// printf("len1 : %s\n",ln1);
					// printf("len2 : %s\n",ln2);
					for (int i = 0; i < strlen(ln1); i++)
					{
						c = ln1[i];
						if (c >= '0' && c <= '9')
						{
							if (ln1[i - 1] == '-')
								flag = 1;
							f3++;
							digit = c - '0';
							number1 = number1 * 10 + digit;
						}
						else if (i > 0 || c != '-')
						{
							invf++;
						}
						if (c == '-' && !(ln1[i + 1] >= '0' && ln1[i + 1] <= '9'))
							invf++;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (flag == 1)
					{
						number1 = number1 * -1;
					}
					invf = 0;
					flag = 0;
					for (int i = 0; i < strlen(ln2); i++)
					{
						c = ln2[i];
						if (c >= '0' && c <= '9')
						{
							if (ln2[i - 1] == '-')
								flag = 1;
							f4++;
							digit = c - '0';
							number2 = number2 * 10 + digit;
						}
						else if (i > 0 || c != '-')
						{
							invf++;
						}
						if (c == '-' && !(ln2[i + 1] >= '0' && ln2[i + 1] <= '9'))
							invf++;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (flag == 1)
					{
						number2 = number2 * -1;
						// printf("%d %d\n",number1,number2);
					}
					if (f4 == 0 && f3 != 0)
					{
						number2 = number1;
					}
					if (f4 == 0 && f3 == 0)
					{
						number1 = 0;
						number2 = 999999999;
					}
					// printf("%d %d\n",number1,number2);
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "read");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s", fn1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%d", number1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%d", number2);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					recieve_and_print(connfd);
				}
				else if (strncmp("/delete", buffer, 7) == 0 && buffer[7] == ' ')
				{
					char fn1[SIZE], com[100], ln1[SIZE], ln2[SIZE];
					bzero(ln2, SIZE);
					bzero(ln1, SIZE);
					bzero(fn1, SIZE);
					int f2 = 0, invf = 0, digit, number1 = 0, number2 = 0, flag = 0, f3 = 0, f4 = 0;
					sscanf(buffer, "%s %s %s %s", com, fn1, ln1, ln2);
					for (int i = 0; i < strlen(buffer); i++)
					{
						c = buffer[i];
						if (c == ' ')
						{
							f2++;
							if (buffer[i + 1] == ' ' || buffer[i + 1] == '\t')
							{
								invf++;
								continue;
							}
						}
					}
					if (f2 > 3)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					invf = 0;
					// printf("len1 : %s\n",ln1);
					// printf("len2 : %s\n",ln2);
					for (int i = 0; i < strlen(ln1); i++)
					{
						c = ln1[i];
						if (c >= '0' && c <= '9')
						{
							if (ln1[i - 1] == '-')
								flag = 1;
							f3++;
							digit = c - '0';
							number1 = number1 * 10 + digit;
						}
						else if (i > 0 || c != '-')
						{
							invf++;
						}
						if (c == '-' && !(ln1[i + 1] >= '0' && ln1[i + 1] <= '9'))
							invf++;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (flag == 1)
					{
						number1 = number1 * -1;
					}
					invf = 0;
					flag = 0;
					for (int i = 0; i < strlen(ln2); i++)
					{
						c = ln2[i];
						if (c >= '0' && c <= '9')
						{
							if (ln2[i - 1] == '-')
								flag = 1;
							f4++;
							digit = c - '0';
							number2 = number2 * 10 + digit;
						}
						else if (i > 0 || c != '-')
						{
							invf++;
						}
						if (c == '-' && !(ln2[i + 1] >= '0' && ln2[i + 1] <= '9'))
							invf++;
					}
					if (invf != 0)
					{
						printf("Invalid Command.\n");
						continue;
					}
					if (flag == 1)
					{
						number2 = number2 * -1;
						// printf("%d %d\n",number1,number2);
					}
					if (f4 == 0 && f3 != 0)
					{
						number2 = number1;
					}
					if (f4 == 0 && f3 == 0)
					{
						number1 = 0;
						number2 = 999999999;
					}
					// printf("%d %d\n",number1,number2);
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "delete");
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%s", fn1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%d", number1);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					bzero(buffer, SIZE);
					snprintf(buffer, sizeof(buffer), "%d", number2);
					write(connfd, buffer, sizeof(buffer));
					bzero(buffer, SIZE);
					read(connfd, buffer, sizeof(buffer));
					if (strncmp("ok", buffer, 2) != 0)
					{
						printf("%s", buffer);
						bzero(buffer, sizeof(buffer));
						continue;
					}
					recieve_and_print(connfd);
				}
				else
				{
					printf("Invalid Command.\n");
				}
			}
		}
		// client(clientSocket);
	}
	else
	{
		sleep(0.25);
		ret = connect(clientSocket1, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
		if (ret < 0)
		{
			printf("Error in connection.\n");
			exit(1);
		}
		else
		{
			read(clientSocket1, buffer, sizeof(buffer));
			// printf("%s\n buffer printed", buffer);
			if (strncmp("FAIL", buffer, 4) == 0)
			{
				bzero(buffer, sizeof(buffer));
				exit(0);
			}
			else if (strncmp("ECHO", buffer, 4) == 0)
			{
				// printf("Connected to Server.\n");
				bzero(buffer, sizeof(buffer));
			}
		}
		int *count = shmat(id1, 0, 0);
		while (1)
		{
			char data[SIZE];
			bzero(data, SIZE);
			int mval = read(clientSocket1, data, sizeof(data));
			if (strncmp("exit", data, 4) == 0 || mval == 0)
			{
				exit(0);
			}
			printf("\n%s\n", data);
			*count = 1;
			printf("Only enter \"yes\" or \"no\".\nAnything else will be treated as invalid command and will not be sent to server.\n");
			bzero(data, SIZE);
		}
	}
	return 0;
}
