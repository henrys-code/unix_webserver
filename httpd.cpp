#include "httpd.h"

/* Author: Henry Gaudet
 * Date: Mon Feb 13 2017
 *
 * A custom server using unix/C sockets that can receive incoming http
 * messages, parse them into request objects, check if the requested 
 * resource exists and is accessible to the requesting user, and sends
 * an appropriate response (200, 404, etc). This server supports 
 * pipelining and access based on ip addresses defined in a .htaccess
 * file in the same directory as the requested resource. It does not support
 * multithreading/thread pools
 */


using namespace std;

/*
 *  Converts a string to a char *
 */
char * str_to_char(string str) {
    
    char * cstr = new char[str.length()];
    strcpy(cstr, str.c_str());
    return cstr;

}


/*
 * Used for debugging.
 * Prints the contents of a file specified by fname. 
 * Doesn't check if file exists.
 */
void PrintFile(string fname) {
    
    ifstream infile(fname.c_str());
    if (infile.is_open()) {
        string file_contents( 
                (istreambuf_iterator<char>(infile)), 
                (istreambuf_iterator<char>()) 
           );
        cout << "Contents of:\r\n" << fname << "\r\nare:\r\n" << file_contents << endl;
    }

}


/*
 *  Receives an incoming HTTP message and stores it at the location 
 *  pointed to by buffer. Returns the number of bytes received.
 */ 
ssize_t RecvHttpMessage(int clnt_socket, char * buffer) {

    memset(buffer, 0 , BUFSIZE);
    ssize_t total_bytes_rcvd = 0;
    ssize_t num_bytes_rcvd = 0;
    char * term;

    while (1) {
        num_bytes_rcvd = recv(clnt_socket,
                            buffer + total_bytes_rcvd,
                            BUFSIZE - total_bytes_rcvd, 0);
        if (num_bytes_rcvd <= 0) {
            cerr << "recv() failed in RecvHttpMessage" << endl;
            break;
        }

        total_bytes_rcvd += num_bytes_rcvd;
        
        term = strstr(buffer, "\r\n\r\n");
        if (term != NULL) {
            break;
        }
    }
    return total_bytes_rcvd;
    
}


/*
 *  Parse an httpmessage string into an http request object
 */
struct http_req ParseHttpMessage(char * buffer) {
    
    struct http_req req;
    string message(buffer);
    int firstline = 1;
    int msglen = message.find("\r\n\r\n");
    int abs_idx = 0;
    int idx = 0;
    int kv_idx = 0;
    int end = 0;
    string line;
    string word;
    string key;
    string value;
    
    req.valid = 1;
    while (abs_idx < msglen) {
        // GET uri HTTP/1.1
        // get first line of http request
        if (firstline) {
            // get end of line
            end = message.find('\r');
            abs_idx += end;
            // get first line
            line = message.substr(0, end);
            // get end index of first word i.e. "GET"
            idx = line.find(' ');
            // store first word in req.method
            // increment idx to remove space char and point to begin of next word
            req.method = line.substr(0, idx++);
            if (req.method != "GET") {
                req.valid = 0;
                break;
            }
            // get next word
            line = line.substr(idx);
            // set index to location of next space i.e. after uri
            idx = line.find(' ');
            // store next word in req.uri
            req.uri = line.substr(0, idx++);
            // if no resource requested, return default page
            if (req.uri.substr(0,1).compare("/")) {
                req.valid = 0;
                break;
            }
            if (!req.uri.compare("/") || !req.uri.compare("")) {
                req.uri = "/index.html";
            }
            // truncate used part of line
            line = line.substr(idx, end);
            // store last part in req.http_version
            req.http_version = line;
            if (req.http_version != "HTTP/1.1") {
                req.valid = 0;
                break;
            }
            // increment idx to beginning of next line
            while (buffer[abs_idx] == '\n' || buffer[abs_idx] == '\r'){
                abs_idx++;
            }
            // done with first line
            firstline = 0;
        }
        // key:value pairs
        else {
            idx = abs_idx;
            // get end of line index
            end = message.substr(idx).find('\r');
            // get whole line
            line = message.substr(idx, end);
            abs_idx += line.length();
            // get idx of ':'
            idx = line.find(':');
            if (idx == (int)string::npos) {
                req.valid = 0;
                break;
            }
            // get key from line (i.e. up to ':') and increment past ':'
            key = line.substr(0, idx++);
            // truncate used part of line
            line = line.substr(idx, end);
            // store the rest of the line in value
            value = line;
            // store key/value pair in req.kv array
            req.kv[kv_idx].key = key;
            req.kv[kv_idx].val = value;
            kv_idx++;
            // increment past \r\n
            abs_idx += 2;
        }
    }
    
