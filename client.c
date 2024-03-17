#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

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
    fprintf(fp, "-s <size>, --size <size>\n");
    fprintf(fp, "                  The number of pseudo-random bytes to send to the server (default: %u)\n", DEFAULT_SIZE);
    fprintf(fp, "-c <congestion>, --congestion <congestion>\n");
    fprintf(fp, "                  The TCP congestion control algorithm to use (default: %s)\n", DEFAULT_CONGESTION_CONTROL);
    fprintf(fp, "-h, --help\n");
    fprintf(fp, "                  Display this help and exit\n");
}

int main(int argc, char *argv[])
{
    uint16_t port = DEFAULT_PORT;
    long bytes_to_send = DEFAULT_SIZE;
    char* congestion_control_algorithm = DEFAULT_CONGESTION_CONTROL;
    int opt, opt_index;
    while (optind < argc) {
        if ((opt = getopt_long(argc, argv, "p:n:c:h", cli_options, &opt_index)) != -1) {
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
                    print_help(stderr, argv[0]);
                    return EXIT_FAILURE;
            }
        } else {
            print_help(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }
    printf("%d, %ld, %s", port, bytes_to_send, congestion_control_algorithm);
}