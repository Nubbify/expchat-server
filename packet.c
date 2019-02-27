#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "packet.h"

void makeRoomListPacket(struct packet *pack, struct roomList *list) {
  int pack_len = 1; //Success acknowledgement takes up a spot.
  int counter = 0;
  uint8_t name_size = 0;
  unsigned char *payload = malloc(0);
  unsigned char *temp;
  struct room *curr = list->head;
  while (curr != NULL) {
    name_size = strlen(curr->name);
    pack_len += 1 + name_size;
    temp = realloc(payload, pack_len);
    if (temp == NULL) {
      exit(-1);
    }
    payload = temp;
    memcpy(payload + counter, &name_size, 1);
    memcpy(payload + counter + 1, curr->name, name_size);
    counter = pack_len - 1;
    curr = curr->next;
  }

  pack->initialization = htons(1047);
  pack->length = htonl(pack_len);
  pack->request_type = 254;
  pack->payload = payload;
}

void makeUserListPacket(struct packet *pack, struct userList *list) {
  int pack_len = 1; //Success acknowledgement takes up a spot.
  int counter = 0;
  uint8_t name_size = 0;
  unsigned char *payload = malloc(0);
  unsigned char *temp;
  struct userNode *curr = list->head;
  while (curr != NULL) {
    name_size = strlen(curr->user->name);
    pack_len += 1 + name_size;
    temp = realloc(payload, pack_len);
    if (temp == NULL) {
      exit(-1);
    }
    payload = temp;
    memcpy(payload + counter, &name_size, 1);
    memcpy(payload + counter + 1, curr->user->name, name_size);
    counter = pack_len - 1;
    curr = curr->next;
  }

  pack->initialization = htons(1047);
  pack->length = htonl(pack_len);
  pack->request_type = 254;
  pack->payload = payload;
  
}

void makeMessagePacket(struct packet *pack, char *from, char *to, char *message) {
  pack->initialization = htons(1047);
  pack->request_type = 29;
  uint8_t fromlen = strlen(from);
  uint8_t tolen = strlen(to);
  uint16_t messagelen = htons(strlen(message));
  uint32_t pack_len = fromlen + tolen + ntohs(messagelen) + 4;//1 + 1 + 2 for lengths
  pack->length = htonl(pack_len);
  pack->payload = malloc(pack_len);
  memcpy(pack->payload, &tolen, 1);
  memcpy(pack->payload + 1, to, tolen);
  memcpy(pack->payload + 1 + tolen, &fromlen, 1);
  memcpy(pack->payload + 1 + tolen + 1, from, fromlen);
  memcpy(pack->payload + 1 + tolen + 1 + fromlen, &messagelen, 2);
  memcpy(pack->payload + 1 + tolen + 1 + fromlen + 2, message, ntohs(messagelen));
}

void makePersonalMessagePacket(struct packet *pack, char *from, char *message) {
  pack->initialization = htons(1047);
  pack->request_type = 28;
  uint8_t fromlen = strlen(from);
  uint16_t messagelen = htons(strlen(message));
  uint32_t pack_len = fromlen + ntohs(messagelen) + 3;//1 + 2 for lengths)
  pack->length = htonl(pack_len);
  pack->payload = malloc(pack_len);
  memcpy(pack->payload, &fromlen, 1);
  memcpy(pack->payload + 1, from, fromlen);
  memcpy(pack->payload + 1 + fromlen, &messagelen, 2);
  memcpy(pack->payload + 1 + fromlen + 2, message, ntohs(messagelen));
}

void makeConnectionAckPacket(struct packet *pack, char* name) {
  pack->initialization = htons(1047);
  pack->length = htonl(strlen(name) + 1);
  pack->request_type = 254;
  pack->payload = malloc(strlen(name));
  memcpy(pack->payload, name, strlen(name));
}

void resetPacket(struct packet *pack) {
  pack->initialization = 0;
  pack->length = 0;
  pack->request_type = 0;
  if (pack->payload != NULL)
    free(pack->payload);
  pack->payload = NULL;
}

void makeGenericAckPacket(struct packet *pack) {
  pack->initialization = htons(1047);
  pack->length = htonl(1);
  pack->request_type = 254;
}

