#ifndef   PURRNET_PURRNET_H_
#define   PURRNET_PURRNET_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <iphlpapi.h>
   typedef SOCKET purrnet_sock_handle_t;
#else
#  include <unistd.h>
#  include <sys/types.h> 
#  include <sys/socket.h>
#  include <netinet/in.h>
   typedef int purrnet_sock_handle_t;
#endif

typedef struct sockaddr_in sockaddr_in;
typedef sockaddr_in purrnet_addr_t;
typedef uint16_t purrnet_port_t;

purrnet_addr_t purrnet_addr_create(const char *ip, purrnet_port_t port);
purrnet_addr_t purrnet_addr_create_all(purrnet_port_t port);

typedef enum purrnet_result_e {
  PURRNET_CLOSED = -1,
  PURRNET_SUCCESS = 0,
} purrnet_result_t;

// Socket errors (platform specific)
#ifdef _WIN32
#  define PURRNET_ADDR_IN_USE WSAEADDRINUSE
#else
#  define PURRNET_ADDR_IN_USE EADDRINUSE
#endif

typedef enum purrnet_proto_e {
  PURRNET_TCP = 0,
  PURRNET_UDP,
  PURRNET_PROTOS_COUNT,
} purrnet_proto_t;

bool purrnet_init();
void purrnet_exit();

typedef struct purrnet_message_s {
  char *items;
  size_t count;
  size_t capacity;
} purrnet_message_t;

purrnet_message_t purrnet_message_create(size_t capacity);
purrnet_message_t purrnet_message_create_from_cstr(char *cstr);
void              purrnet_message_free(purrnet_message_t message);

typedef struct purrnet_socket_s {
  purrnet_sock_handle_t handle;
  purrnet_proto_t       protocol;
  purrnet_addr_t        addr;
} purrnet_socket_t;

typedef void(*purrnet_listen_func_t)(purrnet_socket_t*);

purrnet_socket_t *purrnet_socket_create(purrnet_proto_t protocol);
purrnet_result_t  purrnet_socket_connect(purrnet_socket_t *socket, purrnet_addr_t address);
purrnet_result_t  purrnet_socket_bind(purrnet_socket_t *socket, purrnet_addr_t address);
purrnet_result_t  purrnet_socket_listen(purrnet_socket_t *socket, purrnet_listen_func_t cb);
purrnet_result_t  purrnet_socket_send(purrnet_socket_t *socket, purrnet_message_t msg);
purrnet_result_t  purrnet_socket_sendto(purrnet_socket_t *src, purrnet_addr_t dst, purrnet_message_t msg);
purrnet_result_t  purrnet_socket_recv(purrnet_socket_t *socket, purrnet_message_t *msg);
purrnet_result_t  purrnet_socket_recvfrom(purrnet_socket_t *dst, purrnet_addr_t *src, purrnet_message_t *msg);
void              purrnet_socket_free(purrnet_socket_t *socket);

#ifdef __cplusplus
}
#endif

#endif // PURRNET_PURRNET_H_