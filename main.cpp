#include <string>
#include "command_line_parser.h"

int main(int ac, char** av){
    command_line_parser parser(ac, av, "Registry Server");

    parser.add_options<std::string>("config,c", "Set path to config(*.xml) file");
    parser.add_options<std::string>("logfile,l", "Set path to log file");
    parser.add_options<long long>("size,s", "Set size of log file");

}