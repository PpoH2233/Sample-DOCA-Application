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

static void print_help(FILE *output, const char *program) {
  fprintf(output,
          "Usage: %s <command> [arguments]\n\n"
          "Commands:\n"
          "  status                              Show daemon status\n"
          "  vs-create --id <id>                 Create a virtual switch\n"
          "  vs-delete --id <id>                 Delete a virtual switch\n"
          "  vs-port-attach --id <id> --port <p> Attach an available port\n"
          "  vs-port-detach --id <id> --port <p> Detach a member port\n"
          "  vs-list                             List virtual switches\n"
          "  show-fdb [--id <id>]                Show all or one FDB\n"
          "  list-port-available                 List unassigned DPDK ports\n"
          "  --help, -h                          Show this help\n\n"
          "Control socket: %s\n"
          "Override with: ESWITCH_CONTROL_SOCKET=/path/to/socket\n",
          program, ESWITCH_SOCKET_PATH);
}

static bool parse_u16_value(const char *text) {
  char *end = NULL;
  unsigned long value;

  if (text == NULL || *text == '\0')
    return false;
  errno = 0;
  value = strtoul(text, &end, 0);
  return errno == 0 && *end == '\0' && value <= UINT16_MAX;
}

static bool valid_id_arguments(int count, char **arguments, bool optional) {
  if (count == 0)
    return optional;
  if (count == 1)
    return parse_u16_value(arguments[0]);
  return count == 2 && strcmp(arguments[0], "--id") == 0 &&
         parse_u16_value(arguments[1]);
}

static bool valid_port_arguments(int count, char **arguments) {
  bool found_vswitch = false;
  bool found_port = false;

  if (count == 2)
    return parse_u16_value(arguments[0]) && parse_u16_value(arguments[1]);
  if (count != 4)
    return false;
  for (int i = 0; i < count; i += 2) {
    if (strcmp(arguments[i], "--id") == 0 && !found_vswitch) {
      found_vswitch = parse_u16_value(arguments[i + 1]);
      if (!found_vswitch)
        return false;
    } else if (strcmp(arguments[i], "--port") == 0 && !found_port) {
      found_port = parse_u16_value(arguments[i + 1]);
      if (!found_port)
        return false;
    } else {
      return false;
    }
  }
  return found_vswitch && found_port;
}

static bool valid_command_line(int argc, char **argv) {
  const char *command = argv[1];
  int argument_count = argc - 2;
  char **arguments = &argv[2];

  if (strcmp(command, "status") == 0 || strcmp(command, "vs-list") == 0 ||
      strcmp(command, "list-port-available") == 0)
    return argument_count == 0;
  if (strcmp(command, "vs-create") == 0 ||
      strcmp(command, "vs-delete") == 0)
    return valid_id_arguments(argument_count, arguments, false);
  if (strcmp(command, "vs-port-attach") == 0 ||
      strcmp(command, "vs-port-detach") == 0)
    return valid_port_arguments(argument_count, arguments);
  if (strcmp(command, "show-fdb") == 0)
    return valid_id_arguments(argument_count, arguments, true);
  return false;
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
    print_help(stderr, argv[0]);
    return EXIT_FAILURE;
  }
  if (argc == 2 &&
      (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    print_help(stdout, argv[0]);
    return EXIT_SUCCESS;
  }
  if (!valid_command_line(argc, argv)) {
    fprintf(stderr, "Invalid command or arguments.\n\n");
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
