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
	/* REVIEW: don't forget to free this */
	size_t filepath_size;
	if((filepath_size = getline(&filepath_name, &size, stdin)) == -1) {
		perror("Unable to read from console.\n");
		free(filepath_name);
		return -1;
	}
	// getline keeps the '\n' if exists, delete it
	if(filepath_name[filepath_size-1] == '\n') filepath_size--;
	filepath_name[filepath_size]='\0';
	printf("%s  %d\n", filepath_name, filepath_size);

	/*
	 * getcwd gets the absolute path for the current directory (client directory)
	 * returns NULL if the current path is larger that PATH_MAX
	 */
	char current_directory_absolute_path[PATH_MAX];
	if (getcwd(current_directory_absolute_path, PATH_MAX) == NULL) {
		perror("Couldn't get current directory!");
		free(filepath_name);
		return -1;
	} else {
		printf("Current working directory: %s\n", current_directory_absolute_path);
	}

	/*
	 * realpath gets the absolute path for a file that is relative to the current directory
	 * returns NULL if file is not found
	 * allocs memory, don't forget to free it
	 */
	char* desired_file_absolute_path;
	if((desired_file_absolute_path = realpath(filepath_name, NULL)) == NULL){
		perror("Cannot find file with the given name");
		free(filepath_name);
		return -1;
	} else{
		printf("Path found: %s\n", desired_file_absolute_path);
		// free(desired_file_absolute_path); // REVIEW: delete this and free after use
	}

	/* we don't need filepath_name from now on */
	free(filepath_name);

	/*
	 * check if the desired file is inside the current directory
	 */
	for(int i=0; current_directory_absolute_path[i]; i++) {
		if(current_directory_absolute_path[i] != desired_file_absolute_path[i]) {
			printf("Given path is not in the current directory!\n");
			free(desired_file_absolute_path);
			return -1;
		}
	}

	/* client and server DS */
	int client_d;
	struct sockaddr_in server;

	/*
	 * create client socket
	 */
	if ((client_d = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create client");
		free(desired_file_absolute_path);
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
	if ((connect(client_d, (struct sockaddr *) &server, sizeof(server))) == -1) {
		perror("Unable to connect to server");
		free(desired_file_absolute_path);
		return -1;
	}

	/*
	 * open given file
	 */
	int desired_file_descriptor;
	if ((desired_file_descriptor = open(desired_file_absolute_path, O_RDONLY)) == -1) {
		perror("File does not exist or cannot be opened");
		free(desired_file_absolute_path);
		return -1;
	}

	/*
	 * send file
	 */
	unsigned int path_size = strlen(desired_file_absolute_path) + 1;
	printf("Sending size: %d\n", path_size);
	printf("Sending path: %s\n", desired_file_absolute_path);
	send(client_d, &path_size, sizeof(path_size), 0);
	send(client_d, desired_file_absolute_path, path_size, 0);

	/*
	 * get size of file
	 * this will be divided into chunks I think
	 */
	struct stat st;
	fstat(desired_file_descriptor, &st);
	int file_size = st.st_size;
	printf("Size of given file: %d\n", file_size);

	char buf[file_size+1];
	unsigned int bytes_read;
	/*
	 * read file
	 */
	while (bytes_read = read(desired_file_descriptor, buf, file_size)) {
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

	free(desired_file_absolute_path);

	/*
	 * close client
	 */
	close(client_d);
	return 0;
}
