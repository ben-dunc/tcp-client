#include "log.h"
#include "tcp_client.h"
#include <stdio.h>

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

    // read from file & send
    char *action, *message = NULL;
    tcp_client_get_line(fileptr, &action, &message);

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
