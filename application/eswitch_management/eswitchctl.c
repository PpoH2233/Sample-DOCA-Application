#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "cli/eswitch_cli.h"
#include "eswitch_config.h"
#include "router/router.h"

static void print_help(FILE *output, const char *program) {
  eswitch_cli_help(output, program, ESWITCH_SOCKET_PATH);
}

int main(int argc, char **argv) {
  const char *socket_path = getenv("ESWITCH_CONTROL_SOCKET");
  struct sockaddr_un address = {0};
  struct eswitch_cli_command parsed = {0};
  char request[512] = {0};
  char response[16384];
  size_t used = 0;
  size_t sent_total = 0;
  bool response_is_error = false;
  bool first_response = true;
  int fd;

  if (argc < 2) {
    print_help(stderr, argv[0]);
    return EXIT_FAILURE;
  }
  if (argc == 2 &&
      (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    print_help(stdout, argv[0]);
    return EXIT_SUCCESS;
  }
  /* Grammar is validated locally so a stopped daemon still reports misuse.
   * The `vr` group is validated by the router parser below. */
  if (!eswitch_cli_parse((size_t)(argc - 1), (const char *const *)&argv[1],
                         &parsed)) {
    fprintf(stderr, "Invalid command or arguments.\n%s\n",
            eswitch_cli_usage_for_tokens((size_t)(argc - 1),
                                         (const char *const *)&argv[1]));
    print_help(stderr, argv[0]);
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

  if (parsed.verb == ESWITCH_CLI_ROUTER) {
    char error[256];
    if (!router_command_valid(request, error, sizeof(error))) {
      fprintf(stderr, "%s", error);
      return EXIT_FAILURE;
    }
  }

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
    if (errno == ENOENT || errno == ECONNREFUSED) {
      fprintf(stderr,
              "eSwitch Management control socket is not available: %s\n"
              "The daemon may not be running. Check it with:\n"
              "  systemctl status eswitch-management\n",
              socket_path);
    } else {
      fprintf(stderr, "Cannot connect to control socket %s: %s\n",
              socket_path, strerror(errno));
    }
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