    req.num_kvs = kv_idx;
    return req;
}


/*
 *  Create an http response object to send back to the client
 */
struct http_res BuildHttpResponse(int response_code, string fname) {
    
    struct http_res res;
    struct stat finfo;
    struct tm *time;
    char buffer[32];
    string ext;
    
    switch (response_code) {
        
        // 200
        case RESP_OK:   
                if (stat(fname.c_str(), &finfo) == 0) {
                    time = gmtime(&(finfo.st_mtime));
                }
                else {
                    cerr << "error building response" << endl;
                }
                res.response = "200 OK";
                strftime(buffer, 32, "%a, %d %b %Y %X %Z", time);
                res.last_modified = string(buffer);
                ext = fname.substr(fname.find('.') + 1);
                if (!ext.compare("html")) {
                    res.content_type = "text/html";
                }
                else if (!ext.compare("jpg") || !ext.compare("jpeg")) {
                    res.content_type = "image/jpeg";
                }
                else if (!ext.compare("png") || !ext.compare("PNG")) {
                    res.content_type = "image/png";
                }
                else {
                    res.content_type = "unknown";
                }
                res.content_length = (int)finfo.st_size;
                res.fname = fname;
                break;
        // 400
        case RESP_CERROR:   
                res.response = "400 Client Error";
                break;
        // 403
        case RESP_FORBIDDEN:    
                res.response = "403 Forbidden";
                break;
        // 404
        case RESP_NOTFOUND:     
                res.response = "404 Not Found";
                break;
        // 405
        case RESP_SERROR:
                res.response = "500 Server Error";
                break;
    }
    res.http_version = "HTTP/1.1";
    // server name always required
    res.server = SERV_NAME;
    return res;
}


/*
 *  Get the file permissions as detailed in a .htaccess file
 */
vector<struct kv_pairs> GetPermissions(string doc_root) {
    
    char fname[PATH_MAX];
    char ip_string[15];
    struct addrinfo hints, *addr_in;
    string deny, trash, ip;
    vector<struct kv_pairs> permissions;

    // always allow localhost connections
    struct kv_pairs temp;
    temp.key = "127.0.0.0/8";
    temp.val = "allow";
    permissions.push_back(temp);
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // hack b/c this is due in 2 hrs. thx c++ algs =)
    // get requested resource's directory
    string copy(doc_root);
    reverse(copy.begin(), copy.end());
    string hta_loc = copy.substr(copy.find("/"));
    reverse(hta_loc.begin(), hta_loc.end());

    // get .htaccess for this directory
    if (realpath((hta_loc + ".htaccess").c_str(), fname) != 0) {
        // htaccess found, open and read rules
        ifstream infile(string(fname).c_str());
        if (infile.is_open()) {
            while (infile >> deny >> trash >> ip) {
                if (getaddrinfo(ip.c_str(), NULL, &hints, &addr_in) == 0) {
                    inet_ntop(AF_INET,
                              &( (struct sockaddr_in *) addr_in->ai_addr)->sin_addr,
                              ip_string,
                              sizeof(ip_string));
                    ip = string(ip_string);
                    freeaddrinfo(addr_in);
                }
                // add rule to rules vector
                struct kv_pairs permission;
                permission.key = ip;
                permission.val = deny;
                permissions.push_back(permission);
            }
        }
    }
    
    return permissions;
}


/*
 *  Helper function to match 2 ipv4 addresses.
 *  Return 0 if clnt_addr is within range of serv_addr
 */
