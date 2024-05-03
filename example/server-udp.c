#include <purrnet/purrnet.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  purrnet_init();

  purrnet_socket_t *server = purrnet_socket_create(PURRNET_UDP);
  purrnet_addr_t addr = purrnet_addr_create_all(6969);
  purrnet_result_t result = PURRNET_SUCCESS;
  if ((result = purrnet_socket_bind(server, addr)) != PURRNET_SUCCESS) {
    fprintf(stderr, "Failed to bind socket! Error: %d\n", result);
    return 1;
  }
  printf("Listening!\n");
  
  purrnet_addr_t client = {0};
  purrnet_message_t msg = purrnet_message_create(2048);
  while ((result = purrnet_socket_recvfrom(server, &client, &msg)) == PURRNET_SUCCESS) {
    char *ip = inet_ntoa(client.sin_addr);
    purrnet_port_t port = htons(client.sin_port);
    printf("[%s:%hu]: %*s\n", ip, port, (int)msg.count, msg.items);
  }

  purrnet_socket_free(server);

  purrnet_exit();

  return 0;
}