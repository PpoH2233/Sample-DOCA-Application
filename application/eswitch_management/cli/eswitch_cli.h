#ifndef ESWITCH_CLI_H
#define ESWITCH_CLI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../eswitch_vlan.h"

#define ESWITCH_CLI_REQUEST_SIZE 512U
#define ESWITCH_CLI_MAX_COMMAND_SIZE (ESWITCH_CLI_REQUEST_SIZE - 2U)

/* Single source of truth for the L2 command grammar. It is shared by the
 * eswitchctl client-side validator and by the daemon's raw-socket parser so a
 * command accepted locally is always accepted on the wire, and so both sides
 * print the same canonical usage.
 *
 * Canonical grammar is resource-first: <resource> [sub-resource] <action>.
 * The original flat verbs remain accepted as deprecated aliases. The `vr`
 * resource group is owned by router_command_valid()/router_command(); this
 * module only recognizes that a line belongs to it.
 *
 * This module intentionally has no DOCA, DPDK or manager dependency so it can
 * be unit tested on any host. */

enum eswitch_cli_verb {
  ESWITCH_CLI_INVALID = 0,
  ESWITCH_CLI_STATUS,           /* status */
  ESWITCH_CLI_TX_DEBUG,         /* tx-debug */
  ESWITCH_CLI_VS_CREATE,        /* vs create --id <id> */
  ESWITCH_CLI_VS_DELETE,        /* vs delete --id <id> */
  ESWITCH_CLI_VS_SHOW,          /* vs show [--id <id>] */
  ESWITCH_CLI_VS_PORT_ATTACH,   /* vs port attach --id <id> --port <port> */
  ESWITCH_CLI_VS_PORT_DETACH,   /* vs port detach --id <id> --port <port> */
  ESWITCH_CLI_FDB_SHOW,         /* fdb show [--id <id>] */
  ESWITCH_CLI_PORT_SHOW,        /* port show */
  ESWITCH_CLI_ROUTER,           /* vr/link ... (delegated to router parser) */
};

struct eswitch_cli_command {
  enum eswitch_cli_verb verb;
  /* For the show verbs, id 0 means "no filter"; has_id distinguishes an
   * omitted option from an explicit --id 0. */
  uint16_t id;
  uint16_t port_id;
  uint16_t vlan_id;
  uint16_t vlan_last;
  enum eswitch_port_mode port_mode;
  bool has_id;
  bool has_port;
  bool has_mode;
  bool has_vlan;
  /* True when the caller used a deprecated alias or a legacy positional
   * argument instead of the canonical resource-first named form. */
  bool legacy;
};

/* Parses an already tokenized command. tokens[0] is the resource or the legacy
 * flat verb. Returns false on any grammar error without reporting a message;
 * callers pair a failure with eswitch_cli_usage_for_tokens(). */
bool eswitch_cli_parse(size_t token_count, const char *const *tokens,
                       struct eswitch_cli_command *out);

/* Parses one raw request line as received on the control socket. Space and tab
 * separate tokens; parsing stops at the first CR or LF, so one connection
 * carries exactly one command and trailing bytes are ignored. Command text is
 * limited to ESWITCH_CLI_MAX_COMMAND_SIZE bytes, leaving room for LF and NUL
 * in a transport buffer of ESWITCH_CLI_REQUEST_SIZE bytes. */
bool eswitch_cli_parse_line(const char *request,
                            struct eswitch_cli_command *out);

/* True when a raw request line addresses the `vr` resource group, including
 * the bare `vr` word. Such lines must be handed to the router parser, which
 * owns their grammar and their error messages. */
bool eswitch_cli_is_router_line(const char *request);

/* Canonical usage text for the resource the caller attempted, chosen from the
 * leading tokens so an unparsable command still gets a specific hint. Never
 * NULL; the returned string ends with a newline. */
const char *eswitch_cli_usage_for_tokens(size_t token_count,
                                         const char *const *tokens);
const char *eswitch_cli_usage_for_line(const char *request);

/* Canonical help. Aliases are deliberately absent here; CLI.md documents them
 * as deprecated. */
void eswitch_cli_help(FILE *output, const char *program,
                      const char *socket_path);

#endif /* ESWITCH_CLI_H */
