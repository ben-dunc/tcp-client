#include <stdio.h>
#include <string.h>

#include "log.h"
#include "tcp_client.h"

int main(int argc, char *argv[]) {
    // parse arguments
    Config config;
    if (EXIT_FAILURE == tcp_client_parse_arguments(argc, argv, &config))
        return EXIT_FAILURE;

    // connect
    int sockfd = tcp_client_connect(config);
    if (sockfd == -1) {
        log_error("An error occured in tcp_client_connect(). \
        Run again with '--verbose' for more information. Exiting program.");
        return EXIT_FAILURE;
    }

    // send request
    int status = tcp_client_send_request(sockfd, config);
    if (status == EXIT_FAILURE) {
        log_error("An error occured in tcp_client_send_request(). \
        Run again with '--verbose' for more information. Exiting program.");
        return EXIT_FAILURE;
    }

    char buf[TCP_CLIENT_MAX_INPUT_SIZE];

    // receive response
    if (EXIT_FAILURE == tcp_client_receive_response(sockfd, buf, TCP_CLIENT_MAX_INPUT_SIZE)) {
        log_error("An error occured in tcp_client_receive_response(). \
        Run again with '--verbose' for more information. Exiting program.");
        return EXIT_FAILURE;
    }

    // close
    if (tcp_client_close(sockfd)) {
        log_error("An error occured in tcp_client_close(). \
        Run again with '--verbose' for more information.");
    }

    fprintf(stdout, "\n%s\n", buf);
}
