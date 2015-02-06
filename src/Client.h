#ifndef CLIENT_H
#define CLIENT_H

#include "tcpip/storage.h"
#include "tcpip/socket.h"
#include "util.h"

/** \brief Handles the connection to a Client and message exchange.
 *
 * A Client has the following possible states:
 *  - Disconnected
 *  - Ready
 *  - Waiting (the client requested time steps, and should not act
 *             before the target time)
 *  - Disconnecting (a close command was received, but it wasn't yet
 *                   handled)
 *
 * To obtain the current state, use isConnected() and canAct(int)const:
 *   - isConnected() is only false when disconnected
 *   - canAct(int)const is only true when ready
 *
 * Other operations that may be done with a Client are waiting
 * for a connection and exchanging messages: acceptConnection(),
 * getCommands(tcpip::Storage&, int), putAnswers(tcpip::Storage&).
 *
 * Message handling filters the step and close commands, which are
 * handled internally by changing the Client's state.  Also, since
 * those may not be the last command in a message, all handling of
 * pending commands and answers is done internally.
 *
 * Every time a step was taken on the simulator, its result code
 * and description should be passed to handleStepResult(int, bool,
 * tcpip::Storage&)
 */
class Client {

 public:
	/// Prepares to listen for a Client on the given port
	Client(int port);

	virtual ~Client();

	/** \brief Waits for incoming connection
	 *
	 * \return true if received connection, false if already connected.
	 */
	bool acceptConnection() throw( tcpip::SocketException );

	int port();

	/// Determines if the client should act on the current time
	bool canAct(int currentTime) const throw();

	/// Determines if the client is connected
	bool isConnected() const;


	/** \brief Handles a step given by the simulator, and its result.
	 *
	 * In case of error, always use it as an answer to the client (even
	 * if it hasn't reached the target time and is still waiting).
	 *
	 * In case of success, ignores until the target time is reached. Then
	 * use the success as an answer to the client.
	 *
	 * If the answer to a step will be used, and there are no pending
	 * commands, the answer message will be sent to the client.
	 */
	void handleStepResult(int currentTime, bool success,
						  tcpip::Storage &resultMsg);


	/** \brief Obtains commands from the client.
	 *
	 * Forwards commands from the client up to the end of
	 * a message, a step request or a closing request.
	 *
	 * May listen for an incoming message when necessary.
	 *
	 * Handles storing of pending commands from a message
	 * (commands after a step request), and management of
	 * the state (connected, target time).
	 *
	 * \param[out] outStorage The Storage to receive commands
	 * \param[in] currentTime The current time in ms.
	 *
	 * \return true if commands were written to outStorage, false
	 *     otherwise (due to network errors or the internal state).
	 *
	 * \throw ProtocolException Signals an error parsing the commands
	 */
	bool getCommands(tcpip::Storage &outStorage, int currentTime);

	/** \brief Records answers to be sent to the client.
	 *
	 * Handles storing of pending answers (necessary if there
	 * are pending commands), and creates answers for step and
	 * close commands when required.
	 *
	 * Sends a message to the client when possible.
	 *
	 * \return true if the message was sent/stored, false if an
	 * error occured and the client is now disconnected.
	 */
	bool putAnswers(tcpip::Storage &answers);

	// Closes the connection with the client
	void closeConnection();

 private:
	/// Socket for communicating with the client process
	tcpip::Socket mySocket;

	/// Answers for a partially handled message
	tcpip::Storage myPendingAnswers;

	/// Unhandled commands from a message
	tcpip::Storage myPendingCommands;

	/// True if the last command was a close request
	bool myDisconnecting;

	bool myConnected;
	bool myWaiting;

	/** \brief Next time the client becomes active (meaningless
	 *	       with myWaiting==false) */
	int myTargetTime;


	bool hasPendingCommands() throw();
	bool hasPendingAnswers() const throw();

	/** \brief Handles the first command from inStorage.
	 *
	 * The commands are split into three cases:
	 *  - Simulation step: adjust target time and set waiting
	 *  - Close request: change state to disconnecting
	 *  - Other commands: copy to outStorage.
	 *
	 * \param[in] inStorage Storage to read a command
	 * \param[out] outStorage Storage to write common commands
	 *
	 * \return The command code for the handled command
	 *
	 * \throw ProtocolException Signals an error parsing the command
	 */
	unsigned char handleCommand(tcpip::Storage &inStorage,
								tcpip::Storage &outStorage);

	/** \brief Sends the pending answers to the client.
	 *
	 * Tries to send the storage and adjust the internal
	 * state in case of errors.
	 *
	 * Adds an answer to the close command when necessary.
	 *
	 * \return true if successful, false if an error occured.
	 */
	bool sendAnswers();

	// Writes a status answer to the given storage
	void writeStatusCmd(int cmdCode, int status, const std::string &description,
						tcpip::Storage &outStorage);

};

#endif /* CLIENT_H */
