#include <iostream>
#include <sstream>

#include "TraCIConstants.h"
#include "util.h"

#include "TraCIHub.h"

TraCIHub::TraCIHub(std::string sumoHost, int sumoPort,
				   const std::vector<int> &clientPorts, int stepLength) :
	mySumoSocket(sumoHost, sumoPort),
	myClients(),
	myTimestepLength(stepLength),
	myCurrentTime(0)
{
	// Initialize all clients according to their ports
	std::vector<int>::const_iterator it;
	for (it=clientPorts.begin(); it != clientPorts.end(); it++) {
		myClients.push_back(Client(*it));
	}
}

TraCIHub::~TraCIHub()
{

}

int TraCIHub::execute()
{
	int result = 0;

	// Open connections
	if (!connectToSUMO()) {
		return 1;
	}

	if (!acceptClients()) {
		disconnectSUMO();
		return 2;
	}

	// Run all steps required
	try {

		bool active = true;
		while (active) {
			active = handleStep();
		}
	}
	catch (tcpip::SocketException e) {
		// Notify errors on the connection
		std::cout << "Error communicating to SUMO: " << e.what() << std::endl;
		result = 1;
	}
	catch (ProtocolException e) {
		// Notify errors from the protocol
		std::cout << "Error : " << e.what() << std::endl;
		result = e.isFromClient()? 2 : 1;
	}


	// Clean up
	disconnectSUMO();
	if (result == 0) {
		std::cout << "Finished simulation and disconnected from SUMO" << std::endl;
	} else {
		closeClients();
	}

	return result;
}

bool TraCIHub::connectToSUMO()
{
	try {
		// Connect through the socket
		mySumoSocket.connect();
	}
	catch (tcpip::SocketException e) {
		// Notify errors on the connection
		std::cout << "Error: Couldn't connect to SUMO" << std::endl;
		return false;
	}

	// Notify success
	std::cout << "Connected to SUMO on port " << mySumoSocket.port() << std::endl;
	return true;
}

void TraCIHub::disconnectSUMO()
{
	// Do nothing if already disconnected
	if (!mySumoSocket.has_client_connection()) {
		return;
	}

	tcpip::Storage closeCmd;

	// Compose the message
	closeCmd.writeUnsignedByte(1 + 1);
	closeCmd.writeUnsignedByte(CMD_CLOSE);

	// Communicate with SUMO
	mySumoSocket.sendExact(closeCmd);

	mySumoSocket.close();
}


bool TraCIHub::acceptClients()
{
	std::vector<Client>::iterator it;

	try {
		// Accepts connection in all clients
		for (it=myClients.begin(); it != myClients.end(); it++) {
			std::cout << "Waiting for connection on port " 
					  << it->port() << std::endl;
			it->acceptConnection();
		}
	}
	catch (tcpip::SocketException) {
		// Notify any failure
		std::cerr << "Error with client connection on port " 
				  << it->port() << std::endl;
		return false;
	}

	// Notify complete success
	std::cout << "All clients finished connecting" << std::endl << std::endl;
	return true;
}

void TraCIHub::closeClients()
{
	std::vector<Client>::iterator it;

	// Close the connection of all clients
	for (it=myClients.begin(); it != myClients.end(); it++) {
		it->closeConnection();
	}
}

void TraCIHub::runStep()
{
	tcpip::Storage message, answer;

	/* Compose and send the message */
	message.writeByte(1+1+4);
	message.writeChar(CMD_SIMSTEP2);
	message.writeInt(0);

	/* Execute the timestep */
	mySumoSocket.sendExact(message);
	mySumoSocket.receiveExact(answer);
	myCurrentTime += myTimestepLength;

	/* Obtain and verify the result */
	tcpip::Storage modAnswer;
	modAnswer.writeStorage(answer);

	std::string description;
	bool success = verifyStatusResponse(modAnswer, CMD_SIMSTEP2, description);

	if (!success) {
		std::cout << "Error on simulation step: " << description << std::endl;
	}

	/* Notify the clients of the result */
	std::vector<Client>::iterator it;
	for (it=myClients.begin(); it != myClients.end(); it++) {
		it->handleStepResult(myCurrentTime, success, answer);
	}
}

bool TraCIHub::handleStep()
{
	bool someConnected = false;

	std::vector<Client>::iterator it;
	for (it=myClients.begin(); it != myClients.end(); it++) {

		// Handles connected clients
		if (it->isConnected()) {
			handleClient(*it);
			someConnected = someConnected || it->isConnected();
		}
	}

	// After all clients were handled, runs a simulation step
	runStep();

	return someConnected;

}


void TraCIHub::handleClient(Client &client)
{
	tcpip::Storage message, answer;

	/* Exchange messages until the client cannot act
	   (either asked for a timestep or termination) */
	while (client.canAct(myCurrentTime)) {

		// Forward commands to SUMO
		message.reset();
		client.getCommands(message, myCurrentTime);

		if (message.size() > 0) {
			mySumoSocket.sendExact(message);

			// Forward answers to Client
			answer.reset();
			mySumoSocket.receiveExact(answer);
			client.putAnswers(answer);
		}
	}
}


bool TraCIHub::verifyStatusResponse(tcpip::Storage &answer, int cmdCode,
									std::string &description)
	throw (ProtocolException)
{
	// Verify the command size
	int size;
	try {
		size = tcpip::readCommandSize(answer);
	}
	catch (std::invalid_argument) {
	}

	if (size < 6) {
		std::ostringstream err;
		err << "Invalid status response for command " << cmdCode
			<< ": " << size << " bytes is too short.";
		throw ProtocolException(err.str(), mySumoSocket.port());
	}

	// Verify the command code
	int actualCmdCode;
	try {
		actualCmdCode = answer.readChar();
	}
	catch (std::invalid_argument) {
		throw ProtocolException("Message too short: couldn't read command"
								" code", mySumoSocket.port());
	}

	if (actualCmdCode != cmdCode) {
		std::ostringstream err;
		err << "Received status response for command " << actualCmdCode
			<< " when expecting " << cmdCode;
		throw ProtocolException(err.str(), mySumoSocket.port());
	}

	// Obtain the result code and description
	bool success;
	try {
		success = answer.readUnsignedByte() == RTYPE_OK;
	}
	catch (std::invalid_argument) {
		throw ProtocolException("Message too short: couldn't read result"
								" code", mySumoSocket.port());
	}

	try {
		description = answer.readString();
	}
	catch (std::invalid_argument) {
		throw ProtocolException("Message too short: couldn't read result"
								" description", mySumoSocket.port());
	}

	return success;
}
