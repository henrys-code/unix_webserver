#ifndef HTTPD_H
#define HTTPD_H

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <vector>
#include <algorithm>
#include "common.h"

#define RESP_OK         200
#define RESP_APPEND_OK  302
#define RESP_CERROR     400
#define RESP_FORBIDDEN  403
#define RESP_NOTFOUND   404
#define RESP_SERROR     500

#define SERV_VER        "HTTP/1.1"
#define SERV_NAME       "Custom/0.1"

using namespace std;

char * str_to_char(string str);
void PrintFile(string fname);
int MatchAddr(string serv_addr, string clnt_addr);
vector<struct kv_pairs> GetPermissions(string doc_root);
int CheckPermissions(vector<struct kv_pairs> perms, char * clnt_addr);
ssize_t RecvHttpMessage(int clnt_socket, char * buffer);
struct http_req ParseHttpMessage(char * buffer);
int CheckFile(string doc_root, string uri, char * clntName);
struct http_res BuildHttpResponse(int response_code, string fname);
void SendResponse(int clnt_socket, char * buffer, ssize_t len, char * fname);
void HandleTCPClient(int clntSocket, string doc_root, char * clntName);
void start_httpd(unsigned short port, string doc_root);

#endif // HTTPD_H
