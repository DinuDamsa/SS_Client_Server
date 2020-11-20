#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 	1234
#define ADDR 	"127.0.0.1"

int main(int argc, char **argv) {

	/*
	 * check if the number of arguments are correct
	 */
	if (argc != 2) {
		printf("Incorrect usage!\n");
		printf("Should be called: ./client <filepath_name>\n");
		return -1;
	}

	/* client and server DS */
	int client_d;
	struct sockaddr_in server;

	/*
	 * create client socket
	 */
	client_d = socket(AF_INET, SOCK_STREAM, 0);
	if (client_d < 0) {
		printf("Unable to create client!\n");
		return -1;
	}

	/*
	 * set server
	 */
	memset(&server, 0, sizeof(server));
	server.sin_port = htons(PORT);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(ADDR);

	/*
	 * connect to server
	 */
	if (connect(client_d, (struct sockaddr *) &server, sizeof(server)) < 0) {
		printf("Unable to connect to server!\n");
		return -1;
	}

	/*
	 * open given file
	 */
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		printf("File does not exist or cannot be opened.\n");
		return -1;
	}

	/*
	 * send file
	 */
	unsigned int path_size = strlen(argv[1]) + 1;
	printf("Sending size: %d\n", path_size);
	printf("Sending path: %s\n", argv[1]);
	send(client_d, &path_size, sizeof(path_size), 0);
	send(client_d, argv[1], path_size, 0);

	/*
	 * get size of file
	 * this will be divided into chunks I think
	 */
	struct stat st;
	fstat(fd, &st);
	int file_size = st.st_size;
	printf("Size of given file: %d\n", file_size);

	char buf[file_size+1];
	unsigned int bytes_read;
	/*
	 * read file
	 */
	while (bytes_read = read(fd, buf, file_size)) {
		buf[bytes_read++] = '\0';
		printf("Read %d bytes: %s\n", bytes_read, buf);
		/*
		 * send number of bytes read from file
		 */
		send(client_d, &bytes_read, sizeof(bytes_read), 0);
		/*
		 * send text read from file
		 */
		send(client_d, buf, bytes_read, 0);
	}
	/*
	 * close client
	 */
	close(client_d);
	return 0;
}
