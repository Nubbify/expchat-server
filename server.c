#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <stdint.h>
#include "utilities.h"
#include "errors.h"
#include "packet.h"

int main (int argc, char* argv[]) {
  fd_set master_fds; // List of all file descriptors
  fd_set curr_fds; // Copy of above for use in select
  int fdmax; // Maximum file descriptor number
  int serv_sock; //Server's primary listening socket
  int new_fd; //Latest socket descriptor
  struct sockaddr_in serv_addr; // Server address
  struct sockaddr_storage remote_addr; // Client address
  socklen_t addrlen; // Client address length
  char buffer[100000]; //Buffer for creating packets and client data
  uint32_t nbytes = 0; // To keep track of bytes sent
  uint32_t totalbytes = 0; // For send and recv loops
  int port; // Port assigned to the server by arguments
  int i; //For loops  and such
  struct packet client_packet; //To hold the current request from the server
  struct packet server_packet; //To hold the response as it gets made
  struct userList master_user_list = {.length = 0, .head = NULL}; //List of all users on the server
  struct roomList master_room_list = {.length = 0, .head = NULL}; //List of all rooms on the server
  struct room *userRoom = NULL; //For usage in switch statements
  struct user *user = NULL;
  char *message;


  if (argc != 3 || strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0)
    DieWithUserMessage("Arguments", ": Please specify a port with -p <port> and nothing else");
  else if (atoi(argv[2]) < 1025 || atoi(argv[2]) > 65535) 
    DieWithUserMessage("Arguments", ": Please specify an appropriate port");
  else
    port = atoi(argv[2]);

  if ((serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithSystemMessage("socket() failed");

  //Construct local address struct
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  //Bind to local address
  if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    DieWithSystemMessage("bind() failed");

  //And listen for incoming connections
  if (listen(serv_sock, 20) == -1) 
    DieWithSystemMessage("listen() failed");

  FD_SET(serv_sock, &master_fds); //add the server socket to the master set
  fdmax = serv_sock; //and assign our highest file descriptor

  //Core loop
  while (1) {
    curr_fds = master_fds;
    if (select(fdmax+1, &curr_fds, NULL, NULL, NULL) == -1) 
      DieWithSystemMessage("select() failed");
    
    //Run through existing connections looking for incoming packets
    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &curr_fds)) { // Found one
        if (i == serv_sock) {  //Let's handle a new connection on serv_sock
          addrlen = sizeof(remote_addr);
          new_fd = accept(serv_sock, (struct sockaddr *) &remote_addr, &addrlen);
          if (new_fd == -1)
            DieWithSystemMessage("accept() failed");
          else {
            FD_SET(new_fd, &master_fds); // Add to the master set
            if (new_fd > fdmax) 
              fdmax = new_fd; 
          }
        } else { // If it's not a new connection, it means we're managing one
          if ((nbytes = recv(i, buffer, 7, 0)) <= 0) {
            // Got an error or there's a connection closed by client
            close(i); 
            FD_CLR(i, &master_fds);
            deleteUser(i, &master_user_list, &master_room_list);
          } else {
            //We got data from a client
            //First let's grab the header to figure out what we're doing
            while (nbytes < 7) {
              if ((nbytes = (recv(i, buffer + nbytes, 7 - nbytes, 0))) <= 0) {
                close(i); //In case the connection breaks mid packet.
                FD_CLR(i, &master_fds);
                deleteUser(i, &master_user_list, &master_room_list);
              } 
            }
            //Now we identify parts of the packet header
            memcpy(&client_packet.initialization, buffer, 2);
            client_packet.initialization = ntohs(client_packet.initialization);
            if (client_packet.initialization != 1047) {
              close(i); // We close the connection if receiving an improper initialization.
              FD_CLR(i, &master_fds);
              deleteUser(i, &master_user_list, &master_room_list);
            }
            memcpy(&client_packet.length, buffer + 2, 4);
            client_packet.length = ntohl(client_packet.length);
            memcpy(&client_packet.request_type, buffer + 6, 1);

            //Let's get the payload now that we know how big it is.
            while (totalbytes < client_packet.length) {
              if ((nbytes = recv(i, buffer + totalbytes, 
                      client_packet.length - totalbytes, 0)) <= 0) {
                close(i); // We close the connection if receiving an improper initialization.
                FD_CLR(i, &master_fds);
                deleteUser(i, &master_user_list, &master_room_list);
              }
              totalbytes += nbytes;
            }
            client_packet.payload = malloc(client_packet.length);
            memset(client_packet.payload, 0, client_packet.length);
            memcpy(client_packet.payload, buffer, client_packet.length);

            //Now we look at the type in order to identify which command to run
            switch (client_packet.request_type) {
              case 255: //Connection
                ;
                struct user *newUser = createUser(i, &master_user_list);
                makeConnectionAckPacket(&server_packet, newUser->name);
                movePacketToBuffer(buffer, &server_packet, 0);
                sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                break;
              
              case 23: //Joining a room
                ;
                user = findUserBySock(i, &master_user_list);
                userRoom = getUserRoom(user);
                char *roomName = extractRoomName(&client_packet);
                char *roomPass = extractRoomPass(&client_packet);
                if (userRoom != NULL)
                  deleteUserFromRoom(user, &master_room_list);
                if ((joinRoom(user, roomName, roomPass, &master_room_list)) == 1) {
                  makeGenericAckPacket(&server_packet);
                  movePacketToBuffer(buffer, &server_packet, 0);
                } else {
                  makeGenericFailPacket(&server_packet, 2);
                  movePacketToBuffer(buffer, &server_packet, 1);
                }
                sendBufferedPacket(i, buffer, ntohl(server_packet.length)); 
                break;

              case 24: //Leaving a room or the server
                user = findUserBySock(i, &master_user_list);
                userRoom = getUserRoom(user);
                if (userRoom != NULL) {
                  deleteUserFromRoom(user, &master_room_list);
                  makeGenericAckPacket(&server_packet);
                  movePacketToBuffer(buffer, &server_packet, 0);
                  sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                } else {
                  makeGenericAckPacket(&server_packet);
                  movePacketToBuffer(buffer, &server_packet, 0);
                  sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                  close(i); 
                  FD_CLR(i, &master_fds);
                  deleteUser(i, &master_user_list, &master_room_list);
                }
                break;

              case 25: //List rooms
                makeRoomListPacket(&server_packet, &master_room_list);
                movePacketToBuffer(buffer, &server_packet, 0);
                sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                break;

              case 26: //List users (regardless of in room or not)
                user = findUserBySock(i, &master_user_list);
                userRoom = getUserRoom(user);
                if (userRoom == NULL)
                  makeUserListPacket(&server_packet, &master_user_list);
                else
                  makeUserListPacket(&server_packet, userRoom->users);
                movePacketToBuffer(buffer, &server_packet, 0); 
                sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                break;

              case 27: //Changing nick
                ;
                user = findUserBySock(i, &master_user_list);
                char *newName = extractNewName(&client_packet);
                if (changeUserName(user, newName, &master_user_list) != 1) {
                  makeGenericFailPacket(&server_packet, 1);
                  movePacketToBuffer(buffer, &server_packet, 1);
                } else {
                  makeGenericAckPacket(&server_packet);  
                  movePacketToBuffer(buffer, &server_packet, 0);
                }
                sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                break;

              case 28: //Sending a PM
                user = findUserBySock(i, &master_user_list);
                char* to_name = extractPMTarget(&client_packet);
                message = extractPMMessage(&client_packet);
                struct user *to_user = findUserByName(to_name, &master_user_list);
                if (to_user == NULL) {
                  makeGenericFailPacket(&server_packet, 0);
                  movePacketToBuffer(buffer, &server_packet, 1);
                  sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                  break;
                } else {
                  makePersonalMessagePacket(&server_packet, user->name, message);
                  movePacketToBuffer(buffer, &server_packet, 2);
                  sendBufferedPacket(to_user->userSocket, buffer, ntohl(server_packet.length));
                  makeGenericAckPacket(&server_packet);
                  movePacketToBuffer(buffer, &server_packet, 0);
                  sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                }
                free(message);
                free(to_name);
                break;

              case 29: //Sending a regular message to current room
                user = findUserBySock(i, &master_user_list);
                if (user->currentRoom == NULL) {
                  makeGenericFailPacket(&server_packet, 3);
                  movePacketToBuffer(buffer, &server_packet, 1);
                  sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                } else {
                  roomName = extractRoomName(&client_packet);
                  struct room* room = findRoomByName(roomName, &master_room_list);
                  message = extractRoomMessage(&client_packet);
                  makeMessagePacket(&server_packet, user->name, roomName, message);
                  movePacketToBuffer(buffer, &server_packet, 2);
                  sendBufferedPacketMessage(room, i, buffer, ntohl(server_packet.length));
                  free(message);
                  free(roomName);
                  makeGenericAckPacket(&server_packet);
                  movePacketToBuffer(buffer, &server_packet, 0);
                  sendBufferedPacket(i, buffer, ntohl(server_packet.length));
                }
                break;
            }
            //Now we reset the client_packet for the next incoming packet.
            totalbytes = 0;
            resetPacket(&server_packet);
          } 
        }
      }
    }
  }
    
  return 0;
}
