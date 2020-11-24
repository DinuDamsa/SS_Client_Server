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

int main() {

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
	}

	/*
		* NOTE: get path of file from stdin
		* getline returns -1 if unable to read from given input
	*/
	size_t size = PATH_MAX; /* max size for input */
	ssize_t filepath_size;  /* this will have the actual size of the given input */
	char *filepath_name = (char*)malloc(sizeof(char)*PATH_MAX + 1);
	/* REVIEW: don't forget to free this */
	if (NULL == filepath_name) {
		perror("Error on memory allocation");
		return -1;
	}

	if((filepath_size = getline(&filepath_name, &size, stdin)) == -1) {
		perror("Unable to read from console.\n");
		free(filepath_name);
		return -1;
	}
	if(filepath_name[filepath_size-1] == '\n') filepath_size--;
	filepath_name[filepath_size]='\0';

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
	size_t common = 0;
	for(; current_directory_absolute_path[common]; common++) {
		if(current_directory_absolute_path[common] != desired_file_absolute_path[common]) {
			printf("Given path is not in the current directory!\n");
			free(desired_file_absolute_path);
			return -1;
		}
	}
	/*
		* create file to give to server
	*/
	size_t size_file_to_write = strlen(desired_file_absolute_path) - common + 2;
	char* file_to_write = (char*)malloc(sizeof(char) * size_file_to_write);
	if (NULL == file_to_write) {
		perror("Error on memory allocation");
		free(desired_file_absolute_path);
		return -1;
	}
	file_to_write[0] = 's';
	file_to_write[1] = '_';
	/* REVIEW: change algo for file name sent to server */
	if(desired_file_absolute_path[common] == '/') common++;
	for(size_t i=common; desired_file_absolute_path[i]; i++) {
		file_to_write[i-common+2] = desired_file_absolute_path[i];
	}
	file_to_write[size_file_to_write-1] = '\0';

	/*
		* open given file
		* NOTE: this works only if the client has permission to read the file
	*/
	int desired_file_descriptor;
	if ((desired_file_descriptor = open(desired_file_absolute_path, O_RDONLY)) == -1) {
		perror("Error");
		free(desired_file_absolute_path);
		free(file_to_write);
		return -1;
	}
	/* no need for desired_file_absolute_path from now on */
	free(desired_file_absolute_path);

	/*
		* get size of file
		* this will be divided into chunks I think
		*
		* IDEA: let stack free fstat immediately after getting file size
		* (maybe the compiler does this anyway if it sees we don't use it anymore idk)
	*/
	__off_t file_size;
	{
	struct stat st;
	fstat(desired_file_descriptor, &st);
	file_size = st.st_size;
	}

	/*
		* decide the number of chunks to send to the server based on overall size
		* using PIPE_BUF (Maximum number of bytes guaranteed to be written automatically
		* to a pipe.) from limits.h
	*/
	long bytes_chunks = file_size/(PIPE_BUF-1) + (file_size%(PIPE_BUF-1) != 0);

	/*
		* send file name to server
		* NOTE: (from man) "No indication of failure to deliver is implicit in a send().
		* Locally detected errors are indicated by a return value of -1."
		* NOTE: this fails with SIGPIPE if server is closed before sending this
	*/
	if (
		(send(client_d, &size_file_to_write, sizeof(size_file_to_write), 0) < 0)
	||	(send(client_d, file_to_write, size_file_to_write, 0) < 0)
	) {
		perror("Unable to send to server");
		free(file_to_write);
		return -1;
	} else {
		printf("Sent size: %ld\n", size_file_to_write);
		printf("Sent path: %s\n", file_to_write);
	}

	free(file_to_write);

	/*
		* send how many chunks will be sent
	*/
	if(send(client_d, &bytes_chunks, sizeof(bytes_chunks), 0) <0) {
		perror("Unable to send to server");
		return -1;
	} else {
		printf("Sent chunk: %ld\n", bytes_chunks);
	}

	/*
		* read&send file
	*/
	char buf[PIPE_BUF];
	ssize_t bytes_read;
	while ((bytes_read = read(desired_file_descriptor, buf, PIPE_BUF-1)) > 0) {
		buf[++bytes_read] = '\0'; /* send the string null-terminated */
		printf("Read %ld bytes: %s\n", bytes_read, buf);
		size_t bytes_to_send = (size_t)bytes_read;
		/*
			* send size of text & text read from file
			* exists with SIGPIPE if server closes randomly
		*/
		if(
			((send(client_d, &bytes_to_send, sizeof(bytes_to_send), 0)) < 0)
		||	((send(client_d, buf, bytes_to_send, 0)) < 0)
		) {
			perror("Error");
			return -1;
		} else {
			printf("Sent size of chunk: %zu\n", bytes_to_send);
			printf("Sent data of chunk: %s\n", buf);
		}
	}

	if (bytes_read < 0){
		perror("Error while reading file content");
		return -1;
	}

	/*
		* close client
	*/
	short response;
	recv(client_d, &response, sizeof(response), MSG_WAITALL);

	printf("%s\n", !response ? "Succesful" : "Failed" );

	close(client_d);
	return 0;
}
