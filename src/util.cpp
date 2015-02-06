#include <sstream>
#include "util.h"

int tcpip::readCommandSize(tcpip::Storage &inStorage) throw(std::invalid_argument)
{
	// Try to obtain size from first byte
	int size = inStorage.readUnsignedByte();

	// If it's ok, discount the byte
	if (size != 0) {
		return size-1;
	}

	// If it's zero, read from the following integer, discount both
	else {
		size = inStorage.readInt();
		return size-5;
	}
}

void tcpip::writeCommandSize(tcpip::Storage &outStorage, int size)
{
	// Counts the "size byte" in the size
	size++;

	// If size fits in a byte, write it
	if (size < 256) {
		outStorage.writeUnsignedByte(size);
	}

	// If doesn't fit in a byte, write zero and an int
	else {
		outStorage.writeUnsignedByte(0);
		outStorage.writeInt(size+4); // Counts the int
	}
}


ProtocolException::ProtocolException(std::string what, int port, bool isClient) throw () :
	myPort(port),
	myFromClient(isClient)
{
	std::ostringstream msg;
	msg << myWhat << "(on " << (myFromClient? "client" : "SUMO")
		<< " through port " << myPort;
	myWhat = msg.str();
}

ProtocolException::~ProtocolException() throw ()
{
}

const char * ProtocolException::what() const throw()
{
	return myWhat.c_str();
}

bool ProtocolException::isFromClient() const throw()
{
	return myFromClient;
}
