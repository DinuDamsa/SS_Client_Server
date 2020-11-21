#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#define PORT 	1234
#define ADDR 	"127.0.0.1"

int main(int argc, char **argv) {

	/*
	 * check if the number of arguments are correct
	 */
	// if (argc != 2) {
	// 	printf("Incorrect usage!\n");
	// 	printf("Should be called: ./client <filepath_name>\n");
	// 	return -1;
	// }


	/*
	 * get path of file from cli rather than from the argv
	 */
	size_t size = PATH_MAX;
	char *filepath_name = (char*)malloc(sizeof(char)*size);
	size_t filepath_size;
	if((filepath_size = getline(&filepath_name, &size, stdin)) == -1) {
		perror("Unable to read from console.\n");
		return -1;
	}
	// getline keeps the '\n' if exists, delete it
	if(filepath_name[filepath_size-1] == '\n') filepath_size--;
	filepath_name[filepath_size]='\0';
	printf("%s  %d\n", filepath_name, filepath_size);

	/*
	 * getcwd gets the absolute path for the current folder (client folder)
	 * returns NULL if the current path is larger that PATH_MAX
	 */
	char current_folder_absolute_path[PATH_MAX];
	if (getcwd(current_folder_absolute_path, PATH_MAX) == NULL) {
		perror("Couldn't get current folder!");
		return -1;
	} else {
		printf("Current working folder: %s\n", current_folder_absolute_path);
	}

	/*
	 * realpath gets the absolute path for a file that is relative to the current folder
	 * returns NULL if file is not found
	 * allocs memory, don't forget to free it
	 */
	char* desired_file_absolute_path;
	if((desired_file_absolute_path = realpath(filepath_name, NULL)) == NULL){
		perror("Cannot find file with the given name");
		return -1;
	} else{
		printf("Path found: %s\n", desired_file_absolute_path);
		free(desired_file_absolute_path); // REVIEW: delete this and free after use
	}

	// TODO: compare current_folder_absolute_path and desired_file_absolute_path
	// desired_file_absolute_path should be $current_folder_absolute_path/(.)+

	/* client and server DS */
	int client_d;
	struct sockaddr_in server;

	/*
	 * create client socket
	 */
	if ((client_d = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create client");
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
		perror("Unable to connect to server");
		return -1;
	}

	/*
	 * open given file
	 */
	int fd = open(filepath_name, O_RDONLY);
	if (fd < 0) {
		perror("File does not exist or cannot be opened");
		return -1;
	}

	/*
	 * send file
	 */
	unsigned int path_size = filepath_size + 1;
	printf("Sending size: %d\n", path_size);
	printf("Sending path: %s\n", filepath_name);
	send(client_d, &path_size, sizeof(path_size), 0);
	send(client_d, filepath_name, path_size, 0);

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
