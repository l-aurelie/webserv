#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "webserv.hpp"

int main(void){

    int socketServer = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addrServer;
    addrServer.sin_addr.s_addr = inet_addr("127.0.0.1");
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(30000);

    bind(socketServer, (const struct sockaddr *)&addrServer, sizeof(addrServer));
    listen(socketServer, 5);


    return (EXIT_SUCCESS);
}