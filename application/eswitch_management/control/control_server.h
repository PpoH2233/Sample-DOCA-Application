#ifndef CONTROL_SERVER_H
#define CONTROL_SERVER_H

#include <stdbool.h>
#include <stddef.h>

#include <doca_error.h>

typedef doca_error_t (*control_command_handler)(const char *request,
                                                char *response,
                                                size_t response_size,
                                                void *context);

struct control_server {
  int listen_fd;
  int client_fd;
  char request[256];
  size_t request_length;
  char socket_path[108];
  control_command_handler handler;
  void *context;
  bool started;
};

doca_error_t control_server_start(struct control_server *server,
                                  const char *socket_path,
                                  control_command_handler handler,
                                  void *context);
doca_error_t control_server_poll(struct control_server *server,
                                 bool *handled_command);
void control_server_stop(struct control_server *server);

#endif /* CONTROL_SERVER_H */