void makeGenericFailPacket(struct packet *pack, int type) {
  pack->initialization = htons(1047);
  pack->request_type = 254;
  if (type == 3) {
    pack->length = htonl(1 + strlen("You shout into the void and hear nothing but silence."));
    pack->payload = malloc(strlen("You shout into the void and hear nothing but silence."));
    strcpy((char *)pack->payload, "You shout into the void and hear nothing but silence.");
  }
  if (type == 2) {
    pack->length = htonl(1 + strlen("Wrong password. You shall not pass!"));
    pack->payload = malloc(strlen("Wrong password. You shall not pass!") + 1);
    strcpy((char *)pack->payload, "Wrong password. You shall not pass!");
  }
  if (type == 1) {
    pack->length = htonl(1 + strlen("Someone already nicked that nick."));
    pack->payload = malloc(strlen("Someone already nicked that nick.") + 1);
    strcpy((char *)pack->payload, "Someone already nicked that nick.");
  } 
  if (type == 0) {
    pack->length = htonl(1 + strlen("Nick not found."));
    pack->payload = malloc(strlen("Nick not found.") + 1);
    strcpy((char *)pack->payload, "Nick not found.");
  }
}

void movePacketToBuffer(char *buffer, struct packet *pack, int code) {
  memset(buffer, 0, ntohl(pack->length) + 7);
  memcpy(buffer, &pack->initialization, 2);
  memcpy(buffer + 2, &pack->length, 4);
  memcpy(buffer + 6, &pack->request_type, 1);
  switch (code) {
    case 0: //success packet
      memset(buffer + 7, 0, 1);
      if (pack->payload != NULL)
        memcpy(buffer + 8, pack->payload, ntohl(pack->length) - 1);
      break;
    case 1: //fail packet
      memset(buffer + 7, 1, 1);
      if (pack->payload != NULL)
        memcpy(buffer + 8, pack->payload, ntohl(pack->length) - 1);
      break;
    case 2: //message packet
      if (pack->payload != NULL)
        memcpy(buffer + 7, pack->payload, ntohl(pack->length));
      break;
  }
}

int sendBufferedPacket(int socket, char *buffer, uint32_t length) {
  int totalbytes = 0;
  int currbytes = 0;
  while (totalbytes < 7 + (int)length) {
    currbytes = send(socket, buffer + totalbytes,
        length + 7 - totalbytes, 0);
    totalbytes += currbytes;
    if (currbytes <= 0) {
      return 0;
    }
  } 
  return 1;
}

int sendBufferedPacketMessage(struct room *room, int exemption,char *buffer, uint32_t length) {
  struct userNode *curr = room->users->head;
  int ret = 0;
  while (curr != NULL) {
    if (curr->user->userSocket != exemption)
      ret = sendBufferedPacket(curr->user->userSocket, buffer, length);
    curr = curr->next;
  }
  return ret;
}

char *extractNewName(struct packet *packet) {
  uint8_t nameSize;
  memcpy(&nameSize, packet->payload, 1);
  char *ret = malloc(nameSize + 1);
  memcpy(ret, packet->payload + 1, nameSize);
  ret[nameSize] = '\0';
  return ret;
}

char *extractRoomName(struct packet *packet) {
  uint8_t length;
  memcpy(&length, packet->payload, 1);
  char *ret = malloc(length+1);
  memset(ret, 0, length+1);
  memcpy(ret, packet->payload + 1, length);
  ret[length] = '\0';
  return ret;
}

char *extractRoomPass(struct packet *packet) {
  uint8_t namelen;
  memcpy(&namelen, packet->payload, 1);
  uint8_t passlen;
  memcpy(&passlen, packet->payload + namelen + 1, 1);
  char *ret = malloc(passlen + 1);
  memset(ret, 0, passlen + 1);
  memcpy(ret, packet->payload + namelen + 2, passlen);
  ret[passlen] = '\0';
  return ret;
}

char *extractRoomMessage(struct packet *packet) {
  uint8_t targetlen;
  memcpy(&targetlen, packet->payload, 1);
  uint16_t messagelen;
  memcpy(&messagelen, packet->payload + targetlen + 1, 2);
  messagelen = ntohs(messagelen);
  char *ret = malloc(messagelen + 1);
  memset(ret, 0, messagelen + 1);
  memcpy(ret, packet->payload + targetlen + 3, messagelen);
  ret[messagelen] = '\0';
  return ret;
}

char *extractPMMessage(struct packet *packet) {
  return extractRoomMessage(packet); //similar build
}

char *extractPMTarget(struct packet *packet) {
  return extractRoomName(packet); //similar build
}
