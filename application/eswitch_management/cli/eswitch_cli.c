#include "eswitch_cli.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define ESWITCH_CLI_MAX_TOKENS 16U

enum option_bit { OPTION_ID = 1u, OPTION_PORT = 2u };

static bool parse_u16(const char *text, uint16_t *value) {
  unsigned long parsed;
  char *end = NULL;

  if (text == NULL || *text == '\0')
    return false;
  errno = 0;
  parsed = strtoul(text, &end, 0);
  if (errno != 0 || *end != '\0' || parsed > UINT16_MAX)
    return false;
  *value = (uint16_t)parsed;
  return true;
}

/* Canonical option scanning: named options only, each at most once, in any
 * order. A missing required option or an unknown option is a grammar error. */
static bool parse_options(size_t start, size_t count,
                          const char *const *tokens, unsigned allowed,
                          unsigned required, struct eswitch_cli_command *out) {
  unsigned found = 0;

  if ((count - start) % 2 != 0)
    return false;
  for (size_t i = start; i < count; i += 2) {
    const char *key = tokens[i];
    const char *value = tokens[i + 1];
    unsigned bit;

    if (strcmp(key, "--id") == 0) {
      bit = OPTION_ID;
      if (!parse_u16(value, &out->id))
        return false;
      out->has_id = true;
    } else if (strcmp(key, "--port") == 0) {
      bit = OPTION_PORT;
      if (!parse_u16(value, &out->port_id))
        return false;
      out->has_port = true;
    } else {
      return false;
    }
    if ((allowed & bit) == 0 || (found & bit) != 0)
      return false;
    found |= bit;
  }
  return (found & required) == required;
}

/* Legacy flat forms also accept the original positional arguments, so
 * `vs-create 100` and `vs-port-attach 100 1` keep working. */
static bool parse_legacy_id(size_t count, const char *const *tokens,
                            bool required, struct eswitch_cli_command *out) {
  if (count == 1)
    return !required;
  if (count == 2) {
    out->has_id = true;
    return parse_u16(tokens[1], &out->id);
  }
  return parse_options(1, count, tokens, OPTION_ID,
                       required ? OPTION_ID : 0u, out);
}

static bool parse_legacy_port(size_t count, const char *const *tokens,
                             struct eswitch_cli_command *out) {
  if (count == 3) {
    out->has_id = true;
    out->has_port = true;
    return parse_u16(tokens[1], &out->id) && parse_u16(tokens[2], &out->port_id);
  }
  return parse_options(1, count, tokens, OPTION_ID | OPTION_PORT,
                       OPTION_ID | OPTION_PORT, out);
}

bool eswitch_cli_parse(size_t token_count, const char *const *tokens,
                       struct eswitch_cli_command *out) {
  const char *resource;

  if (out == NULL)
    return false;
  *out = (struct eswitch_cli_command){.verb = ESWITCH_CLI_INVALID};
  if (tokens == NULL || token_count == 0 || token_count > ESWITCH_CLI_MAX_TOKENS)
    return false;
  resource = tokens[0];

  /* The router group owns its own grammar, including the bare `vr` word so the
   * router parser reports the operation list. */
  if (strcmp(resource, "vr") == 0) {
    out->verb = ESWITCH_CLI_ROUTER;
    return true;
  }

  /* Canonical: resource-first. */
  if (strcmp(resource, "status") == 0) {
    out->verb = ESWITCH_CLI_STATUS;
    return token_count == 1;
  }
  if (strcmp(resource, "tx-debug") == 0) {
    out->verb = ESWITCH_CLI_TX_DEBUG;
    return token_count == 1;
  }
  if (strcmp(resource, "vs") == 0) {
    if (token_count < 2)
      return false;
    if (strcmp(tokens[1], "create") == 0 || strcmp(tokens[1], "delete") == 0) {
      out->verb = strcmp(tokens[1], "create") == 0 ? ESWITCH_CLI_VS_CREATE
                                                   : ESWITCH_CLI_VS_DELETE;
      return parse_options(2, token_count, tokens, OPTION_ID, OPTION_ID, out) &&
             out->id != 0;
    }
    if (strcmp(tokens[1], "show") == 0) {
      out->verb = ESWITCH_CLI_VS_SHOW;
      return parse_options(2, token_count, tokens, OPTION_ID, 0u, out);
    }
    if (strcmp(tokens[1], "port") == 0) {
      if (token_count < 3)
        return false;
      if (strcmp(tokens[2], "attach") == 0)
        out->verb = ESWITCH_CLI_VS_PORT_ATTACH;
      else if (strcmp(tokens[2], "detach") == 0)
        out->verb = ESWITCH_CLI_VS_PORT_DETACH;
      else
        return false;
      return parse_options(3, token_count, tokens, OPTION_ID | OPTION_PORT,
                           OPTION_ID | OPTION_PORT, out) &&
             out->id != 0;
    }
    return false;
  }
  if (strcmp(resource, "fdb") == 0) {
    if (token_count < 2 || strcmp(tokens[1], "show") != 0)
      return false;
    out->verb = ESWITCH_CLI_FDB_SHOW;
    return parse_options(2, token_count, tokens, OPTION_ID, 0u, out);
  }
  if (strcmp(resource, "port") == 0) {
    if (token_count != 2 || strcmp(tokens[1], "show") != 0)
      return false;
    out->verb = ESWITCH_CLI_PORT_SHOW;
    return true;
  }

  /* Deprecated flat aliases. Behaviour is identical to the canonical form. */
  out->legacy = true;
  if (strcmp(resource, "vs-create") == 0) {
    out->verb = ESWITCH_CLI_VS_CREATE;
    return parse_legacy_id(token_count, tokens, true, out) && out->id != 0;
  }
  if (strcmp(resource, "vs-delete") == 0) {
    out->verb = ESWITCH_CLI_VS_DELETE;
    return parse_legacy_id(token_count, tokens, true, out) && out->id != 0;
  }
  if (strcmp(resource, "vs-list") == 0) {
    out->verb = ESWITCH_CLI_VS_SHOW;
    return parse_legacy_id(token_count, tokens, false, out);
  }
  if (strcmp(resource, "show-fdb") == 0) {
    out->verb = ESWITCH_CLI_FDB_SHOW;
    return parse_legacy_id(token_count, tokens, false, out);
  }
  if (strcmp(resource, "vs-port-attach") == 0) {
    out->verb = ESWITCH_CLI_VS_PORT_ATTACH;
    return parse_legacy_port(token_count, tokens, out) && out->id != 0;
  }
  if (strcmp(resource, "vs-port-detach") == 0) {
    out->verb = ESWITCH_CLI_VS_PORT_DETACH;
    return parse_legacy_port(token_count, tokens, out) && out->id != 0;
  }
  if (strcmp(resource, "list-port-available") == 0) {
    out->verb = ESWITCH_CLI_PORT_SHOW;
    return token_count == 1;
  }
  out->legacy = false;
  out->verb = ESWITCH_CLI_INVALID;
  return false;
}

