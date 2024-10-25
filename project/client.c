#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc, char ** argv) {
    /* 1. Create socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // use IPv4  use UDP

    /* 2. Construct server address */
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET; // use IPv4
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    // Set sending port
    int SEND_PORT = atoi(argv[2]);
    serveraddr.sin_port = htons(SEND_PORT); // Big endian


    // 2.1 Set the client socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) < 0) {
        return errno;
    }

    // 2.2 Make STDIN nonblocking
    flags = fcntl(STDIN_FILENO, F_GETFL);
    flags |= O_NONBLOCK;
    if(fcntl(STDIN_FILENO, F_SETFL, flags) < 0) {
      return errno;
    }

   while(1){
      /* 3. Send data to server */
      int BUF_SIZE = 1024;
      char client_buf[BUF_SIZE];
      int bytes_read = read(STDIN_FILENO, client_buf, BUF_SIZE);
      if(bytes_read < 0){
         if(!(errno == EAGAIN || errno == EWOULDBLOCK)){
            perror("reading from stdin");
            return errno;
         }
      }
      if(bytes_read > 0){
         write(STDERR_FILENO, client_buf, bytes_read);
         int did_send = sendto(sockfd, client_buf, bytes_read, 
                        // socket  send data   how much to send
                           0, (struct sockaddr*) &serveraddr, 
                        // flags   where to send
                           sizeof(serveraddr));
         if (did_send < 0){
            perror("sending to server");
            return errno;
         }
      }
      
      /* 4. Create buffer to store incoming data */
      
      char server_buf[BUF_SIZE];
      socklen_t serversize = sizeof(socklen_t); // Temp buffer for recvfrom API

      /* 5. Listen for response from server */
      int bytes_recvd = recvfrom(sockfd, server_buf, BUF_SIZE, 
                              // socket  store data  how much
                                 0, (struct sockaddr*) &serveraddr, 
                                 &serversize);
      // Execution will stop here until `BUF_SIZE` is read or termination/error
      // Error if bytes_recvd < 0 :(
      if (bytes_recvd < 0){
         if(!(errno==EAGAIN || errno==EWOULDBLOCK)){
            perror("reading from server");
            return errno;
         }
      }
         // Print out data
      if (bytes_recvd > 0){
            write(1, server_buf, bytes_recvd);
      }
    }
    /* 6. You're done! Terminate the connection */     
    close(sockfd);
    return 0;
}
