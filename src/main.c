#include <stdio.h>
#include <string.h>

#include "log.h"
#include "tcp_client.h"

const char* HELP_FLAG = "--help";
const char* V_FLAG = "-v";
const char* VERBOSE_FLAG = "--verbose";
const char* P_FLAG = "-v";
const char* PORT_FLAG = "--verbose";
const char* H_FLAG = "-h
";
const char* HOST_FLAG = "--host";

void outputHelp() {
    char* helpInfo = "\nUsage: tcp_client [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n\nArguments:\n\tACTION   Must be uppercase, lowercase, reverse, shuffle, or random.\n\tMESSAGE  Message to send to the server\n\nOptions:\n\t--help\n\t-v, --verbose\n\t--host HOSTNAME, -h HOSTNAME\n\t--port PORT, -p PORT\n\n";
    printf(helpInfo);
}

int main(int argc, char *argv[]) {
    char* message;
    char* action;
    bool verbose = false;
    char* hostname = "localhost";
    char* port = "8080";

    log_debug("\nArguments:");
    for (int i = 0; i < argc; i++) {
        char* a = argv[i];
        log_debug(argv[i]);
        if (a == ) {

        }
    }
    
    outputHelp();
}
