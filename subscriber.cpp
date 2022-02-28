#include <algorithm>
#include <iomanip>

#include "helpers.hpp"

void parseMessage(char *buffer, uint32_t size) {
  // get information from buffer
  char topic[51];
  memcpy(topic, buffer + 6, 50);
  topic[50] = 0;

  uint32_t type = buffer[56];
  char *data = buffer + 57;

  // if type is not recognized do nothing
  if (type < 0 || type > 3) return;

  std::cout << inet_ntoa(*(in_addr *)(buffer)) << ":"
            << ntohs(*(uint16_t *)(buffer + 4)) << " - " << topic;
  switch (type) {
    case 0: {
      std::cout << " - INT - ";
      int sign = data[0];
      long value = ntohl(*(uint32_t *)(data + 1));
      value *= sign == 1 ? -1 : 1;
      std::cout << value << "\n";
      break;
    }
    case 1: {
      std::cout << " - SHORT_REAL - ";
      float value = ntohs(*(uint16_t *)(data));
      float res = (value) / 100;
      printf("%0.2f\n", res);
      break;
    }
    case 2: {
      std::cout << " - FLOAT - ";
      int sign = data[0];
      uint32_t value = ntohl(*(uint32_t *)(data + 1));
      int power = data[5];
      float divider = 1;
      for (int i = 0; i < power; i++) divider *= 10;
      float floatNr = value;
      floatNr *= sign == 1 ? -1 : 1;
      printf("%f\n", floatNr / divider);
      break;
    }
    case 3: {
      std::cout << " - STRING - " << data << "\n";
      break;
    }
    default:
      break;
  }
}

int main(int argc, char *argv[]) {
  DIE(argc < 4, "Not enough arguments!");
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  int sockfd;
  struct sockaddr_in serv_addr;
  std::string id(argv[1]);
  fd_set read_fds;

  FD_ZERO(&read_fds);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd == -1, "Can't create socket");

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(argv[3]));
  ret = inet_aton(argv[2], &serv_addr.sin_addr);
  DIE(ret == 0, "inet_aton");

  ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(ret == -1, "connect");

  FD_SET(sockfd, &read_fds);
  FD_SET(0, &read_fds);

  // send ID
  strcpy(buffer, "ID: ");
  strcat(buffer, id.c_str());
  SEND_DATA_TO_SERVER(sockfd, buffer, strlen(buffer) + 1);

  while (true) {
    fd_set tmp_fds = read_fds;

    ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(ret == -1, "Select failed!");

    // data received from server
    if (FD_ISSET(sockfd, &tmp_fds)) {
      RECEIVE_DATA_FROM_SERVER(sockfd, buffer);
      parseMessage(buffer, ret);
    }

    // data received from stdin
    if (FD_ISSET(0, &tmp_fds)) {
      fgets(buffer, BUFLEN - 1, stdin);
      std::string s(buffer);

      // handle exit command
      if (!strcmp(buffer, "exit\n")) {
        close(sockfd);
        return 0;
      }

      // handle subscribe command
      if (!strncmp(buffer, "subscribe ", 10)) {
        size_t n = std::count(s.begin(), s.end(), ' ');
        if (n != 2) continue;
        SEND_DATA_TO_SERVER(sockfd, buffer, strlen(buffer) + 1);
        std::cout << "Subscribed to topic.\n";
        continue;
      }

      // handle unsubscribe command
      if (!strncmp(buffer, "unsubscribe ", 12)) {
        size_t n = std::count(s.begin(), s.end(), ' ');
        if (n != 1) continue;
        SEND_DATA_TO_SERVER(sockfd, buffer, strlen(buffer) + 1);
        std::cout << "Unsubscribed from topic.\n";
      }
    }
  }

  // close socket
  close(sockfd);

  return 0;
}
