#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "eswitch_config.h"

static void usage(const char *program) {
  fprintf(stderr,
          "Usage:\n"
          "  %s status\n"
          "  %s vs-create <id>\n"
          "  %s vs-delete <id>\n"
          "  %s vs-port-attach <vs-id> <port-id>\n"
          "  %s vs-list\n"
          "  %s show_fdb [vs-id]\n"
          "  %s list-port-available\n",
          program, program, program, program, program, program, program);
}

int main(int argc, char **argv) {
  const char *socket_path = getenv("ESWITCH_CONTROL_SOCKET");
  struct sockaddr_un address = {0};
  char request[256] = {0};
  char response[16384];
  size_t used = 0;
  size_t sent_total = 0;
  bool response_is_error = false;
  bool first_response = true;
  int fd;

  if (argc < 2) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (socket_path == NULL || *socket_path == '\0')
    socket_path = ESWITCH_SOCKET_PATH;
  for (int i = 1; i < argc; i++) {
    int written = snprintf(request + used, sizeof(request) - used, "%s%s",
                           i == 1 ? "" : " ", argv[i]);
    if (written < 0 || (size_t)written >= sizeof(request) - used) {
      fprintf(stderr, "Command is too long\n");
      return EXIT_FAILURE;
    }
    used += (size_t)written;
  }

  if (used + 1 >= sizeof(request)) {
    fprintf(stderr, "Command is too long\n");
    return EXIT_FAILURE;
  }
  request[used++] = '\n';
  request[used] = '\0';

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return EXIT_FAILURE;
  }
  if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) {
    perror("fcntl");
    close(fd);
    return EXIT_FAILURE;
  }
  address.sun_family = AF_UNIX;
  snprintf(address.sun_path, sizeof(address.sun_path), "%s", socket_path);
  if (connect(fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
    fprintf(stderr, "Cannot connect to %s: %s\n", socket_path, strerror(errno));
    close(fd);
    return EXIT_FAILURE;
  }
  while (sent_total < strlen(request)) {
    ssize_t sent =
        send(fd, request + sent_total, strlen(request) - sent_total, 0);
    if (sent < 0) {
      perror("send");
      close(fd);
      return EXIT_FAILURE;
    }
    sent_total += (size_t)sent;
  }
  if (shutdown(fd, SHUT_WR) != 0) {
    perror("shutdown");
    close(fd);
    return EXIT_FAILURE;
  }
  for (;;) {
    ssize_t count = recv(fd, response, sizeof(response), 0);
    if (count < 0) {
      perror("recv");
      close(fd);
      return EXIT_FAILURE;
    }
    if (count == 0)
      break;
    if (first_response) {
      response_is_error = count >= 3 && memcmp(response, "ERR", 3) == 0;
      first_response = false;
    }
    if (fwrite(response, 1, (size_t)count, stdout) != (size_t)count) {
      close(fd);
      return EXIT_FAILURE;
    }
  }
  close(fd);
  return response_is_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
