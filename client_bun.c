#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BUFF_SIZE 128

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Incorrect usage\n");
		printf("Should be called: ./client <filepath_name>\n");
		return -1;
	}
	int c;
	struct sockaddr_in server;

	c = socket(AF_INET, SOCK_STREAM, 0);
	if (c < 0) {
		printf("Eroare la crearea socketului client\n");
		return -1;
	}

	memset(&server, 0, sizeof(server));
	server.sin_port = htons(1234);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
		
	if (connect(c, (struct sockaddr *) &server, sizeof(server)) < 0) {
		printf("Eroare la conectarea la server\n");
		return -1;
	}
	unsigned int path_size = strlen(argv[1]) + 1;
	printf("Trimit size: %d\n", path_size);
	printf("Trimit path: %s\n", argv[1]);
	send(c, &path_size, sizeof(path_size), 0);
	send(c, argv[1], path_size, 0);
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		printf("Error on file open.\n");
		return -1;
	}
	
	//struct stat st;
	//stat(filename, &st);
	//size = st.st_size;
	
	char buf[MAX_BUFF_SIZE];
	unsigned int bytes_read = read(fd, buf, MAX_BUFF_SIZE);
	while (bytes_read > 0){
		send(c, buf, bytes_read, 0);
		bytes_read = read(fd, buf, MAX_BUFF_SIZE);
	}
	//path -> argv[1]
	//b = htons(b);
	//send(c, &a, sizeof(a), 0);
	//send(c, &b, sizeof(b), 0);
	//recv(c, &suma, sizeof(suma), 0);
	//suma = ntohs(suma);
	//printf("Suma este %hu\n", suma);
	//b = htons(b);
	close(c);
	return 0;
}
