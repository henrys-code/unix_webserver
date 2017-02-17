#include <iostream>
#include "httpd.h"

using namespace std;

void usage(char * argv0)
{
    cerr << "Usage: " << argv0 << " listen_port docroot_dir" << endl;
}

void runtests() {

    int passed = 1;
    cerr << "\n********** STARTING TESTS **********" << endl;
    cerr << "testing ParseHttpMessage..." << endl;
    string str = "GET /home/what HTTP/1.1\r\nHost:no wai\r\nGuest:okiedokie\r\n\r\n";
    struct http_req req = ParseHttpMessage(str_to_char(str));
    if (req.method.compare("GET")) {
        cerr << "expected \"GET\" but was: " << req.method << endl;
        passed = 0;
    }
    if (req.uri.compare("/home/what")) {
        cerr << "expected \"/home/what\" but was: " << req.uri << endl;
        passed = 0;
    }
    if (req.http_version.compare("HTTP/1.1")) {
        cerr << "expected \"HTTP/1.1\" but was: " << req.http_version << endl;
        passed = 0;
    }
    if (req.kv[0].key.compare("Host")) {
        cerr << "expected \"Host\" but was: " << req.kv[0].key << endl;
        passed = 0;
    }
    if (req.kv[0].val.compare("no wai")) {
        cerr << "expected \"no wai\" but was: " << req.kv[0].val << endl;
        passed = 0;
    }
    if (passed) {
        cerr << "PASSED" << endl;
        passed = 1;
    } 
    else {
        cerr << "FAILED" << endl;
    }

    cerr << "testing MatchAddr..." << endl;
    string clnt_addr = "127.0.0.1";
    string serv_addr = "127.0.0.1";
    if (MatchAddr(serv_addr, clnt_addr) != 0) {
        cerr << "1" << endl;
        passed = 0;
    }
    serv_addr = "127.0.0.7";
    if (MatchAddr(serv_addr, clnt_addr) == 0) {
        cerr << "2" << endl;
        passed = 0;
    }
    serv_addr = "127.0.0.12/24";
    if (MatchAddr(serv_addr, clnt_addr) != 0) {
        cerr << "3" << endl;
        passed = 0;
    }
    serv_addr = "128.0.0.0/8";
    if (MatchAddr(serv_addr, clnt_addr)  == 0) {
        cerr << "4" << endl;
        passed = 0;
    }
    serv_addr = "127.0.0.0/16";
    if (MatchAddr(serv_addr, clnt_addr) != 0) {
        cerr << "5" << endl;
        passed = 0;
    }
    clnt_addr = "192.168.0.15";
    serv_addr = "0.0.0.0/0";
    if (MatchAddr(serv_addr, clnt_addr) != 0) {
        cerr << "6" << endl;
        passed = 0;
    }
    serv_addr = "192.168.0.15/32";
    if (MatchAddr(serv_addr, clnt_addr) != 0) {
        cerr << "7" << endl;
        passed = 0;
    }
    if (passed) {
        cerr << "PASSED" << endl;
        passed = 1;
    } 
    else {
        cerr << "FAILED" << endl;
    }

    cerr << "testing GetPermissions..." << endl;
    if (passed) {
        cerr << "PASSED" << endl;
        passed = 1;
    } 
    else {
        cerr << "FAILED" << endl;
    }
    
    cerr << "testing CheckPermissions..." << endl;
    if (passed) {
        cerr << "PASSED" << endl;
        passed = 1;
    } 
    else {
        cerr << "FAILED" << endl;
    }

    cerr << "testing ParseHttpMessage..." << endl;
    if (passed) {
        cerr << "PASSED" << endl;
        passed = 1;
    } 
    else {
        cerr << "FAILED" << endl;
    }

    cerr << "testing CheckFile..." << endl;
    if (passed) {
        cerr << "PASSED" << endl;
        passed = 1;
    } 
    else {
        cerr << "FAILED" << endl;
    }

    cerr << "testing BuildHttpResponse..." << endl;
    if (passed) {
        cerr << "PASSED" << endl;
        passed = 1;
    } 
    else {
        cerr << "FAILED" << endl;
    }

    cerr << "********** ALL TESTS FINISHED **********" << endl << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
            usage(argv[0]);
            return 1;
    }

    long int port = strtol(argv[1], NULL, 10);

    if (errno == EINVAL || errno == ERANGE) {
            usage(argv[0]);
            return 2;
    }

    if (port <= 0 || port > USHRT_MAX) {
            cerr << "Invalid port: " << port << endl;
            return 3;
    }

    string doc_root = argv[2];
    
    runtests();

    start_httpd(port, doc_root);

    return 0;
}

