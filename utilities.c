#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "utilities.h"


struct user *createUser(int sock, struct userList *users) {
  char* name = malloc(8);
  for (int i = 0; i < 1000; i++) {
    sprintf(name, "rand%d", i);
    if (findUserByName(name, users) == NULL) {
      struct user *newUser = malloc(sizeof(struct user));
      newUser->name = name;
      newUser->userSocket = sock;
      struct userNode *newUserNode = createUserNode(newUser);
      struct userNode *curr = users->head;
      users->head = newUserNode;
      newUserNode->next = curr;
      users->length++;
      return newUser;
    }
  }
  return NULL;
}

struct user *findUserBySock(int sock, struct userList *users) {
  struct userNode *curr;
  curr = users->head;
  while (curr != NULL) {
    if (curr->user->userSocket == sock)
      return curr->user;
    curr = curr->next;
  }
  return NULL;
}

struct userNode *createUserNode(struct user *newUser) {
  struct userNode *newNode = malloc(sizeof(struct userNode));
  newNode->user = newUser;
  return newNode;
}

struct room* createRoom(char *name, char *pass, struct roomList *rooms) {
  struct room *room = malloc(sizeof(struct room));
  room->name = malloc(strlen(name) + 1);
  strcpy(room->name, name);
  room->password = malloc(strlen(pass) + 1);
  strcpy(room->password, pass);
  room->users = malloc(sizeof(struct userList));
  room->next = NULL;

  struct room *curr = rooms->head;
  if (curr == NULL) {
    rooms->head = room;
    rooms->length++;
    return room;
  } else if (strcmp(name, curr->name) < 0) {
    rooms->head = room;
    rooms->length++;
    room->next = curr;
    return room;
  } else {
    struct room *next = curr->next;
    while (next != NULL) {
      if (strcmp(name, curr->name) > 0 && strcmp(name, next->name) < 0) {
        curr->next = room;
        room->next = next;
        rooms->length++;
        return room;
      }
      curr = curr->next;
      next = curr->next;
    }
    curr->next = room;
    rooms->length++;
    return room;
  }
}

int joinRoom(struct user *user, char *name, char *pass, struct roomList *rooms) {
  struct room* toJoin = findRoomByName(name, rooms);
  if (toJoin == NULL) 
    toJoin = createRoom(name, pass, rooms);
  if (strcmp(pass, toJoin->password) == 0) {
    struct userNode *user_node = malloc(sizeof(struct userNode));
    user_node->user = user;
    if (toJoin->users->head == NULL) 
      toJoin->users->head = user_node;
    else {
      user_node->next = toJoin->users->head;
      toJoin->users->head = user_node; 
    }
    toJoin->users->length++;
    user->currentRoom = toJoin;
    return 1;
  } else {
    return 0;
  }
}

int deleteUser(int socket, struct userList *users, struct roomList *masterRoomList) {
  struct userNode *curr = users->head;
  if (curr == NULL) 
    return 0;
  if (curr->user->userSocket == socket) { //If it's the head
    if (curr->user->currentRoom != NULL)
      deleteUserFromRoom(curr->user, masterRoomList);
    free(curr->user->name);
    users->head = curr->next;
    free(curr->user);
    free(curr);
    users->length--;
    return 1;
  }

  //Otherwise we go through the list
  struct userNode *next = curr->next;
  struct userNode *newNext;
  while (next != NULL) {
    if (next->user->userSocket == socket) {
      if (next->user->currentRoom != NULL) 
        deleteUserFromRoom(next->user, masterRoomList);
      free(next->user->name);
      newNext = next->next;
      free(next);
      curr->next = newNext;
      return 1;
    } else {
      curr = curr->next;
      next = curr->next;
    }
  }
  return 0;  
}

int removeUserFromList(struct user *toRemove, struct userList *list) {
  struct userNode *curr = list->head;
  if (curr == NULL)
    return 0;
  if (curr->user == toRemove) {
    list->head = curr->next; 
    list->length--;
    return 1;
  }
  struct userNode *next = curr->next;
  while (next != NULL) {
    if (next->user == toRemove) {
      curr->next = next->next;
      list->length--;
      return 1;
    }
    curr = curr->next;
    next = curr->next;
  }
  return 0;
}

int deleteUserFromRoom(struct user *currUser, struct roomList *masterRoomList) {
  struct room *userRoom = currUser->currentRoom;
  int ret = removeUserFromList(currUser, userRoom->users);
  if (ret == 1)
    currUser->currentRoom = NULL;
  if (userRoom->users->length == 0) 
    deleteRoom(userRoom, masterRoomList);
  return ret;
}

int deleteRoom(struct room *userRoom, struct roomList *masterRoomList) {
  struct room *currRoom = masterRoomList->head;
  if (currRoom == userRoom) {
    free(currRoom->name);
    free(currRoom->users);
    free(currRoom->password);
    if (masterRoomList->length > 1)
      masterRoomList->head = currRoom->next;
    else 
      masterRoomList->head = NULL;
    free(currRoom);
    masterRoomList->length--;
    return 1;
  } else if (currRoom->next == NULL) {
    return 0;
  }
  struct room *nextRoom;
  while (currRoom->next != NULL) {
    nextRoom = currRoom->next;
    if (nextRoom == userRoom) {
      free(nextRoom->next->name);
      free(nextRoom->users);
      free(nextRoom->password);
      struct room *newNextRoom = nextRoom->next;
      free(nextRoom);
      currRoom->next = newNextRoom;
      masterRoomList->length--;
      return 1;
    }
    currRoom = nextRoom;
  }
  return 0;
}

struct room *findRoomByName(char *name, struct roomList *rooms) {
  struct room* curr = rooms->head;
  while (curr != NULL) {
    if (strcmp(curr->name,name) == 0)
      return curr;
    curr = curr->next;
  }
  return NULL;
}

struct user *findUserByName(char* name, struct userList *users) {
  struct userNode *curr = users->head;
  if (curr == NULL) 
    return NULL;
  while (curr != NULL) {
    if (strcmp(name, curr->user->name) == 0)
      return curr->user;
    curr = curr->next;
  }
  return NULL;
}

struct room *getUserRoom(struct user *user) {
  return user->currentRoom;
}

int changeUserName(struct user *user, char *newName, struct userList *list) {
   if (findUserByName(newName, list) != NULL)
     return 0;
   free(user->name);
   user->name = malloc(strlen(newName) + 1);
   strcpy(user->name, newName);
   return 1;
}

