#include "eswitch_state.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ESWITCH_STATE_VERSION 1U
#define ESWITCH_STATE_LINE_SIZE 512U

static bool switch_exists(const struct eswitch_state *state,
                          uint16_t vswitch_id) {
  for (size_t i = 0; i < state->switch_count; i++) {
    if (state->switch_ids[i] == vswitch_id)
      return true;
  }
  return false;
}

static bool same_port(const struct eswitch_state_member *left,
                      const struct eswitch_state_member *right) {
  if (left->kind != right->kind)
    return false;
  if (left->kind == ESWITCH_STATE_PORT_PARENT)
    return true;
  return left->host_index == right->host_index &&
         left->pf_index == right->pf_index &&
         left->vf_index == right->vf_index;
}

doca_error_t eswitch_state_init(size_t member_capacity,
                                struct eswitch_state *state) {
  if (state == NULL || member_capacity == 0)
    return DOCA_ERROR_INVALID_VALUE;
  state->members = calloc(member_capacity, sizeof(*state->members));
  if (state->members == NULL)
    return DOCA_ERROR_NO_MEMORY;
  state->member_capacity = member_capacity;
  return DOCA_SUCCESS;
}

void eswitch_state_destroy(struct eswitch_state *state) {
  if (state == NULL)
    return;
  free(state->members);
  *state = (struct eswitch_state){0};
}

