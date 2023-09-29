/*
    tcp_client.c
    Created by Benjamin Duncan
    Sep 22, 2023
    For Computer Networks @ BYU
*/

#include "tcp_client.h"
#include "log.h"
#include <ctype.h>
#include <string.h>

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

        static struct option long_options[] = {{HELP_FLAG, no_argument, 0, '-'},
                                               {VERBOSE_FLAG, no_argument, 0, 'v'},
                                               {PORT_FLAG, required_argument, 0, 'p'},
                                               {HOST_FLAG, required_argument, 0, 'h'},
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
                config->file = arg;
            } else {
                fprintf(stderr, "\nUnkown argument: '%s\n", arg);
                print_usage();
                return EXIT_FAILURE;
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
        log_info("No file provided, using stdin instead. Example usage: echo \"reverse HELLO\" | "
                 "bin/tcp_client --port 8081 -h lundrigan.byu.edu -");
        config->file = "-";
    }

    log_debug("[config] port: %s, host: %s, file: %s", config->port, config->host, config->file);

    log_trace("EXITING parsing");

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
    log_trace("\tENTERING connect");

    struct addrinfo hints, *res;
    int sockfd;
    int errorStatus = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    log_trace("[struct Config config] host: '%s', port: '%s', ", config.host, config.port);
    if (hints.ai_addr != NULL) {
        log_trace("[struct sockaddr hints.ai_addr] sa_family: %i, sa_data: %s",
                  (int)hints.ai_addr->sa_family, hints.ai_addr->sa_data);
    } else {
        log_trace("[struct sockaddr hints.ai_addr] NULL");
    }

    if ((errorStatus = getaddrinfo(config.host, config.port, &hints, &res)) != 0) {
        log_error("getaddrinfo returned error code '%i' which means: %s", errorStatus,
                  gai_strerror(errorStatus));
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    log_trace("sockfd: %i", sockfd);

    if (sockfd == -1)
        return sockfd;

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        log_info("connect() returned -1");
        return -1;
    }

    log_trace("calling freeaddrinfo");

    freeaddrinfo(res); // free the linked list

    log_trace("\tEXITING connect");

    return sockfd;
}

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
int tcp_client_send_request(int sockfd, char *action, char *message) {
    log_trace("\tENTERING send_request");

    log_trace("init messageToSend");
    char messageToSend[strlen(message) + 20];
    log_trace("assigning messageToSend");
    sprintf(messageToSend, "%s %d %s", action, (int)strlen(message), message);
    int bytes_sent = 0;
    log_trace("getting message length");
    int len = strlen(messageToSend);

    log_trace("entering while loop");
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

    log_trace("\tEXITING send_request");

    return EXIT_SUCCESS;
}

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
int tcp_client_receive_response(int sockfd, int (*handle_response)(char *)) {
    log_trace("\tENTERING receive_reponse");

    int bytes_recv = 0, status = 0, buf_len = 1024, parse_index = 0;
    int search_num = 0;
    bool callback_result = false;
    char *buf = malloc(sizeof(char) * buf_len);

    while (!callback_result) {
        log_trace(
            "\n[snapshot] search_num: %i, bytes_recv: %i, parse_index: %i, buf: '%s', buf from "
            "index: '%s'\n",
            search_num, bytes_recv, parse_index, buf, &buf[parse_index]);

        status = recv(sockfd, buf + bytes_recv, buf_len - bytes_recv, 0);

        if (status == 0) {
            fprintf(stderr, "\nThe server closed the connection before all messages received\n");
            return EXIT_FAILURE;
        } else if (status == -1) {
            fprintf(stderr, "\nAn error occured and send() returned -1\n");
            return EXIT_FAILURE;
        }

        bytes_recv += status;

        // need to update?
        if (bytes_recv >= buf_len - 100) {
            buf_len *= 2;
            buf = realloc(buf, buf_len * sizeof(char));
        }

        buf[bytes_recv] = 0;

        log_trace("buffer: %s", buf);

        while ((search_num == 0 && bytes_recv - parse_index > 0) ||
               (search_num > 0 && bytes_recv - parse_index >= search_num)) {
            // check for numbers, parse, and callback
            if (search_num == 0 && bytes_recv - parse_index > 0) {
                // check for numbers
                while (!isdigit(buf[parse_index]))
                    parse_index++;

                // get space ptr
                char *space_pos = strchr(&buf[parse_index], (int)(' ')) + sizeof(char);
                log_trace("buf from space_pos: '%s'", space_pos);

                // parse the number out as char*
                int num_len = (int)(space_pos - &buf[parse_index]) - 1;
                log_trace("num_len: %i", num_len);
                char *num = malloc(sizeof(char) * (num_len + 1));
                log_trace("Executing strncpy(dest, '%s', %i)", &buf[parse_index], num_len);
                strncpy(num, &buf[parse_index], num_len);
                num[num_len] = 0;

                log_trace("Parsed number: '%s'", num);
                log_trace("Parsed number as int: '%i'", atoi(num));

                // convert to integer
                search_num = atoi(num);
                parse_index += num_len + 1; // + 1 for space

                log_trace("\n[snapshot] search_num: %i, bytes_recv: %i,parse_index: % i ",
                          search_num, bytes_recv, parse_index);

                free(num);
            } else if (search_num > 0 && bytes_recv - parse_index >= search_num) {
                // parse out message!
                log_trace("parsing out msg");
                char *msg = malloc(sizeof(char) * search_num);
                msg[search_num] = 0;
                strncpy(msg, &buf[parse_index], search_num);
                log_debug("parsed msg: '%s'", msg);

                // call function
                callback_result = (handle_response)(msg) == 0;

                free(msg);

                // update values
                parse_index += search_num;
                search_num = 0;
            }
        }
    }

    free(buf);

    log_trace("\tEXITING receive_reponse");
    fprintf(stderr, "\tEXITING receive_reponse");
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
    log_trace("\tEXECUTING close (tcp)");
    return close(sockfd) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

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
FILE *tcp_client_open_file(char *file_name) {
    log_trace("\tEXECUTING open_file");
    if (strcmp(file_name, "-") == 0) {
        return stdin;
    } else {
        return fopen(file_name, "r");
    }
}

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
int tcp_client_get_line(FILE *fd, char **action, char **message) {
    log_trace("\tENTERING get_line");
    char *line = NULL;
    size_t len = 0; // does this do anything really?
    ssize_t read;
    bool is_valid_msg = false;

    while (!is_valid_msg) {
        read = getline(&line, &len, fd);
        log_trace("Read %i bytes from file", read);
        if (read == -1)
            return -1;
        else if (read <= 1) {
            log_debug("Read returned 0 or one, skipping line: '%s", line);
            continue;
        }

        log_trace("Replacing newline with null terminator");
        char *newline = strchr(line, (int)('\n'));
        if (newline != NULL)
            newline[0] = '\0';

        log_trace("Line (of length %zu): '%s'", read, line);

        // get space ptr
        char *space_pos = strchr(line, (int)(' '));
        if (space_pos == NULL) {
            log_debug("space_pos is NULL. No space found. Skipping line: %s", line);
            continue;
        }
        space_pos++;
        log_trace("Line from space_pos: '%s'", space_pos);

        // get action
        int action_len = (int)(space_pos - line) - 1;
        *action = malloc((sizeof(char)) * action_len + 1);
        log_trace("Executing strncpy(dest, '%s', %i)", line, action_len);
        strncpy(*action, line, action_len);
        (*action)[action_len] = 0;
        log_trace("Parsed action: '%s'", *action);

        if (!is_valid_action(*action)) {
            log_debug("Action not valid. Skipping line: %s", line);
            continue;
        }

        // get message
        size_t msg_len = (int)(read - action_len) + 1;
        *message = malloc(sizeof(char) * msg_len + 1);
        log_trace("Executing strncpy(dest, '%s', %i)", space_pos, msg_len);
        strncpy(*message, space_pos, msg_len);
        (*message)[msg_len] = 0;
        log_trace("Parsed msg: '%s'", *message);

        if (strlen(*message) == 0) {
            log_debug("No message. Skipping line: %s", line);
            continue;
        }

        is_valid_msg = true;
    }

    log_debug("Action: '%s', message: '%s'", *action, *message);
    log_trace("\tEXITING get_line");
    return read;
}

/*
Description:
    Closes a file.
Arguments:
    FILE *fd: The file pointer to close
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close_file(FILE *fd) {
    log_trace("\tEXECUTING close_file");
    return fclose(fd) == 0 ? 0 : 1;
}
