#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>

#define TRUE 1
#define FALSE 0
#define PORT 8888
#define SIZE 1024
int myclientid = 12345;
struct filestore
{
	char fname[100];
	int valid;
	int owner;
	int collab[5][2];
};
struct filestore myfiles[100];
void write_perm()
{
	FILE *fp;
	fp = fopen("permfile.txt", "w");
	int x, z;
	for (x = 0; x < 100; x++)
	{
		if (myfiles[x].valid != -1)
		{
			fprintf(fp, "%s\t", myfiles[x].fname);
			fprintf(fp, "%d\t", myfiles[x].owner);
			for (z = 0; z < 5; z++)
			{
				if (myfiles[x].collab[z][0] != -1)
				{
					fprintf(fp, "%d ", myfiles[x].collab[z][0]);
					if (myfiles[x].collab[z][1] == 1)
						fprintf(fp, "V\t");
					else
						fprintf(fp, "E\t");
				}
			}
			fprintf(fp, "\n");
		}
	}
	fclose(fp);
}
int iviewer(int x, int id)
{
	if (myfiles[x].owner == id)
		return 1;
	for (int i = 0; i < 5; i++)
	{
		if (myfiles[x].collab[i][0] == id)
		{
			return 1;
		}
	}
	return 0;
}
int ieditor(int x, int id)
{
	if (myfiles[x].owner == id)
		return 1;
	for (int i = 0; i < 5; i++)
	{
		if (myfiles[x].collab[i][0] == id && myfiles[x].collab[i][1] == 2)
		{
			return 1;
		}
	}
	return 0;
}
void delclient(int id)
{
	int x, z;
	for (x = 0; x < 100; x++)
	{
		if (myfiles[x].valid != -1)
		{
			if (myfiles[x].owner == id)
			{
				myfiles[x].valid = -1;
				char fname1[SIZE];
				bzero(fname1, SIZE);
				snprintf(fname1, sizeof(fname1), "%d_%s", myfiles[x].owner, myfiles[x].fname);
				remove(fname1);
				for (z = 0; z < 5; z++)
				{
					myfiles[x].collab[z][0] = -1;
				}
				continue;
			}
			for (z = 0; z < 5; z++)
			{
				if (myfiles[x].collab[z][0] == id)
				{
					myfiles[x].collab[z][0] = -1;
				}
			}
		}
	}
	write_perm();
}
int cline(FILE *fp)
{
	int count = 0;
	char c;
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
void send_file(FILE *fp, int sockfd)
{
	int n;
	char data[1024] = {0};
	while (fgets(data, 1024, fp) != NULL)
	{
		if (write(sockfd, data, sizeof(data)) == -1)
		{
			perror("Error in sending file.");
			exit(1);
		}
		bzero(data, 1024);
		read(sockfd, data, sizeof(data));
		bzero(data, 1024);
	}
	fseek(fp, 0, SEEK_SET);
}
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
		// printf("%s\n", buffer);
		n--;
		bzero(buffer, 1024);
		snprintf(buffer, sizeof(buffer), "ok");
		write(sockfd, buffer, sizeof(buffer));
		bzero(buffer, 1024);
	}
	fseek(fp, 0, SEEK_SET);
	return;
}
void rline(FILE *fp, int sockfd, int ln1, int ln2)
{
	int count = 0;
	char data[1024] = {0};
	while (fgets(data, 1024, fp) != NULL)
	{
		if (count >= ln1 && count <= ln2)
		{
			if (write(sockfd, data, sizeof(data)) == -1)
			{
				perror("Error in sending file.");
				exit(1);
			}
			bzero(data, 1024);
			read(sockfd, data, sizeof(data));
			bzero(data, 1024);
		}
		count++;
		bzero(data, 1024);
	}
	fseek(fp, 0, SEEK_SET);
}
void dline(FILE *fp, int ln1, int ln2)
{
	int count = 0;
	char data[1024] = {0};
	FILE *fp1 = fopen("server_file1.txt", "w");
	while (fgets(data, 1024, fp) != NULL)
	{
		if (count >= ln1 && count <= ln2)
		{
			bzero(data, 1024);
			count++;
			continue;
		}
		fputs(data, fp1);
		count++;
		bzero(data, 1024);
	}
	fclose(fp1);
}
void iline(FILE *fp, int ln1, char *buffer)
{
	int count = 0, flag = 0;
	;
	char data[1024] = {0};
	FILE *fp1 = fopen("server_file1.txt", "w");
	while (fgets(data, 1024, fp) != NULL)
	{
		if (count == ln1)
		{
			fputs(buffer, fp1);
			flag++;
			count++;
		}
		fputs(data, fp1);
		count++;
		bzero(data, 1024);
	}
	if (flag == 0)
		fputs(buffer, fp1);
	fclose(fp1);
}
int main(int argc, char *argv[])
{
	int opt = TRUE;
	int m_sckt, addrlen, new_socket, client_socket[10],
		max_clients = 5, activity, i, valread, newSocket;
	int max_sd;
	int x;
	struct sockaddr_in address;
	char buffer[1024];
	int activeclients[5];
	for (x = 0; x < 5; x++)
	{
		activeclients[x] = -1;
	}
	for (x = 0; x < 100; x++)
	{
		myfiles[x].valid = -1;
	}
	fd_set readfds;

	char *message = "ECHO";

	for (i = 0; i < 10; i++)
	{
		client_socket[i] = 0;
	}
	if ((m_sckt = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(m_sckt, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
				   sizeof(opt)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(m_sckt, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d \n", PORT);

	if (listen(m_sckt, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addrlen = sizeof(address);
	puts("Waiting for connections ...");
	int try_temp = -1;
	while (TRUE)
	{

		FD_ZERO(&readfds);
		FD_SET(m_sckt, &readfds);
		max_sd = m_sckt;

		for (i = 0; i < max_clients; i++)
		{

			newSocket = client_socket[i];

			if (newSocket > 0)
				FD_SET(newSocket, &readfds);

			if (newSocket > max_sd)
				max_sd = newSocket;
		}

		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}

		if (FD_ISSET(m_sckt, &readfds))
		{
			if ((new_socket = accept(m_sckt,
									 (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			int flag = 0;
			if (try_temp != -1)
			{
				client_socket[try_temp] = new_socket;
				try_temp = -1;
				send(new_socket, message, strlen(message), 0);
				flag = 1;
			}
			else
				for (i = 0; i < max_clients; i++)
				{
					if (client_socket[i] == 0)
					{
						myclientid++;
						client_socket[i] = new_socket;
						activeclients[i] = myclientid;
						try_temp = i + 5;
						printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
						send(new_socket, message, strlen(message), 0);
						flag = 1;
						break;
					}
				}
			if (flag == 0)
			{
				message = "FAIL";
				send(new_socket, message, strlen(message), 0);
				flag = 0;
				message = "ECHO";
			}
		}
		for (i = 0; i < max_clients; i++)
		{
			newSocket = client_socket[i];

			if (FD_ISSET(newSocket, &readfds))
			{

				if ((valread = read(newSocket, buffer, 1024)) == 0)
				{
					delclient(activeclients[i]);
					printf("Disconnected from client.\n");
					close(newSocket);
					bzero(buffer, 1024);
					snprintf(buffer, sizeof(buffer), "exit");
					write(client_socket[i + 5], buffer, sizeof(buffer));
					bzero(buffer, 1024);
					close(client_socket[i + 5]);
					client_socket[i] = 0;
					client_socket[i + 5] = 0;
					activeclients[i] = -1;
				}

				else
				{
					printf("%s\n", buffer);
					if (strncmp("users", buffer, 5) == 0)
					{
						bzero(buffer, 1024);
						int counter = 0;
						for (x = 0; x < 5; x++)
						{
							if (activeclients[x] != -1)
							{
								counter++;
							}
						}
						snprintf(buffer, sizeof(buffer), "%d", counter);
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						for (x = 0; x < 5; x++)
						{
							if (activeclients[x] != -1)
							{
								if (x == i)
								{
									snprintf(buffer, sizeof(buffer), "Your Client id: %d\n", activeclients[x]);
									write(newSocket, buffer, sizeof(buffer));
									bzero(buffer, 1024);
									read(newSocket, buffer, sizeof(buffer));
									bzero(buffer, 1024);
									continue;
								}
							}
						}
						for (x = 0; x < 5; x++)
						{
							if (activeclients[x] != -1)
							{
								if (x == i)
								{
									continue;
								}
								snprintf(buffer, sizeof(buffer), "Active client id %d: %d\n", x + 1, activeclients[x]);
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								read(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
							}
						}
					}
					else if (strncmp("invite", buffer, 6) == 0)
					{
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						int maj = atoi(buffer);
						bzero(buffer, 1024);
						int majcl = -1;
						for (x = 0; x < 5; x++)
						{
							if (maj == activeclients[x])
								majcl = x;
						}
						if (majcl == i)
						{
							snprintf(buffer, sizeof(buffer), "Invalid Id.Enter other client not own\n");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
							continue;
						}
						if (majcl == -1)
						{
							snprintf(buffer, sizeof(buffer), "Invalid Client.Client not present.\n");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
							continue;
						}
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						char finame[SIZE], permi[SIZE];
						int stare, n = 0;
						bzero(finame, 1024);
						for (x = 0; x < 100; x++)
						{
							if (myfiles[x].valid == 1 && strcmp(myfiles[x].fname, buffer) == 0)
							{
								if (activeclients[i] != myfiles[x].owner)
								{
									bzero(buffer, 1024);
									snprintf(buffer, sizeof(buffer), "Invalid Action.You are not owner of this file.\n");
									write(newSocket, buffer, sizeof(buffer));
									bzero(buffer, 1024);
									n = 2;
									break;
								}
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "ok");
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								strcpy(finame, myfiles[x].fname);
								n = 1;
								stare = x;
								continue;
							}
						}
						if (n == 2)
							continue;
						if (n != 1)
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "Not valid file name.File does not exist.\n");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
							continue;
						}
						read(newSocket, buffer, sizeof(buffer));
						strcpy(permi, buffer);
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "%d invites you to have permission %s of file %s.", activeclients[i], permi, finame);
						write(client_socket[majcl + 5], buffer, sizeof(buffer));
						bzero(buffer, 1024);
						read(client_socket[majcl], buffer, sizeof(buffer));
						int t_flag = 0;
						if (strncmp("yes", buffer, 3) == 0)
						{
							for (x = 0; x < 5; x++)
							{
								if (myfiles[stare].collab[x][0] == activeclients[majcl])
								{
									if (strcmp("V", permi) == 0)
										myfiles[stare].collab[x][1] = 1;
									if (strcmp("E", permi) == 0)
										myfiles[stare].collab[x][1] = 2;
									write_perm();
									t_flag = 1;
								}
							}
							if (t_flag == 0)
								for (x = 0; x < 5; x++)
								{
									if (myfiles[stare].collab[x][0] == -1)
									{
										myfiles[stare].collab[x][0] = activeclients[majcl];
										if (strcmp("V", permi) == 0)
											myfiles[stare].collab[x][1] = 1;
										if (strcmp("E", permi) == 0)
											myfiles[stare].collab[x][1] = 2;
										write_perm();
										break;
									}
								}
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "ok");
							write(client_socket[majcl], buffer, sizeof(buffer));
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "Client accepted and added to collaborators.\n");
							write(newSocket, buffer, sizeof(buffer));
						}
						else
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "ok");
							write(client_socket[majcl], buffer, sizeof(buffer));
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "Client rejected and not added to collaborators.\n");
							write(newSocket, buffer, sizeof(buffer));
						}
					}
					else if (strncmp("yes", buffer, 3) == 0)
					{
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "Invalid Command\n");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
					}
					else if (strncmp("no", buffer, 2) == 0)
					{
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "Invalid Command\n");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
					}
					else if (strncmp("files", buffer, 5) == 0)
					{
						bzero(buffer, 1024);
						int counter = 0;
						int z = 0;
						for (x = 0; x < 100; x++)
						{
							if (myfiles[x].valid != -1)
							{
								counter++;
							}
						}
						snprintf(buffer, sizeof(buffer), "%d", counter);
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						for (x = 0; x < 100; x++)
						{
							counter = 0;
							if (myfiles[x].valid != -1)
							{
								char fname1[1024];
								bzero(fname1, 1024);
								snprintf(fname1, sizeof(fname1), "%d_%s", myfiles[x].owner, myfiles[x].fname);
								FILE *fp = fopen(fname1, "r");
								int z = cline(fp);
								fclose(fp);
								snprintf(buffer, sizeof(buffer), "\n\nFile Name : %s\tNo. Lines : %d\t", myfiles[x].fname, z);
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								read(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "Owner : %d\n", myfiles[x].owner);
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								read(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								for (z = 0; z < 5; z++)
								{
									if (myfiles[x].collab[z][0] != -1)
									{
										counter++;
									}
								}
								snprintf(buffer, sizeof(buffer), "%d", counter);
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								read(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								for (z = 0; z < 5; z++)
								{
									if (myfiles[x].collab[z][0] != -1 && myfiles[x].collab[z][1] == 1)
									{
										snprintf(buffer, sizeof(buffer), "Viewer : %d\t", myfiles[x].collab[z][0]);
										write(newSocket, buffer, sizeof(buffer));
										bzero(buffer, 1024);
										read(newSocket, buffer, sizeof(buffer));
										bzero(buffer, 1024);
									}
								}
								for (z = 0; z < 5; z++)
								{
									if (myfiles[x].collab[z][0] != -1 && myfiles[x].collab[z][1] == 2)
									{
										snprintf(buffer, sizeof(buffer), "Editor : %d\t", myfiles[x].collab[z][0]);
										write(newSocket, buffer, sizeof(buffer));
										bzero(buffer, 1024);
										read(newSocket, buffer, sizeof(buffer));
										bzero(buffer, 1024);
									}
								}
							}
						}
					}
					else if (strncmp("upload", buffer, 6) == 0)
					{
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						FILE *fp, *fp1;
						int n = 0;
						read(newSocket, buffer, sizeof(buffer));
						for (x = 0; x < 100; x++)
						{
							if (myfiles[x].valid == 1 && strcmp(myfiles[x].fname, buffer) == 0)
							{
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "Not valid file name.File Already Exists.\n");
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								n = 1;
								break;
							}
						}
						if (n == 1)
							continue;
						for (x = 0; x < 100; x++)
						{
							if (myfiles[x].valid == -1)
							{
								strcpy(myfiles[x].fname, buffer);
								myfiles[x].owner = activeclients[i];
								myfiles[x].valid = 1;
								myfiles[x].collab[0][0] = -1;
								myfiles[x].collab[1][0] = -1;
								myfiles[x].collab[2][0] = -1;
								myfiles[x].collab[3][0] = -1;
								myfiles[x].collab[4][0] = -1;
								break;
							}
						}
						char fname1[1024];
						bzero(fname1, 1024);
						snprintf(fname1, sizeof(fname1), "%d_%s", activeclients[i], buffer);
						fp = fopen(fname1, "w");
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						write_file(fp, newSocket);
						fclose(fp);
						write_perm();
					}
					else if (strncmp("read", buffer, 4) == 0)
					{
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						FILE *fp, *fp1;
						int n = 0;
						read(newSocket, buffer, sizeof(buffer));
						char fname1[1024];
						bzero(fname1, 1024);
						for (x = 0; x < 100; x++)
						{
							if (myfiles[x].valid == 1 && strcmp(myfiles[x].fname, buffer) == 0)
							{
								snprintf(fname1, sizeof(fname1), "%d_%s", myfiles[x].owner, myfiles[x].fname);
								if (iviewer(x, activeclients[i]) != 1)
								{
									bzero(buffer, 1024);
									snprintf(buffer, sizeof(buffer), "Invalid Action.You do not have permission to view this file.\n");
									write(newSocket, buffer, sizeof(buffer));
									bzero(buffer, 1024);
									n = 2;
									break;
								}
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "ok");
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								n = 1;
								continue;
							}
						}
						if (n == 2)
							continue;
						if (n != 1)
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "Not valid file name.File does not exist.\n");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
							continue;
						}
						fp = fopen(fname1, "r");
						int z = cline(fp);
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						int ln1 = atoi(buffer);
						if (ln1 < 0)
						{
							ln1 = z + ln1;
						}
						bzero(buffer, SIZE);
						if (ln1 >= z || ln1 < 0)
						{
							snprintf(buffer, sizeof(buffer), "File doesn't have the line number %d.\n", ln1);
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 255);
							continue;
						}
						else
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "ok");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
						}
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						int ln2 = atoi(buffer);
						if (ln2 < 0)
						{
							ln2 = z + ln2;
						}
						else if (ln2 == 999999999)
						{
							ln2 = z - 1;
						}
						bzero(buffer, SIZE);
						n = 0;
						if (ln2 >= z || ln2 < 0)
						{
							snprintf(buffer, sizeof(buffer), "File doesn't have the line number %d.\n", ln2);
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 255);
							continue;
						}
						else
						{
							if (ln2 < ln1)
							{
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "End line number should not be less that start line number.\n");
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								n = 2;
							}
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "ok");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
						}
						if (n == 2)
							continue;
						int tot = ln2 - ln1 + 1;
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "%d", tot);
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						read(newSocket, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						rline(fp, newSocket, ln1, ln2);
						fclose(fp);
					}
					else if (strncmp("insert", buffer, 4) == 0)
					{
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						FILE *fp, *fp1;
						int n = 0;
						read(newSocket, buffer, sizeof(buffer));
						char fname1[1024];
						bzero(fname1, 1024);
						for (x = 0; x < 100; x++)
						{
							if (myfiles[x].valid == 1 && strcmp(myfiles[x].fname, buffer) == 0)
							{
								snprintf(fname1, sizeof(fname1), "%d_%s", myfiles[x].owner, myfiles[x].fname);
								if (ieditor(x, activeclients[i]) != 1)
								{
									bzero(buffer, 1024);
									snprintf(buffer, sizeof(buffer), "Invalid Action.You do not have permission to edit this file.\n");
									write(newSocket, buffer, sizeof(buffer));
									bzero(buffer, 1024);
									n = 2;
									break;
								}
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "ok");
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								n = 1;
								continue;
							}
						}
						if (n == 2)
							continue;
						if (n != 1)
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "Not valid file name.File does not exist.\n");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
							continue;
						}
						fp = fopen(fname1, "r");
						int z = cline(fp);
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						int ln1 = atoi(buffer);
						if (ln1 < 0)
						{
							ln1 = z + ln1;
						}
						if (ln1 == z || ln1 < 0)
						{
							snprintf(buffer, sizeof(buffer), "File doesn't have the line number %d.\n", ln1);
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 255);
							continue;
						}
						else if (ln1 == 999999999)
						{
							ln1 = z;
						}
						bzero(buffer, SIZE);
						if (ln1 > z || ln1 < 0)
						{
							snprintf(buffer, sizeof(buffer), "File doesn't have the line number %d.\n", ln1);
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 255);
							continue;
						}
						else
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "ok");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
						}
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						iline(fp, ln1, buffer);
						fclose(fp);
						remove(fname1);
						rename("server_file1.txt", fname1);
						fp = fopen(fname1, "r");
						int tot = cline(fp);
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "%d", tot);
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						read(newSocket, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						send_file(fp, newSocket);
						fclose(fp);
					}
					else if (strncmp("delete", buffer, 6) == 0)
					{
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						FILE *fp, *fp1;
						int n = 0;
						read(newSocket, buffer, sizeof(buffer));
						char fname1[1024];
						bzero(fname1, 1024);
						for (x = 0; x < 100; x++)
						{
							if (myfiles[x].valid == 1 && strcmp(myfiles[x].fname, buffer) == 0)
							{
								snprintf(fname1, sizeof(fname1), "%d_%s", myfiles[x].owner, myfiles[x].fname);
								if (ieditor(x, activeclients[i]) != 1)
								{
									bzero(buffer, 1024);
									snprintf(buffer, sizeof(buffer), "Invalid Action.You do not have permission to edit this file.\n");
									write(newSocket, buffer, sizeof(buffer));
									bzero(buffer, 1024);
									n = 2;
									break;
								}
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "ok");
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								n = 1;
								continue;
							}
						}
						if (n == 2)
							continue;
						if (n != 1)
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "Not valid file name.File does not exist.\n");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
							continue;
						}
						fp = fopen(fname1, "r");
						int z = cline(fp);
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						int ln1 = atoi(buffer);
						if (ln1 < 0)
						{
							ln1 = z + ln1;
						}
						bzero(buffer, SIZE);
						if (ln1 >= z || ln1 < 0)
						{
							snprintf(buffer, sizeof(buffer), "File doesn't have the line number %d.\n", ln1);
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 255);
							continue;
						}
						else
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "ok");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
						}
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						int ln2 = atoi(buffer);
						if (ln2 < 0)
						{
							ln2 = z + ln2;
						}
						else if (ln2 == 999999999)
						{
							ln2 = z - 1;
						}
						bzero(buffer, SIZE);
						n = 0;
						if (ln2 >= z || ln2 < 0)
						{
							snprintf(buffer, sizeof(buffer), "File doesn't have the line number %d.\n", ln2);
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 255);
							continue;
						}
						else
						{
							if (ln2 < ln1)
							{
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "End line number should not be less that start line number.\n");
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								n = 2;
							}
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "ok");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
						}
						if (n == 2)
							continue;
						dline(fp, ln1, ln2);
						fclose(fp);
						remove(fname1);
						rename("server_file1.txt", fname1);
						fp = fopen(fname1, "r");
						int tot = cline(fp);
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "%d", tot);
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						read(newSocket, buffer, sizeof(buffer));
						bzero(buffer, SIZE);
						send_file(fp, newSocket);
						fclose(fp);
					}
					else if (strncmp("download", buffer, 8) == 0)
					{
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "ok");
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						FILE *fp;
						int n = 0;
						read(newSocket, buffer, sizeof(buffer));
						char fname1[1024];
						bzero(fname1, 1024);
						for (x = 0; x < 100; x++)
						{
							if (myfiles[x].valid == 1 && strcmp(myfiles[x].fname, buffer) == 0)
							{
								snprintf(fname1, sizeof(fname1), "%d_%s", myfiles[x].owner, myfiles[x].fname);
								if (iviewer(x, activeclients[i]) != 1)
								{
									bzero(buffer, 1024);
									snprintf(buffer, sizeof(buffer), "Invalid Action.You do not have permission to view this file.\n");
									write(newSocket, buffer, sizeof(buffer));
									bzero(buffer, 1024);
									n = 2;
									break;
								}
								bzero(buffer, 1024);
								snprintf(buffer, sizeof(buffer), "ok");
								write(newSocket, buffer, sizeof(buffer));
								bzero(buffer, 1024);
								n = 1;
								continue;
							}
						}
						if (n == 2)
							continue;
						if (n != 1)
						{
							bzero(buffer, 1024);
							snprintf(buffer, sizeof(buffer), "Not valid file name.File does not exist.\n");
							write(newSocket, buffer, sizeof(buffer));
							bzero(buffer, 1024);
							continue;
						}
						fp = fopen(fname1, "r");
						int z = cline(fp);
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "%d", z);
						write(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						read(newSocket, buffer, sizeof(buffer));
						bzero(buffer, 1024);
						send_file(fp, newSocket);
						fclose(fp);
					}
					else
					{
						delclient(activeclients[i]);
						printf("Disconnected from client.\n");
						close(newSocket);
						bzero(buffer, 1024);
						snprintf(buffer, sizeof(buffer), "exit");
						write(client_socket[i + 5], buffer, sizeof(buffer));
						bzero(buffer, 1024);
						close(client_socket[i + 5]);
						client_socket[i] = 0;
						client_socket[i + 5] = 0;
						activeclients[i] = -1;
					}
				}
			}
		}
	}

	return 0;
}
