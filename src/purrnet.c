#include "purrnet/purrnet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __linux__
#  include <netinet/in.h>
#  include <arpa/inet.h>
#endif

#ifdef _WIN32
   static WSADATA gPurrnetWSADATA;
#else

#endif

// global: Functions
bool purrnet_init() {
  #ifdef _WIN32
    if (WSAStartup(MAKEWORD(2,2), &gPurrnetWSADATA) != 0) {
      return false;
    }
  #else
  #endif

  return true;
}

void purrnet_exit() {
  #ifdef _WIN32
    WSACleanup();
  #else
  #endif
}

purrnet_result_t purrnet_socket_error() {
  #ifdef _WIN32
    return WSAGetLastError();
  #else
    return errno;
  #endif
}

// purrnet_addr_t: Functions
purrnet_addr_t purrnet_addr_create(const char *ip, purrnet_port_t port) {
  purrnet_addr_t addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);
  return addr;
}

purrnet_addr_t purrnet_addr_create_all(purrnet_port_t port) {
  purrnet_addr_t addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  return addr;
}

char *purrnet_addr_get_ip(purrnet_addr_t addr) {
  return inet_ntoa(addr.sin_addr);
}

purrnet_port_t purrnet_addr_get_port(purrnet_addr_t addr) {
  return htons(addr.sin_port);
}

// purrnet_message_t: Functions
purrnet_message_t purrnet_message_create(size_t capacity) {
  purrnet_message_t msg = {0};
  msg.capacity = capacity;
  msg.items = malloc(capacity);
  return msg;
}

purrnet_message_t purrnet_message_create_from_cstr(char *cstr) {
  size_t count = strlen(cstr);
  purrnet_message_t msg = {0};
  msg.count = count;
  msg.capacity = count;
  msg.items = cstr;
  return msg;
}

void purrnet_message_free(purrnet_message_t message) {
  free(message.items);
}

// purrnet_socket_t: Functions
purrnet_socket_t *purrnet_socket_create(purrnet_proto_t protocol) {
  if (protocol < 0 || protocol >= PURRNET_PROTOS_COUNT) return NULL;
  purrnet_socket_t *sock = malloc(sizeof(purrnet_socket_t));
  if (!sock) return NULL;
  sock->protocol = protocol;
  switch (protocol) {
  case PURRNET_TCP: {
    sock->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  } break;
  case PURRNET_UDP: {
    sock->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  } break;
  default: {
    free(sock);
    return NULL;
  }
  }

  return sock;
}

purrnet_result_t purrnet_socket_connect(purrnet_socket_t *socket, purrnet_addr_t address) {
  if (connect(socket->handle, (struct sockaddr*)&address, sizeof(address)) != 0) {
    return purrnet_socket_error();
  }
  socket->addr = address;
  return PURRNET_SUCCESS;
}

purrnet_result_t purrnet_socket_bind(purrnet_socket_t *socket, purrnet_addr_t address) {
  socket->addr = address;
  if (bind(socket->handle, (struct sockaddr*)&socket->addr, sizeof(socket->addr)) != 0) {
    return purrnet_socket_error();
  }
  return PURRNET_SUCCESS;
}

typedef struct {
  purrnet_socket_t *socket;
  size_t index;
  bool free;
} purrnet_server_client_socket_t;

typedef struct {
  purrnet_server_client_socket_t *client;
  purrnet_listen_func_t callback;
} purrnet_server_arg_t;

void *handleClient(void *arg) {
  purrnet_server_arg_t *argument = (purrnet_server_arg_t*)arg;
  purrnet_server_client_socket_t *client = argument->client;
  purrnet_listen_func_t cb = argument->callback;
  cb(client->socket);
  purrnet_socket_free(client->socket);
  client->free = true;
  return NULL;
}

purrnet_result_t purrnet_socket_listen(purrnet_socket_t *socket, purrnet_listen_func_t cb) {
  if (listen(socket->handle, SOMAXCONN) != 0) {
    return purrnet_socket_error();
  }

  size_t count = 0;
  size_t capacity = 16;
  purrnet_server_client_socket_t **items = malloc(sizeof(purrnet_server_client_socket_t*) * capacity);

  purrnet_result_t result = PURRNET_SUCCESS;
  while (1) {
    if (count >= capacity) {
      items = realloc(items, capacity = capacity * 2);
    }

    purrnet_server_client_socket_t *client = malloc(sizeof(purrnet_server_client_socket_t));

    memset(client, 0, sizeof(purrnet_server_client_socket_t));
    client->index = count;
    client->socket = malloc(sizeof(purrnet_socket_t));

    items[count++] = client;

    int addrSize = sizeof(client->socket->addr);
    #ifdef _WIN32
      client->socket->handle = accept(socket->handle, (struct sockaddr*)&client->socket->addr, &addrSize);
    #else
      client->socket->handle = accept(socket->handle, (struct sockaddr*)&client->socket->addr, (socklen_t*)&addrSize);
    #endif

    if (client->socket->handle < 0) {
      result = purrnet_socket_error();
      break;
    }

    purrnet_server_arg_t arg = {0};
    arg.client = client;
    arg.callback = cb;

    #ifdef _WIN32
      DWORD thread_id;
      HANDLE thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handleClient, &arg, 0, &thread_id);
      if (thread_handle) CloseHandle(thread_handle);
    #else
      if (fork() == 0) {
        close(socket->handle);
        handleClient(&arg);
        exit(0);
      } else {
        close(client->socket->handle);
      }
    #endif
  }

  for (size_t i = 0; i < count; ++i) {
    if (!items[i]->free) {
      purrnet_socket_free(items[i]->socket);
    }
    free(items[i]);
  }
  free(items);

  return result;
}

purrnet_result_t purrnet_socket_send(purrnet_socket_t *dst, purrnet_message_t msg) {
  if (send(dst->handle, msg.items, msg.count, 0) <= 0) {
    return purrnet_socket_error();
  }
  return PURRNET_SUCCESS;
}

purrnet_result_t purrnet_socket_sendto(purrnet_socket_t *src, purrnet_addr_t dst, purrnet_message_t msg) {
  if (sendto(src->handle, msg.items, msg.count, 0, (struct sockaddr*)&dst, sizeof(dst)) <= 0) {
    return purrnet_socket_error();
  }
  return PURRNET_SUCCESS;
}

purrnet_result_t purrnet_socket_recv(purrnet_socket_t *src, purrnet_message_t *msg) {
  int size = 0;
  if ((size = recv(src->handle, msg->items, msg->capacity, 0)) <= 0) {
    if (size == 0) return PURRNET_CLOSED;
    return purrnet_socket_error();
  }
  msg->count = size;
  return PURRNET_SUCCESS;
}

purrnet_result_t purrnet_socket_recvfrom(purrnet_socket_t *dst, purrnet_addr_t *src, purrnet_message_t *msg) {
  #ifdef _WIN32
    int len = sizeof(*src);
  #else
    socklen_t len = sizeof(*src);
  #endif
  int size = 0;
  if ((size = recvfrom(dst->handle, msg->items, msg->capacity, 0, (struct sockaddr*)src, &len)) <= 0) {
    return purrnet_socket_error();
  }
  msg->count = size;
  return PURRNET_SUCCESS;
}

void purrnet_socket_free(purrnet_socket_t *socket) {
  #ifdef _WIN32
    closesocket(socket->handle);
  #else
    close(socket->handle);
  #endif
}
