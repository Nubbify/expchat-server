# expchat-server
A chat server made in C meant for exploring sockets and net code in C. Meant to work with expchat-client.

Server.c holds the core execution loop. Server handles multiple concurrent connections in a list of sockets, waiting for packets from those sockets or new connections. When receiving a new connection, the socket list is expanded to include the new connection. Users are identified by their socket number.

Packet.c holds the networking logic for creating, sending, and reading packets. 
Types of packets include
* Room List Request Response packet
* User List Request Response packet
* Message packet
* Personal Message packet
* Connection Acknowledgement packet
* Command Successful (Generic Acknowledgement) packet
* Command Failed (Generic Fail) packet

Utilities.c holds the data structures and logic that access and modify chat rooms, user lists, usernames, etc.

Errors.c holds functions that throw errors with a message and close the server. 