doca_error_t eswitch_state_add_switch(struct eswitch_state *state,
                                      uint16_t vswitch_id) {
  if (state == NULL || vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if (switch_exists(state, vswitch_id))
    return DOCA_ERROR_ALREADY_EXIST;
  if (state->switch_count >= ESWITCH_MAX_VSWITCHES)
    return DOCA_ERROR_NO_MEMORY;
  state->switch_ids[state->switch_count++] = vswitch_id;
  return DOCA_SUCCESS;
}

doca_error_t eswitch_state_add_member(
    struct eswitch_state *state, const struct eswitch_state_member *member) {
  if (state == NULL || member == NULL || member->vswitch_id == 0 ||
      !switch_exists(state, member->vswitch_id))
    return DOCA_ERROR_INVALID_VALUE;
  if (state->member_count >= state->member_capacity)
    return DOCA_ERROR_NO_MEMORY;
  for (size_t i = 0; i < state->member_count; i++) {
    if (same_port(&state->members[i], member))
      return DOCA_ERROR_ALREADY_EXIST;
  }
  state->members[state->member_count++] = *member;
  return DOCA_SUCCESS;
}

static bool parse_u32(const char *text, uint32_t *value) {
  char *end = NULL;
  unsigned long parsed;

  if (text == NULL || *text == '\0')
    return false;
  errno = 0;
  parsed = strtoul(text, &end, 10);
  if (errno != 0 || *end != '\0' || parsed > UINT32_MAX)
    return false;
  *value = (uint32_t)parsed;
  return true;
}

static size_t split_tokens(char *line, char **tokens, size_t capacity) {
  char *save = NULL;
  char *token;
  size_t count = 0;

  for (token = strtok_r(line, " \t\r\n", &save); token != NULL;
       token = strtok_r(NULL, " \t\r\n", &save)) {
    if (token[0] == '#')
      break;
    if (count == capacity)
      return capacity + 1;
    tokens[count++] = token;
  }
  return count;
}

static doca_error_t parse_line(char *line, unsigned int line_number,
                               bool *version_seen,
                               struct eswitch_state *state) {
  char *tokens[7];
  size_t count = split_tokens(line, tokens, 7);
  uint32_t values[4];
  struct eswitch_state_member member = {0};
  doca_error_t result;

  if (count == 0)
    return DOCA_SUCCESS;
  if (count == 2 && strcmp(tokens[0], "version") == 0) {
    if (*version_seen || !parse_u32(tokens[1], &values[0]) ||
        values[0] != ESWITCH_STATE_VERSION)
      goto invalid;
    *version_seen = true;
    return DOCA_SUCCESS;
  }
  if (!*version_seen)
    goto invalid;
  if (count == 2 && strcmp(tokens[0], "vswitch") == 0) {
    if (!parse_u32(tokens[1], &values[0]) || values[0] == 0 ||
        values[0] > UINT16_MAX)
      goto invalid;
    result = eswitch_state_add_switch(state, (uint16_t)values[0]);
    if (result != DOCA_SUCCESS)
      goto invalid;
    return DOCA_SUCCESS;
  }
  if (count >= 3 && strcmp(tokens[0], "member") == 0) {
    if (!parse_u32(tokens[1], &values[0]) || values[0] == 0 ||
        values[0] > UINT16_MAX)
      goto invalid;
    member.vswitch_id = (uint16_t)values[0];
    if (count == 3 && strcmp(tokens[2], "parent") == 0) {
      member.kind = ESWITCH_STATE_PORT_PARENT;
    } else if (count == 6 && strcmp(tokens[2], "representor") == 0) {
      for (size_t i = 0; i < 3; i++) {
        if (!parse_u32(tokens[i + 3], &values[i + 1]))
          goto invalid;
      }
      member.kind = ESWITCH_STATE_PORT_REPRESENTOR;
      member.host_index = values[1];
      member.pf_index = values[2];
      member.vf_index = values[3];
    } else {
      goto invalid;
    }
    result = eswitch_state_add_member(state, &member);
    if (result != DOCA_SUCCESS)
      goto invalid;
    return DOCA_SUCCESS;
  }

invalid:
  fprintf(stderr, "Invalid eSwitch configuration at line %u\n", line_number);
  return DOCA_ERROR_INVALID_VALUE;
}

doca_error_t eswitch_state_load(const char *path, struct eswitch_state *state,
                                bool *exists) {
  char line[ESWITCH_STATE_LINE_SIZE];
  bool version_seen = false;
  unsigned int line_number = 0;
  FILE *file;

  if (path == NULL || *path == '\0' || state == NULL || exists == NULL ||
      state->members == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  *exists = false;
  file = fopen(path, "r");
  if (file == NULL) {
    if (errno == ENOENT)
      return DOCA_SUCCESS;
    fprintf(stderr, "Cannot open eSwitch configuration %s: %s\n", path,
            strerror(errno));
    return DOCA_ERROR_IO_FAILED;
  }
  *exists = true;
  while (fgets(line, sizeof(line), file) != NULL) {
    doca_error_t result;
    line_number++;
    if (strchr(line, '\n') == NULL && !feof(file)) {
      fprintf(stderr, "eSwitch configuration line %u is too long\n",
              line_number);
      fclose(file);
      return DOCA_ERROR_TOO_BIG;
    }
    result = parse_line(line, line_number, &version_seen, state);
    if (result != DOCA_SUCCESS) {
      fclose(file);
      return result;
    }
  }
  if (ferror(file)) {
    fprintf(stderr, "Failed reading eSwitch configuration %s\n", path);
    fclose(file);
    return DOCA_ERROR_IO_FAILED;
  }
  if (fclose(file) != 0)
    return DOCA_ERROR_IO_FAILED;
  if (!version_seen) {
    fprintf(stderr, "eSwitch configuration has no version line: %s\n", path);
    return DOCA_ERROR_INVALID_VALUE;
  }
  return DOCA_SUCCESS;
}

static doca_error_t sync_parent_directory(const char *path) {
  char directory[PATH_MAX];
  char *separator;
  int fd;

  if (snprintf(directory, sizeof(directory), "%s", path) >=
      (int)sizeof(directory))
    return DOCA_ERROR_TOO_BIG;
  separator = strrchr(directory, '/');
  if (separator == NULL) {
    snprintf(directory, sizeof(directory), ".");
  } else if (separator == directory) {
    separator[1] = '\0';
  } else {
    *separator = '\0';
  }
  fd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0)
    return DOCA_ERROR_IO_FAILED;
  if (fsync(fd) != 0) {
    close(fd);
    return DOCA_ERROR_IO_FAILED;
  }
  if (close(fd) != 0)
    return DOCA_ERROR_IO_FAILED;
  return DOCA_SUCCESS;
}

doca_error_t eswitch_state_save(const char *path,
                                const struct eswitch_state *state) {
  char temporary[PATH_MAX];
  FILE *file = NULL;
  int fd = -1;
  doca_error_t result = DOCA_ERROR_IO_FAILED;

  if (path == NULL || *path == '\0' || state == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (snprintf(temporary, sizeof(temporary), "%s.tmp.%ld", path,
               (long)getpid()) >= (int)sizeof(temporary))
    return DOCA_ERROR_TOO_BIG;
  fd = open(temporary, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0640);
  if (fd < 0)
    goto fail;
  file = fdopen(fd, "w");
  if (file == NULL)
    goto fail;
  fd = -1;

  if (fprintf(file, "# eSwitch Management persistent state\nversion %u\n",
              ESWITCH_STATE_VERSION) < 0)
    goto fail;
  for (size_t i = 0; i < state->switch_count; i++) {
    if (fprintf(file, "vswitch %u\n", state->switch_ids[i]) < 0)
      goto fail;
  }
  for (size_t i = 0; i < state->member_count; i++) {
    const struct eswitch_state_member *member = &state->members[i];
    int written;

    if (member->kind == ESWITCH_STATE_PORT_PARENT) {
      written = fprintf(file, "member %u parent\n", member->vswitch_id);
    } else {
      written = fprintf(file, "member %u representor %u %u %u\n",
                        member->vswitch_id, member->host_index,
                        member->pf_index, member->vf_index);
    }
    if (written < 0)
      goto fail;
  }
  if (fflush(file) != 0 || fsync(fileno(file)) != 0)
    goto fail;
  if (fclose(file) != 0) {
    file = NULL;
    goto fail;
  }
  file = NULL;
  if (rename(temporary, path) != 0)
    goto fail;
  result = sync_parent_directory(path);
  if (result != DOCA_SUCCESS) {
    /* rename() is the commit point: returning failure here would make the
     * caller roll hardware back while the visible file already has new state. */
    fprintf(stderr,
            "Warning: configuration was renamed but directory fsync failed: "
            "%s\n",
            path);
  }
  return DOCA_SUCCESS;

fail:
  fprintf(stderr, "Failed to save eSwitch configuration %s: %s\n", path,
          strerror(errno));
  if (file != NULL)
    fclose(file);
  else if (fd >= 0)
    close(fd);
  unlink(temporary);
  return result;
}
