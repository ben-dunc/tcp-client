#include "log.h"
#include "tcp_client.h"
#include <stdio.h>

#define DEFAULT_MSG_SIZE 1024

int msg_sent = 0;
int total_msg_sent = 0;

/*
Description:
    Handles the response from the receive function. It will output the message, decrement the
    amount of messages still to be received, and return true if all have been received.
Arguments:
    char* msg - the message coming in from the api
Return value:
    0 if all messages have been received, > 0 if not
*/
int handle_response(char *msg) {
    // log_debug("\tHANDLE MSG (%i/%i): %s", total_msg_sent - msg_sent + 1, total_msg_sent, msg);
    fprintf(stdout, "%s\n", msg);
    msg_sent--;
    return msg_sent;
}

int main(int argc, char *argv[]) {

    // char test[] = {0x00, 0x00, 0x00, 0x13};

    // uint32_t *num = (uint32_t *)&test;
    // fprintf(stderr, "\n\n%i\n\n", htonl(*num));

    // return 0;

    // parse arguments
    struct Config config;
    if (tcp_client_parse_arguments(argc, argv, &config) == EXIT_FAILURE) {
        fprintf(stderr, "\nError while parsing arguments. Run again with [-v, --verbose] to see "
                        "more info.\n\n");
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

    // open tcp connection
    int sockfd = tcp_client_connect(config);
    if (sockfd == -1) {
        fprintf(stderr,
                "\nError while connecting to tcp server: %s:%s. Did you enter the correct host &"
                " port "
                "? Run again with[-v,"
                "--verbose] to see more "
                "info.\n\n",
                config.host, config.port);
        return EXIT_FAILURE;
    }

    // read from file &send
    char *action = "uppercase";
    char *message = "this is my message";
    int read = 1;
    while ((read = tcp_client_get_line(fileptr, &action, &message)) > 0) {
        // use action and message to send
        int send_status = tcp_client_send_request(sockfd, action, message);
        msg_sent++;
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

    total_msg_sent = msg_sent;

    // receive
    if (EXIT_FAILURE == tcp_client_receive_response(sockfd, &handle_response)) {
        fprintf(stderr,
                "\nError while receiving responses (%i/%i msgs received). Run again with [-v, "
                "--verbose] to see more "
                "info.\n\n",
                total_msg_sent - msg_sent, total_msg_sent);
        return EXIT_FAILURE;
    }
    log_trace("asdf");

    if (tcp_client_close(sockfd) == EXIT_FAILURE) {
        fprintf(stderr,
                "\nError while closing tcp server connection: %s:%s. Run again with[-v,"
                "--verbose] to see more "
                "info.\n\n",
                config.host, config.port);
        return EXIT_FAILURE;
    }

    // close file
    if (tcp_client_close_file(fileptr) == EXIT_FAILURE) {
        fprintf(stderr,
                "\nError while closing file: %s. Run again with [-v, --verbose] to see more "
                "info.\n\n",
                config.file);
        return EXIT_FAILURE;
    }
}
