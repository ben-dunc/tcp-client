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

/*
Description:
    Prints the usage of this program
Arguments:
    none
Return value:
    void
*/
void print_usage() {
    char* helpInfo = "\nUsage: tcp_client [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n\n"
    "Arguments:\n\tACTION   Must be uppercase, lowercase, reverse, shuffle, or random.\n\tMESSAGE"
    "  Message to send to the server\n\nOptions:\n\t--help\n\t-v, --verbose\n\t--host HOSTNAME, "
    "  -h HOSTNAME\n\t--port PORT, -p PORT\n\n";
    fprintf(stderr, "%s", helpInfo);
}

/*
Description:
    Parses the commandline arguments and options given to the program.
Arguments:
    char * action: the character string representing the action to be considered. It must be
        either "uppercase", "lowercase", "reverse", "shuffle" or "random".
Return value:
    Returns true on success, false on failure.
*/
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

        log_trace("calling getopt_long");
        c = getopt_long(argc, argv, "vp:h:",
            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case '-': // help
                print_usage();
                log_debug("--help argument: \t%s",
                    long_options[option_index].name);
                exit(EXIT_SUCCESS);
                break;
            case 'v': // verbose
                log_set_quiet(false);
                log_debug("-v, --verbose argument: \t%s",
                    long_options[option_index].name);
                break;
            case 'p': // port
                log_debug("-p, --port argument: \t%s, %s", 
                    long_options[option_index].name, optarg);

                if (atoi(optarg) == 0) {
                    log_error("The port argument must be a number. Received argument: \t%s", 
                        optarg);
                    return EXIT_FAILURE;
                }

                config->port = optarg;
                break;
            case 'h': // host
                log_debug("-h, --host argument: \t%s, %s",
                    long_options[option_index].name, optarg);
                config->host = optarg;
                break;
            case '?':
                print_usage();
                break;
            default:
                break;
        }
    }

    log_trace("getting action and message");

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

    log_trace("setting default values if needed");

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

    log_trace("finished parsing arguments");

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





    struct addrinfo {
               int              ai_flags;
               int              ai_family;
               int              ai_socktype;
               int              ai_protocol;
               socklen_t        ai_addrlen;
               struct sockaddr *ai_addr;
               char            *ai_canonname;
               struct addrinfo *ai_next;
           };
*/
int tcp_client_connect(Config config) {
    log_trace("connecting");
    
    struct addrinfo hints, *res;
    int sockfd;
    int errorStatus = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    log_trace("[struct Config config] host: '%s', port: '%s', ", config.host, config.port);
    if (hints.ai_addr != NULL) {
        log_trace("[struct sockaddr hints.ai_addr] sa_family: %i, sa_data: %s", 
            (int) hints.ai_addr->sa_family, hints.ai_addr->sa_data);
    } else {
        log_trace("[struct sockaddr hints.ai_addr] NULL");
    }

    if ((errorStatus = getaddrinfo(config.host, config.port, &hints, &res)) != 0) {
        log_error("getaddrinfo returned error code '%i' which means: %s", 
            errorStatus, gai_strerror(errorStatus));
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    connect(sockfd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res); // free the linked list

    log_trace("connected");

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
    log_trace("sending");
    
    char messageToSend[1024];
    sprintf(messageToSend, "%s %d %s", config.action,
        (int) strlen(config.message), config.message);
    int bytes_sent = 0;
    int len = strlen(messageToSend);

    while (len - bytes_sent > 0) {
        int status = send(sockfd, messageToSend + bytes_sent, len - bytes_sent, 0);
        log_trace("[sending] status: %i, bytes_recv: %i, message sent: '%s', \
            length of message sent: %i", 
            status, bytes_sent, messageToSend + bytes_sent, len - bytes_sent);

        // did it error
        if (status == -1) {
            log_error("An error occured and send returned -1");
            return EXIT_FAILURE;
        }

        bytes_sent += status;
    }

    log_trace("sent");
    
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
    log_trace("receiving");

    int bytes_recv = 0, status = 0;

    while ((status = recv(sockfd, buf + bytes_recv, buf_size, 0)) != 0) {
        log_trace("[receiving] status: %i, bytes_recv: %i", status, bytes_recv);
        
        if (status == -1) {
            log_error("An error occured and send returned -1. Exiting program.");
            return EXIT_FAILURE;
        }

        bytes_recv += status;
    }

    buf[bytes_recv] = 0;
    log_trace("received");
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
    log_trace("close");
    return close(sockfd) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
} 