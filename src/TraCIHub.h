
#include "tcpip/socket.h"
#include "tcpip/storage.h"

#include "Client.h"

class TraCIHub {

 public:
  /**
   * \param sumoHost The host where the SUMO server will be located
   * \param sumoPort The port to which the SUMO server will be listening
   * \param clientPorts All ports to which we will listen for a single client
   * \param stepLength The time in ms each timestep represents
   */
	TraCIHub(std::string sumoHost, int sumoPort, const std::vector<int> &clientPorts, int stepLength=1000);

  /// Destructor
  virtual ~TraCIHub();

  /// Initialize the connections and execute the simulation
  int execute();

 protected:
  /** \brief Open the connection with SUMO.
   *
   * Tries to open connection with SUMO and,
   * in case of error, notifies the user.
   *
   * \return true iff connection was successful.
   */
  bool connectToSUMO();

  /// Close the connection with SUMO
  void disconnectSUMO();

  /** \brief Wait for incoming connections from all clients.
   *
   * Notifies the user of the connections and in
   * case of failure on some attempt.
   *
   * \return true iff all connections were successful.
   */
  bool acceptClients();


  /// Close the connections to all the clients.
  void closeClients();


  /// Requests a single step from SUMO
  void runStep();

  /** \brief Lets all clients run their steps, then request a step from SUMO.
   *
   * \return true if some client is still connected */
  bool handleStep();

  /** \brief Handles commands from client until a step or end request.
   *
   * Redirects all handled commands to SUMO, redirecting
   * answers back to the client.
   *
   * Whenever there's a step request, saves the unhandled
   * commands and freezes the until the requested time.
   *
   * Whenever there's an end request, terminates the client.
   */
  void handleClient(Client &client);


  /** \brief Verifies the integrity of the given status response
   *
   * Checks for correct size, matching commands and success or failure.
   *
   * \param answer The storage that contains the response as the first bytes
   * \param cmdCode The code of the expected command
   * \param[out] description String to receive the result description (especially
   *                           in case of error)
   *
   * \return true iff the result code indicated success
   * \throw ProtocolException Indicates an error in the message, making it invalid
   *                (not the communication of an error, an error in the communication)
   */
  bool verifyStatusResponse(tcpip::Storage &answer, int cmdCode, std::string &description) throw (ProtocolException);

 private:
  /// The socket for connecting to SUMO
  tcpip::Socket mySumoSocket;

  /// Information about all clients
  std::vector<Client> myClients;

  /// The message SUMO sent confirming the connection
  tcpip::Storage myConnectAnswer;

  /// The incremented time for each timestep
  int myTimestepLength;

  /// The current time
  int myCurrentTime;

};
