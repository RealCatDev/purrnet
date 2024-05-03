#include <purrnet/purrnet.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  purrnet_init();

  purrnet_socket_t *client = purrnet_socket_create(PURRNET_TCP);
  purrnet_addr_t addr = purrnet_addr_create("127.0.0.1", 6969);
  purrnet_result_t result = PURRNET_SUCCESS;

  if ((result = purrnet_socket_connect(client, addr)) != PURRNET_SUCCESS) {
    fprintf(stderr, "Failed to connect!");
    return 1;
  }
  printf("Connected!\n");

  purrnet_message_t msg = purrnet_message_create_from_cstr("Hello!");
  purrnet_socket_send(client, msg);

  purrnet_socket_free(client);

  purrnet_exit();

  return 0;
}