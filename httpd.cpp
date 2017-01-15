#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "httpd.h"

using std::cerr;
using std::cout;
using std::string;
using std::endl;

#define QUEUE_SIZE (100)
#define BUFFER_SIZE (2000)

void start_httpd(unsigned short port, string doc_root) {
    cerr << "Starting server (port: " << port << ", doc_root: " << doc_root << ")" << endl;

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in target;
    bzero(&target, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = htonl(INADDR_ANY);
    target.sin_port = htons(port);

    int err = bind(sock, (struct sockaddr*) &target, sizeof(target));
    if (err < 0) {
        cerr << "bind() failed!" << endl;
        return;
    }

    err = listen(sock, QUEUE_SIZE);
    if (err < 0) {
        cerr << "listen() failed!" << endl;
        return;
    }
    
    while (true) {
        struct sockaddr_in client_addr;
        unsigned int client_len = sizeof(client_addr);
        char buffer[BUFFER_SIZE];
        
        int client_sock = accept(sock, (struct sockaddr*) &client_addr, &client_len);
        if (client_sock < 0) {
            cerr << "accept() failed" << endl;
            return;
        }
        
        cout << "accepted connection from " << inet_ntoa(client_addr.sin_addr) << ":" << client_addr.sin_port << endl;
        ssize_t received = 0;
        do {
            received = recv(client_sock, buffer, sizeof(buffer), 0);
            buffer[received] = '\0';
            cout << buffer;
        } while (received > 0);
        
        cout << "client finished sending" << endl;
    }
}
