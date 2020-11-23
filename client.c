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

	/* client and server DS */
	int client_d;
	struct sockaddr_in server;

	/*
	 * setup server
	 */
	memset(&server, 0, sizeof(server));
	server.sin_port = htons(PORT);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(ADDR);

	/*
	 * NOTE: create client socket & connect to server
	 */
	if(
		((client_d = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	||	((connect(client_d, (struct sockaddr *) &server, sizeof(server))) == -1)
	) {
		perror("Error");
		return -1;
	}
	/* client is connected to server */

	/*
	 * NOTE: getcwd gets the absolute path for the current directory (client directory)
	 * returns NULL if the current path is larger that PATH_MAX
	 */
	char current_directory_absolute_path[PATH_MAX];
	if (getcwd(current_directory_absolute_path, PATH_MAX) == NULL) {
		perror("Couldn't get current directory!");
		return -1;
	} else {
		printf("Current working directory: %s\n", current_directory_absolute_path);
	}

	/*
	 * NOTE: get path of file from stdin
	 * getline returns -1 if unable to read from given input
	 */
	size_t size = PATH_MAX; /* max size for input */
	size_t filepath_size; /* this will have the actual size of the given input */
	char *filepath_name = (char*)malloc(sizeof(char)*PATH_MAX + 1);
	/* REVIEW: don't forget to free this */

	if((filepath_size = getline(&filepath_name, &size, stdin)) == -1) {
		perror("Unable to read from console.\n");
		free(filepath_name);
		return -1;
	}
	/* IDEA: getline keeps the '\n' if exists, remove it */
	if(filepath_name[filepath_size-1] == '\n') filepath_size--;
	filepath_name[filepath_size]='\0';
	printf("%s  %ld\n", filepath_name, filepath_size);

	/*
	 * NOTE: realpath gets the absolute path for a file
	 * that is relative to the
	 * current directory
	 * returns NULL if file is not found
	 * realpath(...) allocs memory, don't forget to free it
	 * (FROM MAN) "The resulting pathname is stored as a
	 * null-terminated string, up to a maximum of
     * PATH_MAX bytes, in the buffer pointed to by resolved_path."
	 */
	char* desired_file_absolute_path;
	if((desired_file_absolute_path = realpath(filepath_name, NULL)) == NULL){
		perror("Cannot find file with the given name");
		free(filepath_name);
		return -1;
	} else {
		printf("Path found: %s\n", desired_file_absolute_path);
	}

	/*
	 * IDEA: don't work with given input from now on, use only the absolute paths
	 */
	free(filepath_name);
	/*
	 * XXX: using filepath_name from here on will probably work but will give size 0
	 */

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

	/*
	 * open given file
	 * NOTE: this works only if the client has permission to read the file
	 */
	int desired_file_descriptor;
	if ((desired_file_descriptor = open(desired_file_absolute_path, O_RDONLY)) == -1) {
		perror("Error");
		free(desired_file_absolute_path);
		return -1;
	}

	/*
	 * send file name to server
	 * NOTE: (from man) "No indication of failure to deliver is implicit in a send().
	 * Locally detected errors are indicated by a return value of -1."
	 * XXX: this fails with SIGPIPE if server is closed before sending this
	 */
	size_t path_size = strlen(desired_file_absolute_path) + 1;
	if (
		(send(client_d, &path_size, sizeof(path_size), 0) < 0)
	||	(send(client_d, desired_file_absolute_path, path_size, 0) < 0)
	) {
		perror("Unable to send to server");
		free(desired_file_absolute_path);
		return -1;
	} else {
		printf("Sending size: %ld\n", path_size);
		printf("Sending path: %s\n", desired_file_absolute_path);
	}

	/*
	 * get size of file
	 * this will be divided into chunks I think
	 *
	 * IDEA: let stack free fstat immediately after getting file size
	 * (maybe the compiler does this anyway if it sees we don't use it anymore idk)
	 */
	int file_size;
	{
	struct stat st;
	fstat(desired_file_descriptor, &st);
	file_size = st.st_size;
	}
	printf("Size of given file: %d\n", file_size);

	/*
	 * decide the number of chunks to send to the server based on overall size
	 * using PIPE_BUF (Maximum number of bytes guaranteed to be written automatically
	 * to a pipe.) from limits.h
	 */
	int bytes_chunks = file_size/(PIPE_BUF-1) + (file_size%(PIPE_BUF-1) != 0);
	printf("My PIPE_BUF: %d\n", PIPE_BUF);
	printf("Will send %d chunks.\n", bytes_chunks);

	/*
	 * send how many chunks will be sent
	 */
	if(send(client_d, &bytes_chunks, sizeof(bytes_chunks), 0) <0) {
		perror("Unable to send to server");
		free(desired_file_absolute_path);
		return -1;
	}

	/*
	 * read&send file
	 */
	char buf[PIPE_BUF];
	ssize_t bytes_read;
	while ((bytes_read = read(desired_file_descriptor, buf, PIPE_BUF-1)) > 0) {
		buf[++bytes_read] = '\0'; /* send the string null-terminated */
		/* printf("Read %ld bytes: %s\n", bytes_read, buf); */
		/*
		 * send size of text & text read from file
		 * exists with SIGPIPE if server closes randomly
		 */
		if(
			((send(client_d, &bytes_read, sizeof(bytes_read), 0)) < 0)
		||	((send(client_d, buf, bytes_read, 0)) < 0)
		) {
			perror("Error");
			free(desired_file_absolute_path);
			return -1;
		}
	}

	/* QUESTION: in which case does this fail to read? */
	if (bytes_read < 0){
		perror("Error while reading file content");
		free(desired_file_absolute_path);
		return -1;
	}

	/* free everything left */
	free(desired_file_absolute_path);

	/*
	 * close client
	 */
	close(client_d);
	return 0;
}