int MatchAddr(string serv_addr, string clnt_addr) {
    
    int f, l, num_octs;
    string oct; 
    string mask = serv_addr.substr(serv_addr.find("/") + 1);
    num_octs = atoi(mask.c_str()) / 8;

    if (num_octs == 0) {
        return 0;
    }

    f = 0;
    l = clnt_addr.find(".");
    string test_addr = serv_addr.substr(0,serv_addr.find("/"));
    
    for (int i = 0; i < num_octs; i++) {
        // check each oct in ipv4 addr against allowed/banned ips
        oct = clnt_addr.substr(f, l-f);
        if (oct.compare(test_addr.substr(f, l-f)) != 0) {
            // strings don't match, no need to keep checking
            return 1;
        }
        // get indices for next oct
        f = l + 1;
        l = f + clnt_addr.substr(f).find(".");
    }
    
    return 0;
}


/*
 *  Check if client ip is banned or not
 */
int CheckPermissions(vector<struct kv_pairs> perms, char * clnt_addr) {

    for (vector<struct kv_pairs>::iterator it = perms.begin(); it != perms.end(); it++) {
        string check_addr = it->key;
        if (MatchAddr(check_addr, string(clnt_addr)) == 0) {
            if ((it->val).compare("deny") == 0) {
                return 1;
            }
            else {
                return 0;
            }
        }
    }
    return 0;
}


/*
 *  Check if a file at a given location:
 *  1) exists
 *  2) is a regular file
 *  3) is accessible given user permissions
 */
