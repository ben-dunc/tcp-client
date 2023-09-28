#include "log.h"
#include "tcp_client.h"
#include <stdio.h>

#define DEFAULT_MSG_SIZE 1024

int main(int argc, char *argv[]) {

    // parse arguments
    struct Config config;
    if (tcp_client_parse_arguments(argc, argv, &config) == EXIT_FAILURE) {
        fprintf(stderr,
                "\nError while parsing arguments. Run again with [-v, --verbose] to see more "
                "info.\n\n");
        return EXIT_FAILURE;
    }

    // open file
    FILE *fileptr = tcp_client_open_file(config.file);
    if (fileptr == NULL) {
        fprintf(stderr,
                "\nError while opening file: %s. Does the file exist? Run again with [-v, "
                "--verbose] to see more "
                "info.\n\n",
                config.file);
        return EXIT_FAILURE;
    }

    // obtain file size
    fseek(fileptr, 0, SEEK_END);
    long lSize = ftell(fileptr);
    if (lSize == -1) {
        fprintf(stderr,
                "\nError while finding length of file: %s. Run again with [-v, "
                "--verbose] to see more "
                "info.\n\n",
                config.file);
    }
    rewind(fileptr);

    // open tcp connection
    // int sockfd = 1;
    int sockfd = tcp_client_connect(config);
    if (sockfd == -1) {
        fprintf(stderr,
                "\nError while connecting to tcp server: %s:%s. Did you enter the correct host & "
                "port ? Run again with[-v,"
                "--verbose] to see more "
                "info.\n\n",
                config.host, config.port);
        return EXIT_FAILURE;
    }

    // read from file & send
    char *action;
    char *message;
    int read = 1;
    while ((read = tcp_client_get_line(fileptr, &action, &message)) > 0) {
        // use action and message to send
        int send_status = tcp_client_send_request(sockfd, action, message);
        if (send_status == EXIT_FAILURE) {
            fprintf(stderr,
                    "\nError while sending [action: %s, msg: %s] to tcp server: %s:%s. Did you "
                    "enter the correct host "
                    "& port? "
                    "Run again with [-v, "
                    "--verbose] to see more "
                    "info.\n\n",
                    action, message, config.host, config.port);
            return EXIT_FAILURE;
        }

        // free action and message
        free(action);
        free(message);
    }

    // receive

    // close file
    if (tcp_client_close_file(fileptr) == EXIT_FAILURE) {
        fprintf(stderr,
                "\nError while closing file: %s. Run again with [-v, --verbose] to see more "
                "info.\n\n",
                config.file);
        return EXIT_FAILURE;
    }
}
