#include "httpd.h"

#define SOCK_TIMEOUT    5
#define MAX_HEADER_SIZE 512
#define KV_SIZE         10
#define MAXPENDING      5
#define BUFSIZE         512

using namespace std;

typedef struct kv_pairs {
    string key;
    string val;
} kv_pairs;

typedef struct http_msg {
    char * msg;
} http_msg;

typedef struct http_req {
    int valid;
    string method;
    string uri;
    string http_version;
    struct kv_pairs kv[KV_SIZE];
    int num_kvs;
} http_req;

typedef struct http_res {
    string http_version;
    string response;
    string server;
    string last_modified;
    string content_type;
    int content_length;
    string fname;
} http_res;
