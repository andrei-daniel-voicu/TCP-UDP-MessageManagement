#include <unordered_map>
#include <vector>

#include "helpers.hpp"

struct Client {
  std::string id;
  int sock;
  bool connected;
  std::vector<std::string> messages;

  Client() {
    connected = false;
    sock = -1;
  }

  Client(std::string _id, int _sock) : id(_id) {
    sock = _sock;
    connected = true;
  }
};

std::string getIDFromSocket(
    const std::unordered_map<std::string, Client> &clients, int i) {
  std::string id;
  for (auto &entry : clients)
    if (entry.second.sock == i) return entry.second.id;

  return NULL;
}

int main(int argc, char *argv[]) {
  DIE(argc < 2, "Not enough arguments!");
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  int sockTCP, newSockTCP, port;
  int sockUDP;

  struct sockaddr_in servAddr, cliAddr;
  struct sockaddr_in servAddrUDP;
  const int NON_ZERO_VALUE = 1;
  int sockfd, newsockfd;
  socklen_t cliLen;

  fd_set read_fds;
  fd_set tmp_fds;
  int fdmax = 0;

  // empty the descriptors
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  sockTCP = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockTCP == -1, "Can't create TCP socket!");

  sockUDP = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(sockUDP == -1, "Can't create UDP socket!");

  port = atoi(argv[1]);
  DIE(port == 0, "Wrong port!");

  memset((char *)&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);
  servAddr.sin_addr.s_addr = INADDR_ANY;

  memset((char *)&servAddrUDP, 0, sizeof(servAddrUDP));
  servAddrUDP.sin_family = AF_INET;
  servAddrUDP.sin_port = htons(port);
  inet_aton(argv[1], &(servAddrUDP.sin_addr));

  ret = bind(sockTCP, (struct sockaddr *)&servAddr, sizeof(struct sockaddr));
  DIE(ret < 0, "Can't bind TCP socket!");

  ret = listen(sockTCP, SOMAXCONN);
  DIE(ret < 0, "Can't listen on TCP socket!");

  ret = bind(sockUDP, (struct sockaddr *)&servAddr, sizeof(struct sockaddr));
  DIE(ret < 0, "Can't bind UDP socket!");

  FD_SET(sockTCP, &read_fds);
  FD_SET(sockUDP, &read_fds);
  FD_SET(0, &read_fds);
  fdmax = sockTCP;

  std::unordered_map<std::string, Client> clients;
  std::unordered_map<std::string, std::unordered_map<std::string, bool>> topics;

  while (true) {
    tmp_fds = read_fds;

    ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(ret == -1, "Select failed!");

    for (int i = 0; i <= fdmax; i++) {
      if (!FD_ISSET(i, &tmp_fds)) continue;

      // handle new TCP connection
      if (i == sockTCP) {
        cliLen = sizeof(cliAddr);
        newSockTCP = accept(sockTCP, (struct sockaddr *)&cliAddr, &cliLen);
        DIE(newSockTCP < 0, "accept");

        // disable Nagle algorithm
        int disable = setsockopt(newSockTCP, IPPROTO_TCP, TCP_NODELAY,
                                 (char *)&(NON_ZERO_VALUE), sizeof(int));
        DIE(disable == -1, "disable");

        FD_SET(newSockTCP, &read_fds);
        if (newSockTCP > fdmax) fdmax = newSockTCP;
        continue;
      }

      // handle UDP connection
      if (i == sockUDP) {
        socklen_t sizeAddr = sizeof(struct sockaddr_in);
        int n = recvfrom(i, buffer + 6, 1551, 0, (struct sockaddr *)&cliAddr,
                         &sizeAddr);
        DIE(n <= 0, "error receiving from UDP");
        char _name[51];
        // fill buffer with topic, type, data
        memcpy(_name, buffer + 6, 50);
        _name[50] = 0;
        std::string name(_name);

        // nobody subscribed to respective topic
        if (topics.find(name) == topics.end()) continue;

        // fill buffer with ip, port
        memcpy(buffer, &(cliAddr.sin_addr), sizeof(uint32_t));
        memcpy(buffer + 4, &(cliAddr.sin_port), sizeof(uint16_t));

        // get clients subscribed to respective topic
        std::unordered_map<std::string, bool> &topic = topics[name];

        // send messages to subscribers if connected
        // if not, store them in vector for further send
        // (if forward true)
        for (auto &entry : topic) {
          Client &cl = clients[entry.first];
          if (cl.connected) {
            SEND_DATA_TO_CLIENT(cl.sock, buffer, n + 6);
          } else if (topic[cl.id] == true) {
            std::string str(buffer, n + 6);
            cl.messages.push_back(str);
          }
        }
        continue;
      }

      // handle stdin input
      if (i == 0) {
        fgets(buffer, BUFLEN - 1, stdin);

        // closes connections and exit program
        if (!strcmp(buffer, "exit\n")) {
          for (int i = 0; i <= fdmax; i++) {
            close(i);
          }
          close(sockTCP);
          close(sockUDP);
          return 0;
        }
        continue;
      }

      // data received from one of the connected clients
      RECEIVE_DATA_FROM_CLIENT(i, buffer);
      if (ret == 0) {
        // connection closed on socket i
        for (auto &entry : clients) {
          if (entry.second.sock == i) {
            std::cout << "Client " << entry.second.id << " disconnected.\n";
            entry.second.connected = false;
          }
        }
        // remove descriptor
        FD_CLR(i, &read_fds);
        continue;
      }

      // receive id from connected client
      if (!strncmp(buffer, "ID: ", 4)) {
        bool newClient = true;
        std::string id(buffer + 4);

        // if found then check for already connected
        if (clients.find(id) != clients.end()) {
          newClient = false;

          // if connected then close connection and remove descriptor
          if (clients[id].connected) {
            std::cout << "Client " << id << " already connected.\n";
            close(i);
            FD_CLR(i, &read_fds);
            continue;
          }
        }

        // print login message
        std::cout << "New client " << id << " connected from "
                  << inet_ntoa(cliAddr.sin_addr) << ":"
                  << ntohs(cliAddr.sin_port) << ".\n";

        // if new client then add it to the clients map
        if (newClient) {
          clients.insert({id, Client(id, i)});
          continue;
        }
        clients[id].sock = i;
        clients[id].connected = true;

        // send messages from queue
        for (auto m : clients[id].messages) {
          SEND_DATA_TO_CLIENT(clients[id].sock, m.c_str(), m.length());
        }

        clients[id].messages.clear();
        continue;
      }

      // handle subscribe command
      if (!strncmp(buffer, "subscribe", 9)) {
        strtok(buffer, " ");
        std::string _name(strtok(NULL, " \n"));
        bool store = atoi(strtok(NULL, " "));
        std::string id = getIDFromSocket(clients, i);

        // add to map if necessary
        if (topics.find(_name) == topics.end())
          topics.insert({_name, std::unordered_map<std::string, bool>()});

        // if client is already subscribed, modify forward value
        if (topics[_name].find(id) != topics[_name].end()) {
          topics[_name][id] = store;
          continue;
        }

        // insert new entry <client id, forward>
        topics[_name].insert({id, store});
        continue;
      }

      // handle unsubscribe command
      if (!strncmp(buffer, "unsubscribe", 11)) {
        strtok(buffer, " ");
        std::string _name(strtok(NULL, " \n"));

        // if topic doesn't exist do nothing
        if (topics.find(_name) == topics.end()) continue;

        // remove subscription, if subscribed
        std::string id = getIDFromSocket(clients, i);
        topics[_name].erase(id);
      }
    }
  }

  // close sockets
  close(sockTCP);
  close(sockUDP);

  return 0;
}
