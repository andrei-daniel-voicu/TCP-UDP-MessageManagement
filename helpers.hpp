#ifndef _HELPERS_HPP
#define _HELPERS_HPP 1

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

#define BUFLEN 1560

char buffer[BUFLEN];
int ret;

#define DIE(assertion, call_description)                 \
  do {                                                   \
    if (assertion) {                                     \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__); \
      perror(call_description);                          \
      exit(EXIT_FAILURE);                                \
    }                                                    \
  } while (0)

// sends data to server given a socket, buffer and size
// closes socket and returns from current function on 0 bytes sent
// returns number of bytes sent in ret variable
// on error exits the program and prints error
#define SEND_DATA_TO_SERVER(socket, buffer, size) \
  ret = sendData(socket, buffer, size);           \
  if (ret == 0) {                                 \
    close(socket);                                \
    return 0;                                     \
  }                                               \
  DIE(ret == -1, "Error sending data to server!");

// receives data from server given a socket and buffer
// closes socket and returns from current function when 0 bytes received
// returns number of bytes received in ret variable
// on error exits the program and prints error
#define RECEIVE_DATA_FROM_SERVER(socket, buffer) \
  ret = receiveData(socket, buffer);             \
  if (ret == 0) {                                \
    close(socket);                               \
    return 0;                                    \
  }                                              \
  DIE(ret == -1, "Error receiving data from server!");

// sends data to client given a socket, buffer and size
// closes socket when 0 bytes sent
// returns number of bytes received in ret variable
// on error exits the program and prints error
#define SEND_DATA_TO_CLIENT(socket, buffer, size) \
  ret = sendData(socket, buffer, size);           \
  if (ret == 0) close(socket);                    \
  DIE(ret == -1, "Error sending data to client!");

// receives data from client given a socket and buffer
// closes socket when 0 bytes sent
// returns number of bytes received in ret variable
// on error exits the program and prints error
#define RECEIVE_DATA_FROM_CLIENT(socket, buffer) \
  ret = receiveData(socket, buffer);             \
  if (ret == 0) close(socket);                   \
  DIE(ret == -1, "Error receiving data from client!");

int sendData(int socket, const char *buffer, uint32_t size) {
  int n = 0;
  int sent = 0;

  n = send(socket, &size, sizeof(uint32_t), 0);
  if (n <= 0) return n;
  while (sent < size) {
    n = send(socket, buffer + sent, size - sent, 0);
    if (n <= 0) return n;
    sent += n;
  }
  return size;
}

int receiveData(int socket, char *buffer) {
  int n = 0;
  int received = 0;
  int size = 0;

  n = recv(socket, &size, sizeof(uint32_t), 0);
  while (received < size) {
    n = recv(socket, buffer + received, size - received, 0);
    if (n <= 0) return n;
    received += n;
  }
  return size;
}
#endif
