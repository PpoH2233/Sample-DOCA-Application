#include "control_server.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "eswitch_config.h"

static bool configure_fd(int fd, bool nonblocking) {
  int descriptor_flags = fcntl(fd, F_GETFD);
  int status_flags = fcntl(fd, F_GETFL);

  if (descriptor_flags < 0 || status_flags < 0)
    return false;
  if (fcntl(fd, F_SETFD, descriptor_flags | FD_CLOEXEC) != 0)
    return false;
  if (nonblocking &&
      fcntl(fd, F_SETFL, status_flags | O_NONBLOCK) != 0)
    return false;
  return true;
}

static doca_error_t prepare_socket_directory(const char *socket_path) {
  char directory[sizeof(((struct sockaddr_un *)0)->sun_path)];
  char *slash;
  struct stat info;

  snprintf(directory, sizeof(directory), "%s", socket_path);
  slash = strrchr(directory, '/');
  if (slash == NULL || slash == directory)
    return DOCA_ERROR_INVALID_VALUE;
  *slash = '\0';
  if (mkdir(directory, 0755) != 0 && errno != EEXIST)
    return DOCA_ERROR_IO_FAILED;
  if (lstat(socket_path, &info) == 0) {
    if (!S_ISSOCK(info.st_mode))
      return DOCA_ERROR_IN_USE;
    if (unlink(socket_path) != 0)
      return DOCA_ERROR_IO_FAILED;
  } else if (errno != ENOENT) {
    return DOCA_ERROR_IO_FAILED;
  }
  return DOCA_SUCCESS;
}

doca_error_t control_server_start(struct control_server *server,
                                  const char *socket_path,
                                  control_command_handler handler,
                                  void *context) {
  struct sockaddr_un address = {0};
  doca_error_t result;

  if (server == NULL || socket_path == NULL || handler == NULL ||
      strlen(socket_path) >= sizeof(address.sun_path))
    return DOCA_ERROR_INVALID_VALUE;
  if (server->started)
    return DOCA_ERROR_BAD_STATE;
  result = prepare_socket_directory(socket_path);
  if (result != DOCA_SUCCESS)
    return result;
  server->listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server->listen_fd < 0 || !configure_fd(server->listen_fd, true)) {
    if (server->listen_fd >= 0)
      close(server->listen_fd);
    server->listen_fd = -1;
    return DOCA_ERROR_IO_FAILED;
  }
  server->client_fd = -1;
  address.sun_family = AF_UNIX;
  snprintf(address.sun_path, sizeof(address.sun_path), "%s", socket_path);
  if (bind(server->listen_fd, (struct sockaddr *)&address,
           offsetof(struct sockaddr_un, sun_path) + strlen(address.sun_path) +
               1) != 0 ||
      chmod(socket_path, 0660) != 0 || listen(server->listen_fd, 16) != 0) {
    close(server->listen_fd);
    server->listen_fd = -1;
    (void)unlink(socket_path);
    return DOCA_ERROR_IO_FAILED;
  }
  snprintf(server->socket_path, sizeof(server->socket_path), "%s",
           socket_path);
  server->handler = handler;
  server->context = context;
  server->started = true;
  return DOCA_SUCCESS;
}

doca_error_t control_server_poll(struct control_server *server,
                                 bool *handled_command) {
  char *response;
  ssize_t received;
  size_t response_offset;

  if (server == NULL || !server->started || handled_command == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  *handled_command = false;
  if (server->client_fd < 0) {
    server->client_fd = accept(server->listen_fd, NULL, NULL);
    if (server->client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return DOCA_SUCCESS;
      return DOCA_ERROR_IO_FAILED;
    }
    if (!configure_fd(server->client_fd, true)) {
      close(server->client_fd);
      server->client_fd = -1;
      return DOCA_ERROR_IO_FAILED;
    }
    server->request_length = 0;
  }

  received = recv(server->client_fd,
                  server->request + server->request_length,
                  sizeof(server->request) - server->request_length - 1, 0);
  if (received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return DOCA_SUCCESS;
    close(server->client_fd);
    server->client_fd = -1;
    return DOCA_ERROR_IO_FAILED;
  }
  if (received == 0 && server->request_length == 0) {
    close(server->client_fd);
    server->client_fd = -1;
    return DOCA_SUCCESS;
  }
  if (received > 0)
    server->request_length += (size_t)received;
  server->request[server->request_length] = '\0';
  if (received > 0 && strchr(server->request, '\n') == NULL &&
      server->request_length + 1 < sizeof(server->request))
    return DOCA_SUCCESS;
  response = calloc(1, ESWITCH_RESPONSE_SIZE);
  if (response == NULL)
    return DOCA_ERROR_NO_MEMORY;
  (void)server->handler(server->request, response, ESWITCH_RESPONSE_SIZE,
                        server->context);
  response_offset = 0;
  while (response_offset < strlen(response)) {
    size_t chunk = strlen(response) - response_offset;
    ssize_t sent;

    if (chunk > 8192)
      chunk = 8192;
    sent = send(server->client_fd, response + response_offset, chunk,
                MSG_NOSIGNAL);
    if (sent < 0) {
      free(response);
      close(server->client_fd);
      server->client_fd = -1;
      return DOCA_ERROR_IO_FAILED;
    }
    response_offset += (size_t)sent;
  }
  free(response);
  close(server->client_fd);
  server->client_fd = -1;
  server->request_length = 0;
  *handled_command = true;
  return DOCA_SUCCESS;
}

void control_server_stop(struct control_server *server) {
  if (server == NULL)
    return;
  if (server->client_fd >= 0)
    close(server->client_fd);
  if (server->listen_fd >= 0)
    close(server->listen_fd);
  if (server->socket_path[0] != '\0')
    (void)unlink(server->socket_path);
  *server = (struct control_server){.listen_fd = -1, .client_fd = -1};
}
