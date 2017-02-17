#include <iostream>
#include "client.h"

void start_httpd_client(uint16_t port, string doc_root) {

    int sock; 
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "client: socket() failed " << endl;
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;

    int rtnVal = inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr.s_addr);
    if (rtnVal == 0) {
        cerr << "inet_pton() failed: invalid address string" << endl;
    }
    if (rtnVal < 0) {
        cerr << "inet_pton() failed " << endl;
    }
    
    servAddr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        cerr << "connect() failed " << endl;
    }

    string get = "GET " + doc_root + " HTTP/1.1";
    char * req_str = new char[get.length() + 1];
    strcpy(req_str, get.c_str());

    //ssize_t numBytes = send(sock, req_str, strlen(req_str + 1), 0);
}