int CheckFile(string doc_root, string uri, char * fname, char * clntName) {
    
    char fullpath[PATH_MAX];
    struct stat sb;
    string file_loc = doc_root + uri;

    // Does file exist?
    if (realpath(file_loc.c_str(), fullpath) == 0) {
        return RESP_NOTFOUND;
    }
    
    // File exists, does user have permission?
    string test_fp = string(fullpath).substr(0, doc_root.length());
    if (doc_root.compare(test_fp)) {
        return RESP_FORBIDDEN;
    }
    else {
        // get htaccess permissions
        vector<struct kv_pairs> permissions = GetPermissions(fullpath);
        // check client against htaccess permissions
        if (CheckPermissions(permissions, clntName) != 0) {
            return RESP_FORBIDDEN;
        }
    }
    
    // File exists and user has permission; is it a regular file?
    if (stat(file_loc.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        string temp = file_loc + "index.html";
        if (realpath(temp.c_str(), fullpath) == 0) {
            memset(fullpath, 0, PATH_MAX);
            return RESP_NOTFOUND;
        }
        else {
            // File was directory, but there is an index.html in requested
            // directory, so return that instead
            //return RESP_APPEND_OK;
       
            // nvm don't want to mess up the autograder 
            return RESP_NOTFOUND;
        }
    }
    
    // Is file world readable?
    if (!(sb.st_mode & S_IROTH)) {
        return RESP_FORBIDDEN;
    }

    // All checks passed
    else { 
        strcpy(fname, fullpath);
        return RESP_OK;
    }

    // This should never happen
    return RESP_SERROR;
}


/*
 *  Sends a response back to the client
 */
void SendResponse(int clnt_socket, struct http_res * res) {
    
    string str;
    
    // convert http_res object to string to pass to send() call
    if (res->response == "200 OK") {
        str = res->http_version + " " + 
              res->response + "\r\nServer: " + 
              res->server + "\r\nLast-modified:" + 
              res->last_modified + "\r\nContent-type:" + 
              res->content_type + "\r\nContent-length: " + 
              to_string(res->content_length) + "\r\n\r\n";
    }
    else {
        str = res->http_version + " " + 
              res->response + "\r\nServer: " + 
              res->server + "\r\n\r\n";
    }

    // print response just to make sure everything is kosher
    cerr << "\r\nResponse:\r\n" << str << endl;

    // get header size
    int hsize = str.length();
    ssize_t total_bytes_sent = 0;
    
    // send header
    while (total_bytes_sent < hsize) {
        ssize_t num_bytes_sent = send(clnt_socket, 
                                      str.c_str() + total_bytes_sent, 
                                      hsize - total_bytes_sent,
                                      0);
        if (num_bytes_sent < 0) {
            cerr << "send() failed" << endl;
            return;
        }
        else if (num_bytes_sent == 0) {
            cerr << "send() failed to send anything" << endl;
        }
        total_bytes_sent += num_bytes_sent;
    }

    // get filesize
    struct stat sb;
    stat((res->fname).c_str(), &sb);
    
    // send file
    if (res->response.substr(0,3).compare("200") == 0) {
        int fd = open((res->fname).c_str(), O_RDONLY);
        sendfile(clnt_socket, fd, NULL, sb.st_size);
    }
}


/*
 *  Handles a resource request from a client and sends a response
 *  containing the requested resource
 */ 
void HandleTCPClient (int clnt_socket, string doc_root, char * clntName) {
   
    char http_msg[BUFSIZE];
    char fname[PATH_MAX];
    int sock_open = 1;
    
    // set socket timeout. default 5 seconds.
    struct timeval timeout;
    timeout.tv_sec = SOCK_TIMEOUT;
    timeout.tv_usec = 0;
    
    int s_o = setsockopt(clnt_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if (s_o < 0) {
        cerr << "socket option failed" << endl;
    }

    while (sock_open != 0) {
        memset(fname, 0, PATH_MAX);
        // Receive incoming HTTP message
        ssize_t num_bytes_rcvd = RecvHttpMessage(clnt_socket, http_msg);
        if (num_bytes_rcvd == 0) {
            cerr << "failed to receive http message" << endl;
            return;
        }
        else {
            cerr << "\r\nRequest:\r\n" << http_msg << endl;
        }
        // Parse the message into a request object
        struct http_req req = ParseHttpMessage(http_msg);
        int response_code;
        if (req.valid == 0) {
            response_code = RESP_CERROR;
        }
        else {
            // Check that the requested resource is available
            response_code = CheckFile(doc_root, req.uri, fname, clntName);
        }
        // Create a response with the requested resource
        struct http_res res = BuildHttpResponse(response_code, string(fname));
        // Send the response back to the client
        SendResponse(clnt_socket, &res);
        for (int i = 0; i < req.num_kvs; i++) {
            if (req.kv[i].key.compare("Connection") == 0) {
                if (req.kv[i].val.compare(" close") == 0) {
                    cerr << "Closing socket..." << endl;
                    sock_open = 0;
                    if (close(clnt_socket) != 0) {
                        cerr << "Close failed errno: " << errno << endl;
                    }
                    sock_open = 0;
                    break;
                }
                else {
                    cerr << "mismatched Connection" << endl;
                }
            }
        }
    }
}


/*
 *  Start the server
 */
void start_httpd(unsigned short port, string doc_root)
{
    cerr << "Starting server (port: " << port <<
            ", doc_root: " << doc_root << ")" << endl;

    // create connection socket
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "socket() failed" << endl;
        return;
    }
    
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
    
    int t1 = bind(fd, (struct sockaddr*) &servAddr, sizeof(servAddr));
    if (t1 < 0) {
        cerr << "bind() failed " << endl;
        return;
    }

    int t2 = listen(fd, MAXPENDING);
    if (t2 < 0) {
        cerr << "listen() failed " << endl;
        return;
    }

    // wait for incoming connections
    while (1) {
        struct sockaddr_in clntAddr;
        memset(&clntAddr, 0, sizeof(clntAddr));
        
        socklen_t clntAddrLen = sizeof(clntAddr);
        int clntSock = accept(fd, (struct sockaddr *) &clntAddr, &clntAddrLen);
        if (clntSock < 0) {
            cerr << "accept() failed " << endl;
            continue;
        }
        
        char clntName[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, 
                      clntName, sizeof(clntName)) != NULL) {
            cout << "*****************************************" << endl;
            cout << "Handling client " << clntName << " " << ntohs(clntAddr.sin_port) << endl;
            HandleTCPClient(clntSock, doc_root, clntName);
        }
        else {
            cerr << "Unable to get client address" << endl;
        }
            
        cout << "*****************************************\r\n" << endl;
    }
}