static size_t tokenize(const char *request, char *buffer, size_t buffer_size,
                       const char **tokens, size_t max_tokens, bool *overflow) {
  char *save = NULL;
  size_t count = 0;
  size_t length;

  *overflow = false;
  if (request == NULL) {
    *overflow = true;
    return 0;
  }
  /* One command per connection: anything after the first line terminator is
   * not part of this request. */
  length = strcspn(request, "\r\n");
  if (length > ESWITCH_CLI_MAX_COMMAND_SIZE || length >= buffer_size) {
    *overflow = true;
    return 0;
  }
  memcpy(buffer, request, length);
  buffer[length] = '\0';
  for (char *token = strtok_r(buffer, " \t", &save); token != NULL;
       token = strtok_r(NULL, " \t", &save)) {
    if (count == max_tokens) {
      *overflow = true;
      return count;
    }
    tokens[count++] = token;
  }
  return count;
}

bool eswitch_cli_parse_line(const char *request,
                            struct eswitch_cli_command *out) {
  char buffer[ESWITCH_CLI_REQUEST_SIZE];
  const char *tokens[ESWITCH_CLI_MAX_TOKENS];
  bool overflow = false;
  size_t count = tokenize(request, buffer, sizeof(buffer), tokens,
                          ESWITCH_CLI_MAX_TOKENS, &overflow);

  if (out != NULL)
    *out = (struct eswitch_cli_command){.verb = ESWITCH_CLI_INVALID};
  if (overflow)
    return false;
  return eswitch_cli_parse(count, tokens, out);
}

bool eswitch_cli_is_router_line(const char *request) {
  size_t i = 0;

  if (request == NULL)
    return false;
  while (request[i] == ' ' || request[i] == '\t')
    i++;
  if (request[i] != 'v' || request[i + 1] != 'r')
    return false;
  switch (request[i + 2]) {
  case ' ':
  case '\t':
  case '\r':
  case '\n':
  case '\0':
    return true;
  default:
    return false;
  }
}

