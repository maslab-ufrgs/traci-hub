#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <getopt.h>

#include "TraCIHub.h"

#define STEP_LENGTH 7
#define SUMO_HOST 8

std::string argv0 = "tracihub";

std::string sumoHost = "localhost";
int sumoPort;

std::vector<int> clientPorts;

int stepLength = 1000;


void printUsage(std::ostream &out);
void parseOptions(int argc, char **argv);

int main(int argc, char **argv)
{
	if (argc >= 1) {
		argv0 = std::string(argv[0]);
	}
	parseOptions(argc, argv);

	TraCIHub hub(sumoHost, sumoPort, clientPorts, stepLength);
	return hub.execute();
}

void printUsage(std::ostream &out)
{
	out << "   Usage:\t" << argv0 << " [options] sumo_port"
		<< " client_port [client_port ...]" << std::endl;
	out << std::endl;
	out << "Options:" << std::endl;
	out << '\t' << std::setiosflags(std::ios::left) << std::setw(30) << "--sumo-host HOST"
		<< "The host where the SUMO is located. [default: localhost]" << std::endl;
	out << '\t' << std::setiosflags(std::ios::left) << std::setw(30) << "--step-length NUM"
		<< "The time (in ms) a timestep is supposed to represent. [default 1000]" 
		<< std::endl;
	out << '\t' << std::setiosflags(std::ios::left) << std::setw(30) << "--help -h"
		<< "Display this message." << std::endl;
}

void parseOptions(int argc, char **argv)
{

	/* Reads all options */
	int c;
	static struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"step-length", required_argument, NULL, STEP_LENGTH},
		{"sumo-host", required_argument, NULL, SUMO_HOST},
		{NULL, 0, NULL, 0}
	};

	int option_index = 0;
	while ((c = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
		switch (c) {
		case STEP_LENGTH:
			if (sscanf(optarg, "%d", &stepLength) < 1) {
				std::cerr << "Error parsing step length value \"" << optarg << '"' << std::endl;
				printUsage(std::cerr);
			}
			break;

		case SUMO_HOST:
			sumoHost = std::string(optarg);
			break;

		case 'h':
			printUsage(std::cout);
			exit(0);
			break;

		case '?':
			break;

		default:
			std::cerr << "Error parsing command line options (getopt returned"
				" character code 0" << std::oct << c << ")" << std::endl;
		}
	}

	/* Obtains the port of the SUMO server */
	if (optind < argc) {
		if (sscanf(argv[optind], "%d", &sumoPort) < 1) {
			std::cerr << "Cannot parse SUMO server port \""
					  << argv[optind] << '"' << std::endl;
			printUsage(std::cerr);
			exit(1);
		}
		optind++;
	}
	else {
		std::cerr << "Missing SUMO server port." << std::endl;
		printUsage(std::cerr);
		exit(1);
	}

	/* Obtains the ports of the SUMO clients */
	if (optind == argc) {
		std::cerr << "Missing port for at least one client." << std::endl;
		printUsage(std::cerr);
		exit(1);
	}

	while(optind < argc) {
		int port;
		if (sscanf(argv[optind], "%d", &port) < 1) {
			std::cerr << "Cannot parse client port \""
					  << argv[optind] << '"' << std::endl;
			printUsage(std::cerr);
			exit(1);
		}
		optind++;
		clientPorts.push_back(port);
	}
}
