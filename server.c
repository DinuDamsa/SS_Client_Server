#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>

#define PORT 	1234
#define ADDR 	"127.0.0.1"

int main() {
	int server_d;
	int client_d;
	/* client and server DS */
	struct sockaddr_in server, client;

	/*
	 * create socket for server
	 */
	if ((server_d = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create server");
		return -1;
	}

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
	memset(&client, 0, sizeof(client));

	/*
	 * bind server
	 */
	if (bind(server_d, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("Unable to bind server");
		return -1;
	}

	/* listen for connections */
	listen(server_d, 1);

	for(;;) {
		/* accept client */
		client_d = accept(server_d, (struct sockaddr *) &client, sizeof(client));
		printf("Client connected.\n");
		/* receive file path */
		size_t path_size;
		recv(client_d, &path_size, sizeof(path_size), MSG_WAITALL);
		printf("Received size: %d\n", path_size);
		char *buffer = (char *) malloc(path_size * sizeof(char));
		recv(client_d, buffer, path_size, MSG_WAITALL);
		printf("Received path: %s\n", buffer);

		/* receive text */
		//TODO: pay attention on receiving multiple chunks from client
		ssize_t text_size;
		recv(client_d, &text_size, sizeof(text_size), MSG_WAITALL);
		printf("Received size: %d\n", text_size);
		char *buff_text = (char*) malloc(sizeof(char)*text_size);
		recv(client_d, buff_text, text_size, MSG_WAITALL);
		printf("Received text: %s\n", buff_text);

		//implement file opening and write data received from client
		
		/*get current directory*/
		char current_directory_absolute_path[PATH_MAX];
		if (getcwd(current_directory_absolute_path, PATH_MAX) == NULL) {
			perror("Couldn't get current directory!");
			free(filepath_name);
			return -1;
		} else {
			printf("Current working directory: %s\n", current_directory_absolute_path);
		}
		
		

		/* free buffer memory */
		free(buffer);
		free(buff_text);

		/* close client */
		close(client_d);
		printf("Client disconnected\n");
	}
}