const char *eswitch_cli_usage_for_tokens(size_t token_count,
                                         const char *const *tokens) {
  const char *resource;

  if (tokens == NULL || token_count == 0)
    return "Run: eswitchctl --help\n";
  resource = tokens[0];
  if (strcmp(resource, "vs-create") == 0)
    return "Usage: vs create --id <id>\n";
  if (strcmp(resource, "vs-delete") == 0)
    return "Usage: vs delete --id <id>\n";
  if (strcmp(resource, "vs-list") == 0)
    return "Usage: vs show [--id <id>]\n";
  if (strcmp(resource, "vs-port-attach") == 0)
    return "Usage: vs port attach --id <id> --port <port-id>\n";
  if (strcmp(resource, "vs-port-detach") == 0)
    return "Usage: vs port detach --id <id> --port <port-id>\n";
  if (strcmp(resource, "show-fdb") == 0)
    return "Usage: fdb show [--id <id>]\n";
  if (strcmp(resource, "list-port-available") == 0)
    return "Usage: port show\n";
  if (strcmp(resource, "fdb") == 0)
    return "Usage: fdb show [--id <id>]\n";
  if (strcmp(resource, "port") == 0)
    return "Usage: port show\n";
  if (strcmp(resource, "status") == 0)
    return "Usage: status\n";
  if (strcmp(resource, "tx-debug") == 0)
    return "Usage: tx-debug\n";
  if (strcmp(resource, "vs") != 0)
    return "Run: eswitchctl --help\n";
  if (token_count >= 2 && strcmp(tokens[1], "create") == 0)
    return "Usage: vs create --id <id>\n";
  if (token_count >= 2 && strcmp(tokens[1], "delete") == 0)
    return "Usage: vs delete --id <id>\n";
  if (token_count >= 2 && strcmp(tokens[1], "show") == 0)
    return "Usage: vs show [--id <id>]\n";
  if (token_count >= 2 && strcmp(tokens[1], "port") == 0) {
    if (token_count >= 3 && strcmp(tokens[2], "attach") == 0)
      return "Usage: vs port attach --id <id> --port <port-id>\n";
    if (token_count >= 3 && strcmp(tokens[2], "detach") == 0)
      return "Usage: vs port detach --id <id> --port <port-id>\n";
    return "Usage: vs port attach|detach --id <id> --port <port-id>\n";
  }
  return "Usage: vs create|delete|show|port attach|port detach ...\n";
}

const char *eswitch_cli_usage_for_line(const char *request) {
  char buffer[ESWITCH_CLI_REQUEST_SIZE];
  const char *tokens[ESWITCH_CLI_MAX_TOKENS];
  bool overflow = false;
  size_t count = tokenize(request, buffer, sizeof(buffer), tokens,
                          ESWITCH_CLI_MAX_TOKENS, &overflow);

  if (count == 0)
    return "Run: eswitchctl --help\n";
  /* The returned pointer is a string literal, so the local copy going out of
   * scope is safe. */
  return eswitch_cli_usage_for_tokens(count, tokens);
}

void eswitch_cli_help(FILE *output, const char *program,
                      const char *socket_path) {
  fprintf(output,
          "Usage: %s <resource> [sub-resource] <action> [options]\n\n"
          "Service:\n"
          "  status                                   Show daemon status\n"
          "  tx-debug                                 Show SF return/TX "
          "diagnostics\n\n"
          "Virtual switch (L2):\n"
          "  vs create --id <id>                      Create a virtual "
          "switch\n"
          "  vs delete --id <id>                      Delete a virtual "
          "switch\n"
          "  vs show [--id <id>]                      Show all or one "
          "virtual switch\n"
          "  vs port attach --id <id> --port <port>   Attach an available "
          "port\n"
          "  vs port detach --id <id> --port <port>   Detach a member port\n\n"
          "Ports and forwarding tables:\n"
          "  port show                                Show unassigned DPDK "
          "ports\n"
          "  fdb show [--id <id>]                     Show all or one "
          "learned FDB\n\n"
          "Virtual router (L3):\n"
          "  vr create|delete|show --id <id>\n"
          "  vr port attach --id <id> --port <port> --name <name>\n"
          "  vr port detach --id <id> --interface <name>\n"
          "  vr switch attach --id <id> --switch-id <vs> --name <name>\n"
          "  vr switch detach --id <id> --interface <name>\n"
          "  vr interface set --id <id> --interface <name> --mac <mac>\n"
          "  vr ip add|del --id <id> --interface <name> "
          "--address <ip/prefix>\n"
          "  vr route add --id <id> --prefix <cidr> --via <ip> "
          "--interface <name>\n"
          "  vr route del --id <id> --prefix <cidr>\n"
          "  vr route show --id <id>\n"
          "  vr nat enable --id <id> --interface <name> "
          "--address <interface|ip>\n"
          "      --port-range <first-last>            TCP/UDP ports and ICMP "
          "Echo IDs\n"
          "  vr nat disable|show --id <id>\n\n"
          "  Private VS gateway ARP and ICMP echo are active. Eligible "
          "private routes\n"
          "  use hardware LPM; unsupported cases fail open to the Arm slow "
          "path.\n"
          "  Arm TCP/UDP/ICMP Echo NAT and uplink ARP are active; hardware CT "
          "is pending.\n\n"
          "  --help, -h                               Show this help\n\n"
          "Deprecated flat aliases (vs-create, vs-list, show-fdb,\n"
          "list-port-available, vr port-attach, ...) are still accepted.\n"
          "See CLI.md for the alias table and the control-socket contract.\n\n"
          "Control socket: %s\n"
          "Override with: ESWITCH_CONTROL_SOCKET=/path/to/socket\n",
          program, socket_path);
}
