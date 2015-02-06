#include "TraCIConstants.h"
#include "Client.h"

Client::Client(int port) :
	mySocket(port),
	myPendingAnswers(),
	myPendingCommands(),

	myDisconnecting(false),
	myConnected(false),
	myWaiting(false),
	myTargetTime(-1)
{
	// No further initialization needed
}

Client::~Client()
{
	// No destruction required
}


bool Client::acceptConnection() throw( tcpip::SocketException )
{
	// Connect if not already connected
	if (!myConnected) {
		mySocket.accept();
		return (myConnected = true);
	}

	// Warn if already connected
	else {
		return false;
	}
}


int Client::port()
{
	return mySocket.port();
}


bool Client::canAct(int currentTime) const throw()
{
	/* Must be connected, cannot be waiting and
	   must not have received a close request */
	return myConnected && !myWaiting && !myDisconnecting;
}

bool Client::isConnected() const
{
	return mySocket.has_client_connection();
}


void Client::handleStepResult(int currentTime, bool success, 
							  tcpip::Storage &resultMsg)
{
	// Don't act on premature success
	if (success && currentTime < myTargetTime) {
		return;
	}

	// Change state and send the response
	myWaiting = false;
	putAnswers(resultMsg);
}

bool Client::getCommands(tcpip::Storage &message, int currentTime)
{
	/* Don't act if disconnected, waiting for steps or
	   the client asked for disconnection */
	if (!canAct(currentTime) && !myDisconnecting && !hasPendingAnswers()) {
		return false;
	}

	// Obtain a new message, if there are no commands pending
	if (!hasPendingCommands()) {
		myPendingCommands.reset();

		try {
			mySocket.receiveExact(myPendingCommands);
		}
		catch (tcpip::SocketException) {
			return (myConnected = false);
		}
	}

	// Filter the commands
	unsigned char cmd;
	int processedCmds = 0;

	while(myPendingCommands.valid_pos()) {
		cmd = handleCommand(myPendingCommands, message);
		processedCmds++;

		if (cmd == CMD_SIMSTEP2 || cmd == CMD_CLOSE) {
			break;
		}
	}

	// If the only command was 'close', sends its answer
	if (cmd == CMD_CLOSE && processedCmds == 1) {
		return sendAnswers();
	}

	return true;
}


bool Client::putAnswers(tcpip::Storage &answers)
{
	// Don't act if disconnected
	if (!myConnected) {
		return false;
	}

	// Record the answers
	myPendingAnswers.writeStorage(answers);

	/* Send answers if we're not waiting for timesteps and either all commands
	   have been handled or the client asked for disconnection */
	if (!myWaiting && (!hasPendingCommands() || myDisconnecting)) {
		bool result = sendAnswers();
		return result;
	}

	return true;
}


bool Client::hasPendingCommands() throw()
{
	return myPendingCommands.valid_pos();
}

bool Client::hasPendingAnswers() const throw()
{
	return myPendingAnswers.size() > 0;
}


unsigned char Client::handleCommand(tcpip::Storage &inStorage,
									tcpip::Storage &outStorage)
{
	// Obtain and verify the command size
	int size;
	try {
		size = tcpip::readCommandSize(inStorage);
	}
	catch (std::invalid_argument) {
		throw ProtocolException("Message too short: cannot read the size"
								" of a command", port(), true);
	}

	// Obtain the command code
	unsigned char commandCode;
	try {
		commandCode = inStorage.readChar();
	}
	catch (std::invalid_argument) {
		throw ProtocolException("Message too short: cannot read the code"
								" of a command", port(), true);
	}


	switch (commandCode) {
	case CMD_SIMSTEP2:
		// On simulation step: adjust target time and set waiting
		int nextT;
		try {
			nextT = inStorage.readInt();
		}
		catch (std::invalid_argument) {
			throw ProtocolException("Message too short: cannot read the target"
									" time of a SIMSTEP2 command", port(), true);
		}

		myTargetTime = (nextT == 0)? -1 : nextT;
		myWaiting = true;

		break;

	case CMD_CLOSE:
		// On close: schedule disconnection
		myDisconnecting = true;
		break;

	default:
		// Any other command: write on outStorage
		tcpip::writeCommandSize(outStorage, size);
		outStorage.writeChar(commandCode);

		try {
			for (int i=0; i < size-1; i++) {
				outStorage.writeChar(inStorage.readChar());
			}
		}
		catch (std::invalid_argument) {
			throw ProtocolException("Message too short: couldn't read all bytes"
									" from the command", port(), true);
		}
	}

	return commandCode;
}

bool Client::sendAnswers()
{
	// Don't act if disconnected
	if (!myConnected) {
		return false;
	}

	// If disconnecting, write a close answer
	if (myDisconnecting) {
		writeStatusCmd(CMD_CLOSE, RTYPE_OK, "Goodbye", myPendingAnswers);
	}

	// Send the answers
	try {
		mySocket.sendExact(myPendingAnswers);
	}
	catch (tcpip::SocketException) {
		// Alert when disconnected
		return (myConnected = false);
	}

	// If disconnecting, close the connection
	if (myDisconnecting) {
		closeConnection();
	}

	myPendingAnswers.reset();
	return true;
}

void Client::writeStatusCmd(int cmdCode, int status,
							const std::string &description,
							tcpip::Storage &outStorage)
{
	outStorage.writeUnsignedByte(1 + 1 + 1 + 4 +
								 static_cast<int>(description.length()));
	outStorage.writeUnsignedByte(cmdCode);
	outStorage.writeUnsignedByte(status);
	outStorage.writeString(description);
}

void Client::closeConnection()
{
	if (myConnected) {
		mySocket.close();
		myConnected = false;
	}
}
