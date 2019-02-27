#ifndef _UTILITIESH_
#define _UTILITIESH_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

struct user {
  char *name;
  int userSocket;
  struct room *currentRoom;
};

struct userNode {
  struct user *user;
  struct userNode *next;
};

struct userList {
  struct userNode *head;
  int length;
};

struct room {
  char *name;
  struct userList *users;
  char *password;
  struct room *next;
};

struct roomList {
  struct room *head;
  int length;
};

struct user *createUser(int sock, struct userList *);
struct user *findUserBySock(int sock, struct userList *);
struct userNode *createUserNode(struct user *);
struct room *createRoom(char *name, char *pass, struct roomList *list);
int joinRoom(struct user *, char *name, char *pass, struct roomList *list);
int deleteUser(int socket, struct userList *, struct roomList *masterRoomList);
int removeUserFromList(struct user *, struct userList *);
int deleteUserFromRoom(struct user *, struct roomList *);
int deleteRoom(struct room *, struct roomList *);
struct room *findRoomByName(char* name, struct roomList *);
struct user *findUserByName(char* name, struct userList *);
struct room *getUserRoom(struct user *);
int changeUserName(struct user *, char *newName, struct userList *);

#endif
