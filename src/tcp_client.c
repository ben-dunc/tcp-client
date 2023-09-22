#include "log.h"

#include "tcp_client.h"

#include <ctype.h>

#define HELP_FLAG "--help"
#define V_FLAG "-v"
#define VERBOSE_FLAG "--verbose"
#define P_FLAG "-p"
#define PORT_FLAG "--port"
#define H_FLAG "-h"
#define HOST_FLAG "--host"

#define ACTION_UPPERCASE "uppercase"
#define ACTION_LOWERCASE "lowercase"
#define ACTION_REVERSE "reverse"
#define ACTION_SHUFFLE "shuffle"
#define ACTION_RANDOM "random"

void print_usage() {
    char* helpInfo = "\nUsage: tcp_client [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n\nArguments:\n\tACTION   Must be uppercase, lowercase, reverse, shuffle, or random.\n\tMESSAGE  Message to send to the server\n\nOptions:\n\t--help\n\t-v, --verbose\n\t--host HOSTNAME, -h HOSTNAME\n\t--port PORT, -p PORT\n\n";
    fprintf(stderr, "%s", helpInfo);
}

bool is_valid_action(char * action) {
    return  strcmp(action, ACTION_UPPERCASE) == 0 || 
            strcmp(action, ACTION_LOWERCASE) == 0 || 
            strcmp(action, ACTION_REVERSE) == 0 || 
            strcmp(action, ACTION_SHUFFLE) == 0 || 
            strcmp(action, ACTION_RANDOM) == 0;
}

/*
Description:
    Parses the commandline arguments and options given to the program.
Arguments:
    int argc: the amount of arguments provided to the program (provided by the main function)
    char *argv[]: the array of arguments provided to the program (provided by the main function)
    Config *config: An empty Config struct that will be filled in by this function.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_parse_arguments(int argc, char *argv[], Config *config) {
    log_set_quiet(true);
    int c;
    config->action = NULL;
    config->message = NULL;
    config->port = NULL;
    config->host = NULL;

    while (1) {
        int option_index = 0;
        
        static struct option long_options[] = {
            { "help",     no_argument,       0,  '-' },
            { "verbose",  no_argument,       0,  'v' },
            { "port",     required_argument, 0,  'p' },
            { "host",     required_argument, 0,  'h' },
            { 0,          0,                 0,   0  }
        };

        c = getopt_long(argc, argv, "vp:h:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case '-': // help
                print_usage();
                log_debug("--help argument: %s", long_options[option_index].name);
                break;
            case 'v': // verbose
                log_set_quiet(false);
                log_debug("-v, --verbose argument: %s", long_options[option_index].name);
                break;
            case 'p': // port
                log_debug("-p, --port argument: %s, %s", long_options[option_index].name, optarg);

                if (!isdigit(optarg)) {
                    log_error("The port argument must be a number. Received argument: %s", optarg);
                    return EXIT_FAILURE;
                }

                config->port = optarg;
                break;
            case 'h': // host
                log_debug("-h, --host argument: %s, %s", long_options[option_index].name, optarg);
                config->host = optarg;
                break;
            case '?':
                // fprintf(stderr, "\nunkown argument: %s", argv[optind - 1]);
                print_usage();
                break;
            default:
                break;
        }
    }

    if (optind < argc) {
        while (optind < argc) {
            char * arg = argv[optind++];
            if (config->action == NULL) {
                if (is_valid_action(arg)) {
                    log_debug("Found valid action: %s", arg);
                    config->action = arg;
                } else {
                    log_warn("Invalid action: %s", arg);
                    return EXIT_FAILURE;
                }
            } else {
                if (config->message == NULL) {
                    log_debug("Found message: %s", arg);
                    config->message = arg;
                }
            }
        }
    }

    log_debug("[CONFIG] port: %s, host: %s, action: %s, message: %s", config->port, config->host, config->action, config->message);

    if (config->host == NULL) {
        config->host = TCP_CLIENT_DEFAULT_HOST;
        log_info("Using default host value: %s", TCP_CLIENT_DEFAULT_HOST);
    }

    if (config->port == NULL) {
        config->port = TCP_CLIENT_DEFAULT_PORT;
        log_info("Using default port value: %s", TCP_CLIENT_DEFAULT_PORT);
    }

    if (config->action == NULL || config->message == NULL) {
        log_error("An action and a message must be provided.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
/////////////////////// SOCKET RELATED FUNCTIONS //////////////////////
///////////////////////////////////////////////////////////////////////

/*
Description:
    Creates a TCP socket and connects it to the specified host and port.
Arguments:
    Config config: A config struct with the necessary information.
Return value:
    Returns the socket file descriptor or -1 if an error occurs.
*/
int tcp_client_connect(Config config) {
    log_debug("connecting");
    
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(config.host, config.port, &hints, &res) != 0) {
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    connect(sockfd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res); // free the linked list

    log_debug("connected");

    return sockfd;
}

/*
Description:
    Creates and sends request to server using the socket and configuration.
Arguments:
    int sockfd: Socket file descriptor
    Config config: A config struct with the necessary information.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_send_request(int sockfd, Config config) {
    log_debug("sending");
    
    int len = strlen(config.message);
    int bytes_sent = 0;

    while (len - bytes_sent > 0) {
        int status = send(sockfd, config.message + bytes_sent, len - bytes_sent, 0);
        log_trace("[sending] status: %i, bytes_recv: %i, message sent: '%s', length of message sent: %i", status, bytes_sent, config.message + bytes_sent, len - bytes_sent);

        // did it error
        if (status == -1) {
            log_error("An error occured and send returned -1");
            return EXIT_FAILURE;
        }

        bytes_sent += status;
    }

    log_debug("sent");
    
    return EXIT_SUCCESS;
}

/*
Description:
    Receives the response from the server. The caller must provide an already allocated buffer.
Arguments:
    int sockfd: Socket file descriptor
    char *buf: An already allocated buffer to receive the response in
    int buf_size: The size of the allocated buffer
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_receive_response(int sockfd, char *buf, int buf_size) {
    log_debug("receiving");

    int bytes_recv = 0, status = 0;

    while ((status = recv(sockfd, buf + bytes_recv, buf_size, 0)) != 0) {
        log_trace("[receiving] status: %i, bytes_recv: %i", status, bytes_recv);
        
        if (status == -1) {
            log_error("An error occured and send returned -1. Exiting program.");
            return EXIT_FAILURE;
        }

        bytes_recv += status;
    }

    buf[bytes_recv + 1] = 0;
    log_debug("received");
    return EXIT_SUCCESS;
}

/*
Description:
    Closes the given socket.
Arguments:
    int sockfd: Socket file descriptor
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close(int sockfd) {
    log_debug("close");
    return close(sockfd) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
} 