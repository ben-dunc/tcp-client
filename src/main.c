#include <stdio.h>
#include <string.h>

#include "log.h"
#include "tcp_client.h"

/*
    const char* HELP_FLAG = "--help";
    const char* V_FLAG = "-v";
    const char* VERBOSE_FLAG = "--verbose";
    const char* P_FLAG = "-p";
    const char* PORT_FLAG = "--port";
    const char* H_FLAG = "-h";
    const char* HOST_FLAG = "--host";
*/

int main(int argc, char *argv[]) {
    // parse arguments
    Config config;
    tcp_client_parse_arguments(argc, argv, &config);

    // connect

    // send request

    // receive response

    // close
}
