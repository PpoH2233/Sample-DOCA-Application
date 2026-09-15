/* Grammar tests for the resource-first CLI contract. These run on any host:
 * the parser is shared by eswitchctl and the daemon's raw-socket path, so a
 * case proven here holds for both transports. */
#include "eswitch_cli.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;

/* Exercises the raw-socket path: a request line exactly as a Python or
 * CloudStack adapter would write it, newline included. */
static struct eswitch_cli_command line(const char *request, bool expected) {
  struct eswitch_cli_command parsed = {0};
  bool ok = eswitch_cli_parse_line(request, &parsed);

  if (ok != expected)
    fprintf(stderr, "unexpected result for \"%s\"\n", request);
  assert(ok == expected);
  checks++;
  return parsed;
}

static void accept_verb(const char *request, enum eswitch_cli_verb verb,
                        bool legacy) {
  struct eswitch_cli_command parsed = line(request, true);

  assert(parsed.verb == verb);
  assert(parsed.legacy == legacy);
}

static void accept_id(const char *request, enum eswitch_cli_verb verb,
                      uint16_t id, bool legacy) {
  struct eswitch_cli_command parsed = line(request, true);

  assert(parsed.verb == verb);
  assert(parsed.has_id && parsed.id == id);
  assert(parsed.legacy == legacy);
}

static void accept_port(const char *request, enum eswitch_cli_verb verb,
                        uint16_t id, uint16_t port, bool legacy) {
  struct eswitch_cli_command parsed = line(request, true);

  assert(parsed.verb == verb);
  assert(parsed.has_id && parsed.id == id);
  assert(parsed.has_port && parsed.port_id == port);
  assert(parsed.legacy == legacy);
}

static void reject(const char *request) { line(request, false); }

