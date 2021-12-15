#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SERVER_PORT 22012

// Function to send a string from server to client
void send_string(int clientfd, char str[]) {
    /*
clientfd: file descriptor
str: string to be sent
    */
    int len = strlen(str);
    if (send(clientfd, str, len, 0) < 0) {
	 puts("Send error\n");
	 exit(1);
    }
}

// Function to receive a string from client to server
int receive_string(int clientfd, char str[]) {
    /*
clientfd: file descriptor
str: string to be received
    */
    int recv_code = recv(clientfd, str, 50, 0);
    if (recv_code < 0) {
	puts("Recv error\n");
	exit(1);
    }
    return recv_code;
}

// While a client is connected we will receive data from them
// if the received string is not 0 we treat the received command
// otherwise the client is going to be disconnected
void serve_client(int clientfd) {
	 printf("Client connected!\n");
         char cmd[1 << 5];
	 // it waits for any number of commands 
	 // (the client can send infinite number of cmds while is connected)
         while(1) {
             // receive data
             int recv_code = receive_string(clientfd, cmd);
             // If the client wants to disconnect, the recv_code will be 0
	     if (recv_code == 0) {
                 puts("Client disconnected!\n");
                 break;
             }

	     // the result string to be sent to IPv4 client
             // 1 << 5 = 2^5 => the maximum capacity of the result
	     char result[1 << 5];
	     
	     // compare the received command with the desired one "07#"
             if (strcmp(cmd, "07#") == 0) {
		 // if the command is implemented we will create a new child process with fork()
		 // if fork() is not 0 the followed code will be execute just by the child process
		 // if fork() is 0 the code is executed by the parent process
		 // we did use fork just for exec :)
                 if(fork() != 0) {
			 // dup2 redirects the stdout (1) and stderr (2) to our client file descriptor
                         // basically instead of writing in standard output, we are going
			 // to write in our file descriptor
			 dup2(clientfd, 1);
                         dup2(clientfd, 2);

			 // exec executes a file by its path
			 // execl does the same but we need to specify a list of arguments
			 // for the file we execute
			 // We write the file path two times because:
			 // first one: it's the path of the file we want to execute
			 // second one: is used for executing the actual file (it's argv[0])
                         // good practice: put NULL at the file so exec knows when the list finishes
			 // exec also overrides the code after its execution
			 execl("./client_IPv6", "./client_IPv6", "www.google.com", "80", (char*) NULL);
                         
			 // we want to close the entire process here
			 exit(1);
                 }
		 // parent process waits for it's child to be finished
		 wait(NULL);
		 // we send a dummy string back to the client
                 strcpy(result, "\n");
             }
             else {
		 // if the command is not implemented we want to send a specific message back to the client
                 strcpy(result, "Command not implemented!\n");
             }
             // send data
	     send_string(clientfd, result);
         }
	 // clean up
         close(clientfd);
}

int main() {
    // fd stands for file descriptor (unique identifier for a file / socket)
    // creating the socket
    // AF_INET -> IPv4 ip classes
    // SOCK_STREAM -> TCP protocol
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        perror("Socket error: ");
        return 1;
    }
    // initialising the server
    struct sockaddr_in server;

    // change the port accordingly
    server.sin_port = htons(SERVER_PORT);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    // binding (reserving the ip and port for our server)
    int retValue = bind(socketfd, (struct sockaddr*)&server, sizeof(server));
    if (retValue < 0) {
        perror("Bind error: ");
        close(socketfd);
        return 2;
    }

    struct sockaddr_in client;
    // clearing the client
    memset(&client, 0, sizeof(client));
    // waiting for clients to serve
    // second argument in listen (5) is the max number of clients in queue
    retValue = listen(socketfd, 5);
    if (retValue < 0) {
        perror("Listen error: ");
        close(socketfd);
        return 3;
    }
    socklen_t sock_len = sizeof(client);
    // server waits for client
    // infinite loop so it can receive any number of clients
    while (1) {
        // accepting a client (serving him)
        int clientfd = accept(socketfd, (struct sockaddr*)&client, &sock_len);
        if (clientfd < 0) {
            perror("Accept eror: ");
            close(socketfd);
            return 4;
        }
	serve_client(clientfd);
    }

    // cleaning up
    close(socketfd);
}
