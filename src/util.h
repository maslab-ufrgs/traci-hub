#ifndef UTIL_H
#define UTIL_H

#include "tcpip/storage.h"

namespace tcpip {

	/** \brief Reads the command size from inStorage.
	 *
	 * Assumes the command starts at the current position,
	 * and handles the two cases:
	 *  - size <= 255, written in a single byte
	 *  - size > 255, written in an integer following a byte with zero
	 *
	 * \return The command size, DISCOUTING the bytes used for the size itself.
	 */
	int readCommandSize(tcpip::Storage &inStorage) throw (std::invalid_argument);

	/** \brief Writes the command size on outStorage.
	 *
	 * Handles the two cases:
	 *  - size <= 255, written in a single byte
	 *  - size > 255, written in an integer following a byte with zero
	 *
	 * The bytes occupied by the size itself will be added to size
	 * internally, as required.
	 *
	 * \param[out] outStorage The storage where the size will be written
	 * \param size The size of the command, DISCOUNTING the bytes used
	 *             for the size itself.
	 */
	void writeCommandSize(tcpip::Storage &outStorage, int size);


}

class ProtocolException: public std::exception {
private:
	std::string myWhat;
	int myPort;
	bool myFromClient;

public:
	ProtocolException( std::string what, int port, bool FromClient=false ) throw ();
	~ProtocolException() throw();

	virtual const char * what() const throw();
	bool isFromClient() const throw();
};

#endif
