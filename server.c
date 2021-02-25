#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

/* maxfd may be a bug*/
#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id; //customer id
    int adultMask;
    int childrenMask;
} Order;

int handle_read(request* reqP) {
    char buf[512];
    read(reqP->conn_fd, buf, sizeof(buf));
    memcpy(reqP->buf, buf, strlen(buf));
    return 0;
}

int isnum(char buf[512]){
    int index = 0;
    while(buf[index] != '\n' && buf[index] != 13){
        if(isdigit(buf[index]) == 0)
            return 0;
        index++;
    }
    return 1;
}

int main(int argc, char** argv) {
    //open orderRecord
    int order_fd;
    order_fd = open("preorderRecord", O_RDWR);

    //use lock
    int islock[21] = {0};
    struct flock lock;
    
    //order struct
    Order *order = (Order *)malloc(sizeof(Order));


    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    //fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);
    
    fd_set read_set;
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    FD_ZERO(&read_set);
    FD_SET(svr.listen_fd, &read_set);
    while (1) {
        // TODO: Add IO multiplexing
        fd_set read_set_copy;
        read_set_copy = read_set;
        
        switch (select(maxfd + 5, &read_set_copy, NULL, NULL, &timeout)){
            case -1:
                //fprintf(stderr, "select error\n");
                break;
            case 0:
                break;
            default:
                for(int i = 0; i < maxfd; ++i){
                    if(FD_ISSET(i, &read_set_copy)){
                        if(i == svr.listen_fd){
                             // Check new connection
                            clilen = sizeof(cliaddr);
                            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                            if (conn_fd < 0) {
                                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                                if (errno == ENFILE) {
                                    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                                    continue;
                                }
                                ERR_EXIT("accept");
                            } 
                            else{
                                requestP[conn_fd].conn_fd = conn_fd;
                                strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                                FD_SET(conn_fd, &read_set);
                                sprintf(buf, "Please enter the id (to check how many masks can be ordered):\n");
                                write(conn_fd, buf, strlen(buf));
                            }                  
                        }
                        else{
                            handle_read(&requestP[i]);
#ifdef READ_SERVER      
                            if(isnum(requestP[i].buf)){
                                requestP[i].id = atoi(requestP[i].buf);
                                if(requestP[i].id >= 902001 && requestP[i].id <= 902020){
                                    requestP[i].id %= 902000;
                                    lock.l_type = F_RDLCK;
                                    lock.l_whence = SEEK_SET;
                                    lock.l_start = sizeof(Order) * (requestP[i].id - 1);
                                    lock.l_len = sizeof(Order);
                                    if(islock[requestP[i].id] == 1 || fcntl(order_fd, F_SETLK, &lock) == -1){
                                        sprintf(buf, "Locked.\n");
                                        write(i, buf, strlen(buf));
                                    }
                                    else{
                                        islock[requestP[i].id] = 1;
                                        lseek(order_fd, sizeof(Order) * (requestP[i].id - 1), SEEK_SET);
                                        read(order_fd, order, sizeof(Order));
                                        sprintf(buf, "You can order %d adult mask(s) and %d children mask(s).\n", order->adultMask, order->childrenMask);
                                        write(i, buf, strlen(buf));
                                        islock[requestP[i].id] = 0;
                                        lock.l_type = F_UNLCK;
                                        lock.l_whence = SEEK_SET;
                                        lock.l_start = sizeof(Order) * (requestP[i].id - 1);
                                        lock.l_len = sizeof(Order);
                                        fcntl(order_fd, F_SETLK, &lock);
                                    }
                                }
                                else{
                                    sprintf(buf, "Operation failed.\n");
                                    write(i, buf, strlen(buf));
                                }
                            }
                            else{
                                sprintf(buf, "Operation failed.\n");
                                write(i, buf, strlen(buf));
                            }               
#else 
                            if(requestP[i].wait_for_write == 0){
                                if(isnum(requestP[i].buf)){
                                    requestP[i].id = atoi(requestP[i].buf);
                                    if(requestP[i].id >= 902001 && requestP[i].id <= 902020){
                                        requestP[i].id %= 902000;
                                        lock.l_type = F_WRLCK;
                                        lock.l_whence = SEEK_SET;
                                        lock.l_start = sizeof(Order) * (requestP[i].id - 1);
                                        lock.l_len = sizeof(Order);

                                        if(islock[requestP[i].id] == 1 || fcntl(order_fd, F_SETLK, &lock) == -1){
                                            sprintf(buf, "Locked.\n");
                                            write(i, buf, strlen(buf));
                                        }
                                        else{
                                            islock[requestP[i].id] = 1;
                                            lseek(order_fd, sizeof(Order) * (requestP[i].id - 1), SEEK_SET);
                                            read(order_fd, order, sizeof(Order));
                                            sprintf(buf, "You can order %d adult mask(s) and %d children mask(s).\nPlease enter the mask type (adult or children) and number of mask you would like to order:\n", order->adultMask, order->childrenMask);
                                            write(i, buf, strlen(buf));
                                            requestP[i].wait_for_write = 1;
                                            continue;
                                        }
                                    }
                                    else{
                                        sprintf(buf, "Operation failed.\n");
                                        write(i, buf, strlen(buf));
                                    }
                                }
                                else{
                                    sprintf(buf, "Operation failed.\n");
                                    write(i, buf, strlen(buf));
                                }
                            }
                            else{
                                if(strncmp(requestP[i].buf, "adult", 5) == 0){
                                    if(requestP[i].buf[5] != '\n' &&  requestP[i].buf[6] != '\n' && 
                                        requestP[i].buf[5] != 13 &&  requestP[i].buf[6] != 13 && isnum(requestP[i].buf + 6)){
                                        int order_num = atoi(requestP[i].buf + 6);
                                        lseek(order_fd, sizeof(Order) * (requestP[i].id - 1), SEEK_SET);
                                        read(order_fd, order, sizeof(Order));
                                        if(order->adultMask <  order_num || order_num == 0){
                                            sprintf(buf, "Operation failed.\n");
                                            write(i, buf, strlen(buf));
                                        }
                                        else{
                                            order->adultMask -= order_num;
                                            lseek(order_fd, sizeof(Order) * (requestP[i].id - 1), SEEK_SET);
                                            write(order_fd, order, sizeof(Order));
                                            sprintf(buf, "Pre-order for 9020%02d successed, %d adult mask(s) ordered.\n", requestP[i].id, order_num);
                                            write(i, buf, strlen(buf));
                                        }
                                    }
                                    else{
                                        sprintf(buf, "Operation failed.\n");
                                        write(i, buf, strlen(buf));
                                    }
                                }
                                else if(strncmp(requestP[i].buf, "children", 8) == 0){
                                    if(requestP[i].buf[8] != '\n' &&  requestP[i].buf[9] != '\n' && 
                                        requestP[i].buf[8] != 13 &&  requestP[i].buf[9] != 13 && isnum(requestP[i].buf + 9)){
                                        int order_num = atoi(requestP[i].buf + 9);
                                        lseek(order_fd, sizeof(Order) * (requestP[i].id - 1), SEEK_SET);
                                        read(order_fd, order, sizeof(Order));
                                        if(order->childrenMask < order_num || order_num == 0){
                                            sprintf(buf, "Operation failed.\n");
                                            write(i, buf, strlen(buf));
                                        }
                                        else{
                                            order->childrenMask -= order_num;
                                            lseek(order_fd, sizeof(Order) * (requestP[i].id - 1), SEEK_SET);
                                            write(order_fd, order, sizeof(Order));
                                            sprintf(buf, "Pre-order for 9020%02d successed, %d children mask(s) ordered.\n", requestP[i].id, order_num);
                                            write(i, buf, strlen(buf));
                                        }
                                    }
                                    else{
                                        sprintf(buf, "Operation failed.\n");
                                        write(i, buf, strlen(buf));
                                    }
                                }
                                else{
                                    sprintf(buf, "Operation failed.\n");
                                    write(i, buf, strlen(buf));
                                }
                                //unlock
                                islock[requestP[i].id] = 0;
                                lock.l_type = F_UNLCK;
                                lock.l_whence = SEEK_SET;
                                lock.l_start = sizeof(Order) * (requestP[i].id - 1);
                                lock.l_len = sizeof(Order);
                                fcntl(order_fd, F_SETLK, &lock);
                            }
#endif                      
                            //close
                            close(i);
                            FD_CLR(i, &read_set);
                            free_request(&requestP[i]);
                        }
                    }
                }
                break;
        }
    }
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
    reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}