/*
    main.c
    Created by Benjamin Duncan
    Sep 22, 2023
    For Computer Networks @ BYU
*/

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
        fprintf(stderr,
                "\nUnable to connect to %s:%s.\n"
                "Run again with '--verbose' for more information. Exiting program.\n",
                config.host, config.port);
        return EXIT_FAILURE;
    }

    // send request
    log_trace("sending request");
    int status = tcp_client_send_request(sockfd, config);
    if (status == EXIT_FAILURE) {
        fprintf(stderr, "\nUnable to send the request.\n"
                        "Run again with '--verbose' for more information. Exiting program.\n");
        return EXIT_FAILURE;
    }

    char buf[TCP_CLIENT_MAX_INPUT_SIZE];

    // receive response
    log_trace("receiving request");
    if (EXIT_FAILURE == tcp_client_receive_response(sockfd, buf, TCP_CLIENT_MAX_INPUT_SIZE)) {
        fprintf(stderr, "\nUnable to receive the response.\n"
                        "Run again with '--verbose' for more information. Exiting program.\n");
        return EXIT_FAILURE;
    }

    // close
    log_trace("closing connection");
    if (tcp_client_close(sockfd)) {
        fprintf(stderr, "Unable to close the connection.\n"
                        "Run again with '--verbose' for more information.");
    }

    fprintf(stdout, "\n%s\n", buf);
}
