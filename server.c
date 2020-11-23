#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#define PORT 	1234

static void exit_with_status(int client_d, short status) {
	if(send(client_d, &status, sizeof(status), 0) < 0) {
		perror("Client disconnected");
	}
	close(client_d);
}

int main() {
	int server_d;
	int client_d;
	/* client and server DS */
	struct sockaddr_in server, client;

	/*
		* set server
	*/
	memset(&server, 0, sizeof(server));
	server.sin_port = htons(PORT);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;

	/*
		* set client
	*/
	socklen_t client_socket_size = sizeof(struct sockaddr_in);
	memset(&client, 0, sizeof(client));

	/*
		* create socket for server & bind
	*/
	if (
		((server_d = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	||	(bind(server_d, (struct sockaddr *) &server, sizeof(server)) < 0)
	) {
		perror("Error");
		return -1;
	}

	/*
		* listen for connections; listen() returns -1 if something goes wrong; 0 otherwise
	*/
	if(listen(server_d, 1) == -1) {
		perror("Error");
		return -1;
	}

	/*
		* get current directory
		* this is null terminated
	*/
	char current_directory_absolute_path[PATH_MAX];
	if (getcwd(current_directory_absolute_path, PATH_MAX) == NULL) {
		perror("Couldn't get current directory!");
		return -1;
	} else {
		printf("Current working directory: %s\n", current_directory_absolute_path);
	}

	for(;;) {
		/* accept client */
		client_d = accept(server_d, (struct sockaddr *) &client, &client_socket_size);
		printf("Client connected.\n");
		/* receive file path */
		/*
			* NOTE: recv only send "-1" as error if it was called in a non-blocking manner
			* check for error if this is changed
		*/
		size_t path_size = 0;
		char *buffer = (char *) malloc(path_size * sizeof(char));
		if (
			(recv(client_d, &path_size, sizeof(path_size), MSG_WAITALL) <= 0)
		||  (recv(client_d, buffer, path_size, MSG_WAITALL) <= 0)
		) {
			exit_with_status(client_d, -1);
			free(buffer);
			printf("Error while receiving file!\n");
			continue;
		}
		printf("Received size: %ld\n", path_size);
		printf("Received path: %s\n", buffer);

		/*
			* open file to write received data
			* create if there is no such file
			* the received file is relative to server's current folder
		*/
		int desired_file_descriptor;
		if ((desired_file_descriptor = open(buffer, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO)) == -1) {
			exit_with_status(client_d, -1);
			free(buffer);
			perror("Error");
			return -1;
		}
		free(buffer);

		/* receive chunks of bytes */
		long bytes_chunks;
		if(recv(client_d, &bytes_chunks, sizeof(bytes_chunks), MSG_WAITALL) <= 0) {
			exit_with_status(client_d, -1);
			printf("Error while receiving chunks.\n");
			continue;
		}
		printf("Received chunks: %ld\n", bytes_chunks);
		while(bytes_chunks--) {
			size_t text_size;
			if(recv(client_d, &text_size, sizeof(text_size), MSG_WAITALL) <= 0) {
				exit_with_status(client_d, -1);
				printf("Error while size of file.\n");
				break;
			}
			printf("Received size: %ld\n", text_size);
			char *buff_text = (char*) malloc(sizeof(char)*text_size);
			if(recv(client_d, buff_text, text_size, MSG_WAITALL) < 0) {
				exit_with_status(client_d, -1);
				free(buff_text);
				printf("Error while size of file.\n");
				break;
			}
			printf("Received text: %s\n", buff_text);
			// buff_text[text_size] = '\0';
			ssize_t written_bytes = write(desired_file_descriptor, buff_text, text_size);
			if(written_bytes < 0) {
				free(buff_text);
				exit_with_status(client_d, -1);
				perror("Error writing new file");
				break;
			} else {
				printf("Wrote %zd bytes in file\n", written_bytes);
			}
			free(buff_text);
		}

		/* close file */
		close(desired_file_descriptor);

		/* close client */
		exit_with_status(client_d, 0);
		printf("Client disconnected\n");
	}
}
