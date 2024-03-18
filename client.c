#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 5000
#define DEFAULT_SIZE 1000
#define DEFAULT_CONGESTION_CONTROL "cubic"

static struct option cli_options[] = {
    { "port", required_argument, 0, 'p' },
    { "size", required_argument, 0, 'n' },
    { "congestion", required_argument, 0, 'c' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 }
};

void err_quit(const char *msg)
{
    if (errno != 0)
	perror(msg);
    else
	fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

void print_help(FILE *fp, const char *progname)
{
    size_t len = strlen(progname);
    char path[len + 1];
    strcpy(path, progname);
    fprintf(fp, "Usage: %s [options]\n\n", basename(path));

    fprintf(fp, "-p <port>, --port <port>\n");
    fprintf(fp, "                  The port to listen on (default: %u)\n", DEFAULT_PORT);
    fprintf(fp, "-n <size>, --size <size>\n");
    fprintf(fp, "                  The number of pseudo-random bytes to send to the server (default: %u)\n", DEFAULT_SIZE);
    fprintf(fp, "-c <congestion>, --congestion <congestion>\n");
    fprintf(fp, "                  The TCP congestion control algorithm to use (default: %s)\n", DEFAULT_CONGESTION_CONTROL);
    fprintf(fp, "-h, --help\n");
    fprintf(fp, "                  Display this help and exit\n");
}

struct sockaddr_in create_socket_address(uint16_t port, char* address){
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
	struct hostent *entry = gethostbyname(address);
	if (!entry) {
	    fputs("could not solve server address", stderr);
	    exit(EXIT_FAILURE);
	}
    memcpy(&addr.sin_addr, entry->h_addr_list[0], sizeof(struct in_addr));
    }
    return addr;
}

int create_connected_socket(struct sockaddr_in addr, char* congestion_control_algorithm, int len){
    // TODO: consider separating into two functions.
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // NOTE: mac os doesn't have this TCP_CONGESTION flag.
    int res = setsockopt(fd, IPPROTO_TCP, TCP_CONGESTION, congestion_control_algorithm, len);
    if(res==-1){
        perror("Failed setting the congestion control algorithm");
        exit(EXIT_FAILURE);
    }
    if (fd < 0) {
	    perror("failed to create socket");
	    exit(EXIT_FAILURE);
    }

    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	    perror("failed to connect to server");
	    close(fd);
	    exit(EXIT_FAILURE);
    }
    return fd;
}

char* create_random_bytes(int size){
    char* random_bytes = malloc(size * sizeof(char));
    if (random_bytes == NULL) {
        return NULL;
    }
    for(size_t i =0; i<size; ++i){
        random_bytes[i] = rand()%256;
    }
    return random_bytes;
}


ssize_t written(int fd, const char *buf, size_t len)
{
    ssize_t written;
    while (len > 0) {
        written = write(fd, buf, len);
        if (written <= 0) {
            if (written < 0 && errno == EINTR){
                continue;
            }
            else{
                return -1;
            }
        }
        len -= written;
        buf += written;
    }
    return len;
}


int main(int argc, char *argv[])
{
    uint16_t port = DEFAULT_PORT;
    long bytes_to_send = DEFAULT_SIZE;
    char* congestion_control_algorithm = DEFAULT_CONGESTION_CONTROL;
    int opt, opt_index;
    char * server_address = NULL;
    while ((opt = getopt_long(argc, argv, "p:n:c:h", cli_options, &opt_index)) != -1) {
        switch (opt) {
            case 'p':{
                long val = strtol(optarg, NULL, 10);
                if (errno || val <= 0 || val > UINT16_MAX)
                err_quit("Invalid port number provided");
                port = (uint16_t) val;
                break;
            }
            case 'n': {
                long size = strtol(optarg, NULL, 10);
                if (errno || size<=0){
                    err_quit("Invalid bytes size to send");
                }
                bytes_to_send = size;
                break;
            }
            case 'c': {
                congestion_control_algorithm = strdup(optarg);
                break;
            }
            case 'h':
                print_help(stdout, argv[0]);
                return EXIT_SUCCESS;
            default:
//                print_help(stderr, argv[0]);
                return EXIT_FAILURE;
        }
    }
    if(argv[optind] == NULL){
        print_help(stderr, argv[0]);
        return EXIT_FAILURE;
    }else{
        server_address = argv[optind];
    }

    struct sockaddr_in socket_address = create_socket_address(port, server_address);
    int tcp_fd = create_connected_socket(socket_address, congestion_control_algorithm, strlen(congestion_control_algorithm));
    char* random_bytes = create_random_bytes(bytes_to_send);
    if(random_bytes == NULL){
        perror("failed to connect to server");
        close(tcp_fd);
        free(congestion_control_algorithm);
        free(server_address);
        return EXIT_FAILURE;
    }
    written(tcp_fd, random_bytes, bytes_to_send);

//    printf("%d, %ld, %s, %s, %d", port, bytes_to_send, congestion_control_algorithm, server_address, tcp_fd);

    // clean up
    close(tcp_fd);
//    free(congestion_control_algorithm);
//    free(server_address);
    return EXIT_SUCCESS;
}