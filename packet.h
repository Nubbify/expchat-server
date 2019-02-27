#ifndef _PACKETH_
#define _PACKETH_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "utilities.h"

struct packet {
  uint16_t initialization;
  uint32_t length;
  uint8_t request_type;
  uint8_t* payload; 
};

void makeRoomListPacket(struct packet *, struct roomList *);
void makeUserListPacket(struct packet *, struct userList *);
void makeMessagePacket(struct packet *, char *from, char *to, char *message);
void makePersonalMessagePacket(struct packet *, char *from, char *message);
void makeConnectionAckPacket(struct packet *, char* name);
void makeGenericAckPacket(struct packet *); 
//^join room, name change success, or message sent
void makeGenericFailPacket(struct packet *, int type); 
//^Failed name change (1) or wrong pass(2) or message not sent(0)
void resetPacket(struct packet *pack);
void movePacketToBuffer(char *buffer, struct packet *pack, int code);
int sendBufferedPacket(int socket, char *buffer, uint32_t length);
int sendBufferedPacketMessage(struct room *, int exemption, char *buffer, uint32_t length);
char *extractNewName(struct packet *);
char *extractRoomName(struct packet *);
char *extractRoomPass(struct packet *);
char *extractRoomMessage(struct packet *);
char *extractPMMessage(struct packet *);
char *extractPMTarget(struct packet *);

#endif
