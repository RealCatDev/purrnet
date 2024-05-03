#include <purrnet/purrnet.h>

#include <stdio.h>
#include <stdlib.h>

void callback(purrnet_socket_t *client) {
  char *ip = inet_ntoa(client->addr.sin_addr);
  purrnet_port_t port = htons(client->addr.sin_port);
  printf("Client (%s:%hu) connected!\n", ip, port);
  purrnet_message_t msg = purrnet_message_create(2048);
  purrnet_result_t result;
  while ((result = purrnet_socket_recv(client, &msg)) == PURRNET_SUCCESS) {
    printf("Client (%s:%hu): %*s\n", ip, port, (int)msg.count, msg.items);
  }
  printf("Client (%s:%hu) disconnected!\n", ip, port);
}

int main(int argc, char **argv) {
  purrnet_init();

  purrnet_socket_t *server = purrnet_socket_create(PURRNET_TCP);
  purrnet_addr_t addr = purrnet_addr_create_all(6969);
  purrnet_result_t result = PURRNET_SUCCESS;
  if ((result = purrnet_socket_bind(server, addr)) != PURRNET_SUCCESS) {
    fprintf(stderr, "Failed to bind socket! Error: %d\n", result);
    return 1;
  }
  printf("Listening!\n");
  if (purrnet_socket_listen(server, callback) != PURRNET_SUCCESS) return 1;

  purrnet_socket_free(server);

  purrnet_exit();

  return 0;
}