int main(void) {
  /* Canonical service commands. */
  accept_verb("status\n", ESWITCH_CLI_STATUS, false);
  accept_verb("tx-debug\n", ESWITCH_CLI_TX_DEBUG, false);

  /* Canonical vs resource. */
  accept_id("vs create --id 100\n", ESWITCH_CLI_VS_CREATE, 100, false);
  accept_id("vs delete --id 100\n", ESWITCH_CLI_VS_DELETE, 100, false);
  accept_port("vs port attach --id 100 --port 1\n",
              ESWITCH_CLI_VS_PORT_ATTACH, 100, 1, false);
  accept_port("vs port detach --id 100 --port 1\n",
              ESWITCH_CLI_VS_PORT_DETACH, 100, 1, false);
  /* Named options are order independent. */
  accept_port("vs port detach --port 1 --id 100\n",
              ESWITCH_CLI_VS_PORT_DETACH, 100, 1, false);

  /* The vs show filter is optional: absent means the whole collection. */
  struct eswitch_cli_command all = line("vs show\n", true);
  assert(all.verb == ESWITCH_CLI_VS_SHOW && !all.has_id && all.id == 0);
  accept_id("vs show --id 200\n", ESWITCH_CLI_VS_SHOW, 200, false);

  /* Canonical port and fdb resources. */
  accept_verb("port show\n", ESWITCH_CLI_PORT_SHOW, false);
  struct eswitch_cli_command fdb = line("fdb show\n", true);
  assert(fdb.verb == ESWITCH_CLI_FDB_SHOW && !fdb.has_id && fdb.id == 0);
  accept_id("fdb show --id 100\n", ESWITCH_CLI_FDB_SHOW, 100, false);

  /* The vr group is recognized but delegated to the router parser. */
  accept_verb("vr create --id 5\n", ESWITCH_CLI_ROUTER, false);
  accept_verb("vr port attach --id 5 --port 7 --name up0\n",
              ESWITCH_CLI_ROUTER, false);
  accept_verb("vr\n", ESWITCH_CLI_ROUTER, false);
  accept_verb("vr nonsense --whatever\n", ESWITCH_CLI_ROUTER, false);
  assert(eswitch_cli_is_router_line("vr show --id 5\n"));
  accept_verb("link create --id 10\n", ESWITCH_CLI_ROUTER, false);
  assert(eswitch_cli_is_router_line("link show --id 10\n"));
  assert(eswitch_cli_is_router_line("vr\n"));
  assert(!eswitch_cli_is_router_line("vrx show\n"));
  assert(!eswitch_cli_is_router_line("vs show\n"));
  checks += 4;

  /* Legacy flat aliases keep working and are reported as deprecated. */
  accept_id("vs-create --id 100\n", ESWITCH_CLI_VS_CREATE, 100, true);
  accept_id("vs-delete --id 100\n", ESWITCH_CLI_VS_DELETE, 100, true);
  accept_port("vs-port-attach --id 100 --port 1\n",
              ESWITCH_CLI_VS_PORT_ATTACH, 100, 1, true);
  accept_port("vs-port-detach --port 1 --id 100\n",
              ESWITCH_CLI_VS_PORT_DETACH, 100, 1, true);
  accept_verb("vs-list\n", ESWITCH_CLI_VS_SHOW, true);
  accept_verb("show-fdb\n", ESWITCH_CLI_FDB_SHOW, true);
  accept_id("show-fdb --id 100\n", ESWITCH_CLI_FDB_SHOW, 100, true);
  accept_verb("list-port-available\n", ESWITCH_CLI_PORT_SHOW, true);
  /* Version-1 positional forms. */
  accept_id("vs-create 100\n", ESWITCH_CLI_VS_CREATE, 100, true);
  accept_id("vs-delete 100\n", ESWITCH_CLI_VS_DELETE, 100, true);
  accept_port("vs-port-attach 100 1\n", ESWITCH_CLI_VS_PORT_ATTACH, 100, 1,
              true);
  accept_port("vs-port-detach 100 1\n", ESWITCH_CLI_VS_PORT_DETACH, 100, 1,
              true);
  accept_id("show-fdb 100\n", ESWITCH_CLI_FDB_SHOW, 100, true);
  /* 0x notation stays accepted, as in version 1. */
  accept_id("vs create --id 0x64\n", ESWITCH_CLI_VS_CREATE, 100, false);

  /* Invalid grammar. Canonical nested forms are strict: no positional values,
   * no unknown or duplicate options, no missing required options. */
  reject("");
  reject("\n");
  reject("vs\n");
  reject("vs create\n");
  reject("vs create 100\n");
  reject("vs create --id\n");
  reject("vs create --id 100 --id 101\n");
  reject("vs create --id 65536\n");
  reject("vs create --id 0\n");
  reject("vs delete --id 0\n");
  reject("vs create --id -1\n");
  reject("vs create --id abc\n");
  reject("vs create --id 100 --port 1\n");
  reject("vs remove --id 100\n");
  reject("vs port\n");
  reject("vs port attach --id 100\n");
  reject("vs port attach --id 0 --port 1\n");
  reject("vs port attach --port 1\n");
  reject("vs port connect --id 100 --port 1\n");
  reject("vs port attach 100 1\n");
  reject("vs show --port 1\n");
  reject("vs show --id 100 extra\n");
  reject("port\n");
  reject("port show --id 1\n");
  reject("port list\n");
  reject("fdb\n");
  reject("fdb dump\n");
  reject("fdb show --port 1\n");
  reject("status --id 1\n");
  reject("tx-debug --id 1\n");
  reject("vs-list --port 1\n");
  reject("list-port-available --id 1\n");
  reject("nonsense\n");
  reject("vs-porta-ttach --id 1 --port 1\n");
  reject("vs-create 0\n");
  reject("vs-port-attach 0 1\n");

  /* Whitespace and CRLF handling on the raw socket. */
  accept_port("  vs   port\tattach  --id 100 --port 1  \r\n",
              ESWITCH_CLI_VS_PORT_ATTACH, 100, 1, false);
  accept_verb("status", ESWITCH_CLI_STATUS, false);
  /* One command per connection: trailing bytes after the first line are not
   * part of this request, matching the version-1 daemon. */
  accept_verb("status\nvs show\n", ESWITCH_CLI_STATUS, false);
  accept_id("vs show --id 7\r\ngarbage\r\n", ESWITCH_CLI_VS_SHOW, 7, false);

  /* Oversized lines are refused rather than silently truncated. */
  char oversized[700];
  memset(oversized, 'x', sizeof(oversized) - 1);
  oversized[sizeof(oversized) - 1] = '\0';
  reject(oversized);
  char exact_limit[ESWITCH_CLI_REQUEST_SIZE];
  memset(exact_limit, ' ', ESWITCH_CLI_MAX_COMMAND_SIZE);
  memcpy(exact_limit, "status", strlen("status"));
  exact_limit[ESWITCH_CLI_MAX_COMMAND_SIZE] = '\n';
  exact_limit[ESWITCH_CLI_MAX_COMMAND_SIZE + 1] = '\0';
  accept_verb(exact_limit, ESWITCH_CLI_STATUS, false);
  char over_limit[ESWITCH_CLI_REQUEST_SIZE + 1];
  memset(over_limit, ' ', ESWITCH_CLI_MAX_COMMAND_SIZE + 1);
  memcpy(over_limit, "status", strlen("status"));
  over_limit[ESWITCH_CLI_MAX_COMMAND_SIZE + 1] = '\0';
  reject(over_limit);
  /* A long trailing payload behind a valid first line is still fine. */
  char trailing[900];
  snprintf(trailing, sizeof(trailing), "vs show --id 9\n");
  memset(trailing + strlen(trailing), 'y', sizeof(trailing) - strlen(trailing) - 1);
  trailing[sizeof(trailing) - 1] = '\0';
  accept_id(trailing, ESWITCH_CLI_VS_SHOW, 9, false);

  /* Usage hints name the canonical form even for a legacy alias. */
  assert(strstr(eswitch_cli_usage_for_line("vs-port-detach --id 1\n"),
                "vs port detach --id <id> --port <port-id>"));
  assert(strstr(eswitch_cli_usage_for_line("show-fdb --port 1\n"),
                "fdb show [--id <id>]"));
  assert(strstr(eswitch_cli_usage_for_line("vs-list --port 1\n"),
                "vs show [--id <id>]"));
  assert(strstr(eswitch_cli_usage_for_line("list-port-available --id 1\n"),
                "port show"));
  assert(strstr(eswitch_cli_usage_for_line("vs create\n"),
                "vs create --id <id>"));
  assert(strstr(eswitch_cli_usage_for_line("vs port attach --id 0 --port 1\n"),
                "vs port attach --id <id> --port <port-id>"));
  assert(strstr(eswitch_cli_usage_for_line("vs port detach --id 0 --port 1\n"),
                "vs port detach --id <id> --port <port-id>"));
  assert(strstr(eswitch_cli_usage_for_line("vs port bogus\n"),
                "vs port attach|detach"));
  assert(strstr(eswitch_cli_usage_for_line("nonsense\n"), "--help"));
  checks += 9;

  /* Canonical help must not advertise the deprecated flat verbs. */
  FILE *help = tmpfile();
  assert(help);
  eswitch_cli_help(help, "eswitchctl", "/run/eswitch-management/control.sock");
  long length = ftell(help);
  assert(length > 0 && (size_t)length < 8192);
  rewind(help);
  char text[8192] = {0};
  assert(fread(text, 1, (size_t)length, help) == (size_t)length);
  assert(fclose(help) == 0);
  assert(strstr(text, "vs create --id <id>"));
  assert(strstr(text, "vs port attach --id <id> --port <port>"));
  assert(strstr(text, "vs show [--id <id>]"));
  assert(strstr(text, "fdb show [--id <id>]"));
  assert(strstr(text, "port show"));
  assert(strstr(text, "vr port attach"));
  assert(strstr(text, "vr switch detach"));
  /* Only the deprecation note may mention the old verbs. */
  assert(strstr(text, "Deprecated flat aliases"));
  assert(strstr(text, "vs-create --id") == NULL);
  assert(strstr(text, "vs-port-attach --id") == NULL);
  assert(strstr(text, "show-fdb [--id") == NULL);
  checks += 11;

  printf("PASS: %u CLI grammar checks (canonical, legacy, invalid, raw line, "
         "usage and help)\n",
         checks);
  return 0;
}
