/*
    tcp_client.c
    Created by Benjamin Duncan
    Sep 22, 2023
    For Computer Networks @ BYU
*/

#include "tcp_client.h"
#include "log.h"

#define HELP_FLAG "help"
#define VERBOSE_FLAG "verbose"
#define PORT_FLAG "port"
#define HOST_FLAG "host"

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
    char *helpInfo = "Usage: tcp_client [--help] [-v] [-h HOST] [-p PORT] FILE\n"
                     "Arguments:\n"
                     "\tFILE\tA file name containing actions and messages to\n"
                     "\t\tsend to the server. If \"-\"\n"
                     " is provided, stdin will\n"
                     "\t\tbe read.\n"
                     "Options:\n"
                     "\t--help\n"
                     "\t-v, --verbose\n"
                     "\t--host HOSTNAME, -h HOSTNAME\n"
                     "\t--port PORT, -p PORT\n";

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
bool is_valid_action(char *action) {
    return strcmp(action, ACTION_UPPERCASE) == 0 || strcmp(action, ACTION_LOWERCASE) == 0 ||
           strcmp(action, ACTION_REVERSE) == 0 || strcmp(action, ACTION_SHUFFLE) == 0 ||
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
    config->file = NULL;
    config->port = NULL;
    config->host = NULL;

    while (1) {
        int option_index = 0;

        static struct option long_options[] = {{"help", no_argument, 0, '-'},
                                               {"verbose", no_argument, 0, 'v'},
                                               {"port", required_argument, 0, 'p'},
                                               {"host", required_argument, 0, 'h'},
                                               {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "vp:h:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case '-': // help
            print_usage();
            log_debug("--help argument: \t%s", long_options[option_index].name);
            exit(EXIT_SUCCESS);
            break;
        case 'v': // verbose
            log_set_quiet(false);
            log_debug("-v, --verbose argument: \t%s", long_options[option_index].name);
            break;
        case 'p': // port
            log_debug("-p, --port argument: \t%s, %s", long_options[option_index].name, optarg);

            if (atoi(optarg) == 0) {
                fprintf(stderr, "\nThe port argument must be a number. Received argument: \t%s",
                        optarg);
                print_usage();
                return EXIT_FAILURE;
            }

            config->port = optarg;
            break;
        case 'h': // host
            log_debug("-h, --host argument: \t%s, %s", long_options[option_index].name, optarg);
            config->host = optarg;
            break;
        case '?':
            print_usage();
            break;
        default:
            break;
        }
    }

    log_trace("getting filename");

    if (optind < argc) {
        while (optind < argc) {
            char *arg = argv[optind++];
            if (config->file == NULL) {
                if (is_valid_action(arg)) {
                    log_debug("Found valid action: %s", arg);
                    config->file = arg;
                } else {
                    fprintf(stderr, "\nInvalid action: %s\n", arg);
                    print_usage();
                    return EXIT_FAILURE;
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

    if (config->file == NULL) {
        fprintf(stderr, "\nA file must be profided.\n");
        print_usage();
        return EXIT_FAILURE;
    }

    log_debug("[config] port: %s, host: %s, file: %s", config->port, config->host, config->file);

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
*/
int tcp_client_connect(Config config);

/*
Description:
    Creates and sends request to server using the socket and configuration.
Arguments:
    int sockfd: Socket file descriptor
    char *action: The action that will be sent
    char *message: The message that will be sent
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_send_request(int sockfd, char *action, char *message);

/*
Description:
    Receives the response from the server. The caller must provide a function pointer that handles
the response and returns a true value if all responses have been handled, otherwise it returns a
    false value. After the response is handled by the handle_response function pointer, the response
    data can be safely deleted. The string passed to the function pointer must be null terminated.
Arguments:
    int sockfd: Socket file descriptor
    int (*handle_response)(char *): A callback function that handles a response
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_receive_response(int sockfd, int (*handle_response)(char *));

/*
Description:
    Closes the given socket.
Arguments:
    int sockfd: Socket file descriptor
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close(int sockfd);

///////////////////////////////////////////////////////////////////////
//////////////////////// FILE RELATED FUNCTIONS ///////////////////////
///////////////////////////////////////////////////////////////////////

/*
Description:
    Opens a file.
Arguments:
    char *file_name: The name of the file to open
Return value:
    Returns NULL on failure, a FILE pointer on success
*/
FILE *tcp_client_open_file(char *file_name);

/*
Description:
    Gets the next line of a file, filling in action and message. This function should be similar
    design to getline() (https://linux.die.net/man/3/getline). *action and message must be allocated
    by the function and freed by the caller.* When this function is called, action must point to the
    action string and the message must point to the message string.
Arguments:
    FILE *fd: The file pointer to read from
    char **action: A pointer to the action that was read in
    char **message: A pointer to the message that was read in
Return value:
    Returns -1 on failure, the number of characters read on success
*/
int tcp_client_get_line(FILE *fd, char **action, char **message);

/*
Description:
    Closes a file.
Arguments:
    FILE *fd: The file pointer to close
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close_file(FILE *fd);
