#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <limits.h>
#define PORT 	1234
#define ADDR 	"127.0.0.1"

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
		size_t path_size;
		/*
		 * NOTE: recv only send "-1" as error if it was called in a non-blocking manner
		 * check for error if this is changed
		 */
		recv(client_d, &path_size, sizeof(path_size), MSG_WAITALL);
		printf("Received size: %ld\n", path_size);
		char *buffer = (char *) malloc(path_size * sizeof(char));
		recv(client_d, buffer, path_size, MSG_WAITALL);
		printf("Received path: %s\n", buffer);

		/* receive chunks of bytes */
		int bytes_chunks;
		recv(client_d, &bytes_chunks, sizeof(bytes_chunks), MSG_WAITALL);
		printf("Received chunks: %d\n", bytes_chunks);
		while(bytes_chunks--) {
			ssize_t text_size;
			recv(client_d, &text_size, sizeof(text_size), MSG_WAITALL);
			printf("Received size: %ld\n", text_size);
			char *buff_text = (char*) malloc(sizeof(char)*text_size);
			recv(client_d, buff_text, text_size, MSG_WAITALL);
			// printf("Received text: %s\n", buff_text);
			// TODO: implement file opening and write data received from client
			free(buff_text);
		}

		/* free buffer memory */
		free(buffer);

		/* close client */
		close(client_d);
		printf("Client disconnected\n");
	}

	return 0;
}
