#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

// Function which helps us to establish a connection via socket to the web server
// host: hostname (e.g. www.google.com)
// port: WEB port (80)
int socket_connect(char *host, char *port){
	struct sockaddr_in6 addr;
	
	// some information parsed to the web server
	// hints indicates to the server information about the client (us)
	struct addrinfo hints;
	// here we will store the information sent from the web server
	struct addrinfo *result;
	// s: indicates the error code if there were something wrong when we
	// tried to receive info from the web server
	// sock: descriptor of the WEB socket
	int sock, s;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	// getaddrinfo will send us the information needed for connecting to the WEB server
	s = getaddrinfo(host, port, &hints, &result);
	if (s != 0) {
        	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        	exit(EXIT_FAILURE);
        }
	
	// creating a new socket with the information received by getaddrinfo
	sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if(sock == -1){
		perror("setsockopt");
		exit(1);
	}
	
	// connecting to the WEB server (socket)
	if(connect(sock, result->ai_addr, result->ai_addrlen) == -1){
		perror("connect");
		exit(1);
	}
	
	// deallocating the memory used by result
	freeaddrinfo(result);
	// send the socket descriptor so we can use it
	return sock;
}
 
#define BUFFER_SIZE 1024 // chucks maximum size (normally it has to be a power of 2)
#define HEADER_SIZE 512  // header maximum size

int main(int argc, char *argv[]) {
	// The descriptor of socket received by the web server
	int fd;
	// The buffer where we keep the chucks of data received from the GET request from the web server
	char buffer[BUFFER_SIZE];

	// We verify if the number of given arguments is valid (more than 2)
	if(argc < 3) {
		fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
		exit(1); 
	}
	
	// getting the web socket
	fd = socket_connect(argv[1], argv[2]);

	// The required headers for our GET request
	char req_header[HEADER_SIZE];
	// defining our GET request headers
	sprintf(req_header, "GET /\r\n HTTP/1.0\r\nHost: %s\r\nContent-Type: text/html\r\nConnection: close\r\n", argv[1]);

	// HERE we make the GET request to the web server
	write(fd, req_header, strlen(req_header));
	// cleaning up the memory (storing 0 on every byte)
	bzero(buffer, BUFFER_SIZE);
	
	// html_start: flag which indicates if the response headers were read
	// if html_start is 0 then we are still reading header data
	// otherwise we can start read the actual HTML data (content) received from web server
	int html_start = 0;
	FILE* file = fopen("index.html", "w");
	// it helps us to know when the header is done
	char* head;	

	// reading the chucks of data of BUFFER_SIZE
	while(read(fd, buffer, BUFFER_SIZE) != 0) {
		// After the headers are done there is a blank line which splits
		// the headers from the actual content (HTML CODE in our case)
		// We find this blank space by using strstr which position us to the
		// beggining of the searched string ("\r\n\r\n")
		if (html_start == 0 && (head = strstr(buffer, "\r\n\r\n")) != NULL) {
			// if the HTTP headers are done we set our flag to 1
			html_start = 1;
			// writing to file (head + 4 means head string beggining from it's 4th position)
			// to skip "\r\n\r\n"
			fprintf(file, "%s", head + 4);
			// writing to stdout (which will be redirect to IPv4 CLIENT by using dup2)
			printf("%s", head + 4);
			// clearing the memory
			bzero(buffer, BUFFER_SIZE);
			continue;
		}
		// if we skipped the headers part we can now write to our destinations as we want
		if (html_start == 1) {
			fprintf(file, "%s", buffer);
			printf("%s", buffer);
		}
		bzero(buffer, BUFFER_SIZE);
	}
	// shutting down the socket connection to the web server
	shutdown(fd, SHUT_RDWR); 
	close(fd); 

	return 0;
}
