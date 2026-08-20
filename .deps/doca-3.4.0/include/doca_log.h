/*
 * Copyright (c) 2021-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */

/**
 * @file doca_log.h
 * @page logger
 * @defgroup DOCA_LOG DOCA Logging Management
 *
 * Define functions for internal and external logging management
 *
 * To add DOCA trace level compile with "-D DOCA_LOGGING_ALLOW_TRACE"
 *
 * @{
 */

#ifndef DOCA_LOG_H_
#define DOCA_LOG_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buffer size to hold logger name. Including a null terminator.
 */
#define DOCA_LOGGER_NAME_SIZE 256

/**
 * @brief log levels, sorted by verbosity level from high to low
 */
enum doca_log_level {
	DOCA_LOG_LEVEL_DISABLE = 10, /**< Disable log messages */
	DOCA_LOG_LEVEL_CRIT = 20,    /**< Critical log level */
	DOCA_LOG_LEVEL_ERROR = 30,   /**< Error log level */
	DOCA_LOG_LEVEL_WARNING = 40, /**< Warning log level */
	DOCA_LOG_LEVEL_INFO = 50,    /**< Info log level */
	DOCA_LOG_LEVEL_DEBUG = 60,   /**< Debug log level */
	DOCA_LOG_LEVEL_TRACE = 70,   /**< Trace log level */
};

/**
 * @brief Opaque handle representing a logging backend.
 *
 * @details A logging backend defines the destination and mechanism for outputting
 * application or SDK log messages (e.g., console, file, syslog).
 */
struct doca_log_backend;

/**
 * @brief Opaque handle representing a registered logging source.
 *
 * @details A logger corresponds to a specific module or component that generates
 * log messages. It is registered with the logging system and associated with a
 * name and verbosity level.
 */
struct doca_logger;

/**
 * @brief logging backend flush() handler
 */
typedef void (*log_flush_callback)(char *buf);

/**
 * @brief Get the maximum number of backends for application messages that can be created.
 *
 * @return
 * The maximum number of backends that can be created.
 */
DOCA_EXPERIMENTAL
uint32_t doca_log_get_max_num_backends(void);

/**
 * @brief Set the lower log level of a specific logging backend for application messages.
 *
 * Dynamically change the lower log level of the given logging backend, any application message with
 * verbosity level equal or above this level will be shown.
 *
 * @param[in] backend
 * Logging backend to update.
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if level is an invalid verbosity level.
 * - DOCA_ERROR_NOT_PERMITTED - cannot change the log level of this backend.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_set_level_lower_limit(struct doca_log_backend *backend, uint32_t level);

/**
 * @brief Set the upper log level limit of a specific logging backend for application messages.
 *
 * Dynamically change the upper log level limit of the given logging backend, any application message with
 * verbosity level above this level will not be shown.
 *
 * @param[in] backend
 * Logging backend to update.
 * @param[in] upper_limit
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if level is an invalid verbosity level.
 * - DOCA_ERROR_NOT_PERMITTED - cannot change the log level of this backend.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_set_level_upper_limit(struct doca_log_backend *backend, uint32_t upper_limit);

/**
 * @brief Mark the lower log level limit of a specific logging backend for application messages as strict.
 *
 * Mark the lower log level limit of a specific logging backend for application messages as strict,
 * preventing it from being lowered by any future log level changes, both global and direct.
 *
 * @param[in] backend
 * Logging backend to update.
 * @return
 * DOCA_SUCCESS - always.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_set_level_lower_limit_strict(struct doca_log_backend *backend);

/**
 * @brief Mark the upper log level limit of a specific application logging backend for application messages as strict.
 *
 * Mark the upper log level limit of a specific logging backend for application messages as strict,
 * preventing it from being raised by any future log level changes, both global and direct.
 *
 * @param[in] backend
 * Logging backend to update.
 * @return
 * DOCA_SUCCESS - always.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_set_level_upper_limit_strict(struct doca_log_backend *backend);

/**
 * @brief Set the log level of ALL logging backends for application messages.
 *
 * Dynamically change the log level of ALL the logging backends for application messages,
 * any application message with verbosity level equal or above this level will be shown.
 * Newly created logging backends for application messages will use this as their default lower log level limit.
 *
 * If there is a logging backend for application messages that its lower log level limit was set as strict,
 * its lower level won't be set to lower values and it isn't considered an error.
 *
 * Default value of the global lower level limit is DOCA_LOG_LEVEL_INFO.
 *
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if level is an invalid verbosity level.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_level_set_global_lower_limit(uint32_t level);

/**
 * @brief Set the log upper level limit of ALL logging backends for application messages.
 *
 * Dynamically change the log upper level limit of ALL the application logging backends,
 * any application message with verbosity level above this level will not be shown.
 * Newly created logging backends for application messages will use this as their default upper log level limit.
 *
 * If there is a logging backend for application messages that its upper log level limit was set as strict,
 * its upper level won't be set to upper values and it isn't considered an error.
 *
 * Default value of the global upper level limit is DOCA_LOG_LEVEL_CRIT.
 *
 * @param[in] upper_limit
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if level is an invalid verbosity level.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_level_set_global_upper_limit(uint32_t upper_limit);

/**
 * @brief Get the global log level for application messages.
 *
 * Dynamically query for the global lower log level, any application message with verbosity level equal or above this
 * level will be shown.
 * The global lower level is used as the initial value when a new logging backend for application messages is created.
 *
 * @return
 * Log level enum DOCA_LOG_LEVEL.
 */
DOCA_EXPERIMENTAL
uint32_t doca_log_level_get_global_lower_limit(void);

/**
 * @brief Get the global upper log level for application messages.
 *
 * Dynamically query for the global upper log level, any application message with verbosity level above this level will
 * not be shown. The global upper level is used as the initial value when a new logging backend for application messages
 * is created.
 *
 * @return
 * Log level enum DOCA_LOG_LEVEL.
 */
DOCA_EXPERIMENTAL
uint32_t doca_log_level_get_global_upper_limit(void);

/**
 * @brief Set the upper log level for unaggregated messages.
 *
 * Change the log upper level limit for messages to be committed immediately.
 * Any application message with verbosity level above this level will be aggregated as normal.
 *
 * Default value of the max unaggregated level limit is DOCA_LOG_LEVEL_WARNING
 *
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if level is an invalid verbosity level.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_set_max_unaggregated_level(uint32_t level);

/**
 * @brief Set the maximum size of aggregated messages before committing
 *
 * Sets the maximum size of log messages to aggregate before committing them, in bytes.
 *
 * Default value is 128KB
 *
 * @param[in] size
 * Size in bytes of log messages to aggregate.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If size is an invalid value
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_set_max_aggregated_size(uint32_t size);

/**
 * @brief Set the sampling period for committing log messages
 *
 * Set the sampling/sleep period, in microseconds, before committing any outstanding log messages.
 * A value of 0 can use used to commit messages as soon as they are logged.
 *
 * @param[in] usec
 * Sampling period, in microseconds.
 * @return
 * DOCA_SUCCESS - always.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_set_bg_sampling_period(uint32_t usec);

/**
 * @brief Background log aggregation thread
 *
 * This function should be started in its own thread where it will aggregatiate doca_log messages
 * and periodically commit messages as defined using the doca_log_set_max_aggregated_size and
 * doca_log_get_bg_sampling_period functions.
 * If this function is not called, all log messages will be committed immediately.
 *
 * @param[in] unused
 * Not used
 * @return
 * Does not return, will run continuously
 */
DOCA_EXPERIMENTAL
void *doca_log_process_msg(void *unused);

/**
 * @brief Register a log source.
 *
 * Will return the identifier associated with the log source. Log source is used to describe the logging
 * module of the messages in that source file.
 *
 * The maximum allowed length for `source_name` is (DOCA_LOGGER_NAME_SIZE - 1) characters.
 * If the name exceeds this limit, it will be truncated.
 *
 * @note Recommended to only be used via DOCA_LOG_REGISTER.
 *
 * @param[in] source_name
 * The string identifying the log source. Should be in an hierarchic form (i.e. DPI::Parser).
 * @param[out] source
 * Source identifier that was allocated to this log source name (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for log source registration.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_register_source(const char *source_name, int *source);

/**
 * @brief Unregister a log source.
 *
 * Unregisters a given log source as part of the teardown process of the running program.
 * Currently, doesn't perform any operation.
 *
 * @param[in] source
 * The source identifier of source to be unregistered, as allocated by doca_log_register_source.
 * @return
 * DOCA_SUCCESS - always.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_unregister_source(int source);

/**
 * @brief Creates a list of loggers under a specified root logger.
 *
 * @details Allocates and populates a list of loggers that are descendants of the given root logger.
 * If `depth` is 0, all descendants are included; otherwise, only descendants up to the specified depth are included.
 * The root logger itself is not included in the list.
 *
 * @note The allocated list must be released using `doca_logger_destroy_list`.
 *
 * @param [in] logger
 * Pointer to the root logger. Pass NULL to retrieve loggers from the top-level hierarchy.
 * @param [in] depth
 * Depth of traversal:
 * - 0: include all descendants.
 * - Non-zero: include descendants up to this depth.
 * @param [out] logger_list
 * Pointer to an array of logger pointers that will be allocated by this function.
 * @param [out] nb_loggers
 * Pointer to store the number of loggers in the list.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any argument is invalid (e.g., NULL pointer).
 * - DOCA_ERROR_NOT_FOUND - `logger` is not a valid logger.
 * - DOCA_ERROR_NO_MEMORY - cannot allocate memory.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_logger_create_list(const struct doca_logger *logger,
				     uint8_t depth,
				     struct doca_logger ***logger_list,
				     uint32_t *nb_loggers);

/**
 * @brief Releases a previously allocated list of loggers.
 *
 * @param [in] logger_list
 * Pointer to the list of loggers to free.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any argument is invalid (e.g., NULL pointer).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_logger_destroy_list(struct doca_logger **logger_list);

/**
 * @brief Sets the verbosity level for a logger and optionally its descendants.
 *
 * @details If `propagate` is non-zero, the verbosity level is applied to the logger and all its descendants.
 *
 * @note If `logger` is NULL, `propagate` must be non-zero to apply the level globally.
 *
 * @param [in] logger
 * Pointer to the root logger. Pass NULL to apply the level to all top-level loggers.
 * @param [in] level
 * Verbosity level (enum DOCA_LOG_LEVEL).
 * @param [in] propagate
 * - 0: Apply to the specified logger only.
 * - Non-zero: Apply to the logger and all its descendants.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any argument is invalid (invalid `level`).
 * - DOCA_ERROR_NOT_FOUND - `logger` is not a valid logger.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_logger_set_level(struct doca_logger *logger, int level, uint8_t propagate);

/**
 * @brief Retrieves the current verbosity level of a logger.
 *
 * @param [in] logger
 * Pointer to the logger.
 * @param [out] level
 * Pointer to store the current verbosity level (enum DOCA_LOG_LEVEL).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any argument is invalid (e.g., NULL pointer).
 * - DOCA_ERROR_NOT_FOUND - `logger` is not a valid logger.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_logger_get_level(const struct doca_logger *logger, int *level);

/**
 * @brief Retrieves the name of a logger.
 *
 * @param [in] logger
 * Pointer to the logger.
 * @param [out] name
 * Buffer to store the logger name. Must be at least DOCA_LOGGER_NAME_SIZE bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any argument is invalid (e.g., NULL pointer).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_logger_get_name(const struct doca_logger *logger, char name[DOCA_LOGGER_NAME_SIZE]);

/**
 * @brief Finds a logger by its name within a list.
 *
 * @param [in] logger_list
 * Pointer to the list of loggers.
 * @param [in] name
 * Name of the logger to find.
 *
 * @return
 * Pointer to the matching logger, or NULL if not found.
 */
DOCA_EXPERIMENTAL
struct doca_logger *doca_logger_find_by_name(struct doca_logger **logger_list, const char *name);

/**
 * @brief Create a logging backend for application messages with a FILE* stream.
 *
 * Creates a new logging backend for application messages.
 *
 * @param[in] fptr
 * The FILE * for the logging backend stream.
 * @param[out] backend
 * Logging backend that wraps the given fptr (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_with_file(FILE *fptr, struct doca_log_backend **backend);

/**
 * @brief Create a logging backend for application messages with an fd stream.
 *
 * Creates a new logging backend for application messages.
 *
 * @param[in] fd
 * The file descriptor (int) for the logging backend.
 * @param[out] backend
 * Logging backend that wraps the given fd (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer or bad value for fd.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 * - DOCA_ERROR_NOT_SUPPORTED - fd isn't supported by the operating system.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_with_fd(int fd, struct doca_log_backend **backend);

/**
 * @brief Create a logging backend for application messages with a char buffer stream.
 *
 * Creates a new logging backend for application messages.
 * If the buffer's capacity is large enough to hold the log record,
 * the logging backend will write it at the beginning of this buffer and call the handler.
 *
 * @param[in] buf
 * The char buffer (char *) for the logging backend stream.
 * @param[in] capacity
 * Maximal amount of chars that could be written to the stream.
 * @param[in] handler
 * Handler to be called when the log record should be flushed from the stream.
 * @param[out] backend
 * Logging backend that wraps the given buffer (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer or capacity is zero.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_with_buf(char *buf,
					      size_t capacity,
					      log_flush_callback handler,
					      struct doca_log_backend **backend);

/**
 * @brief Create a logging backend for application messages with a syslog output.
 *
 * Creates a new logging backend for application messages.
 *
 * @param[in] name
 * The syslog name for the logging backend.
 * @param[out] backend
 * Logging backend that exposes the desired syslog functionality (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 * - DOCA_ERROR_NOT_SUPPORTED - syslog isn't supported by the operating system.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_with_syslog(const char *name, struct doca_log_backend **backend);

/**
 * @brief Get the maximum number of backends for SDK messages that can be created.
 *
 * @return
 * The maximum number of backends that can be created.
 */
DOCA_EXPERIMENTAL
uint32_t doca_log_get_max_num_backends_sdk(void);

/**
 * @brief Set the log level limit for SDK logging backends.
 *
 * Dynamically change the log level limit of the given SDK logging backend, any log under this
 * level will not be shown.
 * @param[in] backend
 * SDK logging backend to update.
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if level is an invalid verbosity level.
 * - DOCA_ERROR_NOT_PERMITTED - cannot change the log level of this backend.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_set_sdk_level(struct doca_log_backend *backend, uint32_t level);

/**
 * @brief Create a logging backend with a FILE* stream for SDK messages.
 *
 * Creates a new logging backend.
 *
 * @param[in] fptr
 * The FILE * for the logging backend stream.
 * @param[out] backend
 * Logging backend that wraps the given fptr (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_with_file_sdk(FILE *fptr, struct doca_log_backend **backend);

/**
 * @brief Create a logging backend with an fd stream for SDK messages.
 *
 * Creates a new logging backend.
 *
 * @param[in] fd
 * The file descriptor (int) for the logging backend.
 * @param[out] backend
 * Logging backend that wraps the given fd (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer or bad value for fd.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 * - DOCA_ERROR_NOT_SUPPORTED - fd isn't supported by the operating system.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_with_fd_sdk(int fd, struct doca_log_backend **backend);

/**
 * @brief Create a logging backend with a char buffer stream for SDK messages.
 *
 * Creates a new logging backend. The logging backend will write each log record at the
 * beginning of this buffer.
 *
 * @param[in] buf
 * The char buffer (char *) for the logging backend stream.
 * @param[in] capacity
 * Maximal amount of chars that could be written to the stream.
 * @param[in] handler
 * Handler to be called when the log record should be flushed from the stream.
 * @param[out] backend
 * Logging backend that wraps the given buffer (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer or capacity is zero.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_with_buf_sdk(char *buf,
						  size_t capacity,
						  log_flush_callback handler,
						  struct doca_log_backend **backend);

/**
 * @brief Create a logging backend with a syslog output for SDK messages.
 *
 * Creates a new logging backend.
 *
 * @param[in] name
 * The syslog name for the logging backend.
 * @param[out] backend
 * Logging backend that exposes the desired syslog functionality (only valid if no error occurred).
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - any of the arguments is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 * - DOCA_ERROR_NOT_SUPPORTED - syslog isn't supported by the operating system.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_with_syslog_sdk(const char *name, struct doca_log_backend **backend);

/**
 * @brief Set the log level of ALL logging backends for SDK messages.
 *
 * Dynamically change the log level of ALL the logging backends for SDK messages,
 * any SDK message with verbosity level equal or above this level will be shown.
 * Newly created logging backends for SDK messages will use this as their default log level limit.
 *
 * Default value of the level limit is DOCA_LOG_LEVEL_INFO.
 *
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if level is an invalid verbosity level.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_level_set_global_sdk_limit(uint32_t level);

/**
 * @brief Get the global log level for SDK messages.
 *
 * Dynamically query for the global log level, any SDK message with verbosity level equal or above this
 * level will be shown.
 * The global lower level is used as the initial value when a new logging backend for SDK messages is created.
 *
 * @return
 * Log level enum DOCA_LOG_LEVEL.
 */
DOCA_EXPERIMENTAL
uint32_t doca_log_level_get_global_sdk_limit(void);

/**
 * @brief Create default, non-configurable backends for application messages.
 *
 * Creates a set of 2 backends for application messages:
 * - stdout shall print the range from global lower level up to DOCA_LOG_LEVEL_INFO,
 *   which is set as the upper strict level.
 * - stderr shall print the range from DOCA_LOG_LEVEL_WARNING, which is set as the lower strict level,
 *   up to global upper level.
 *
 * @note doca_log_level_set_global_lower_limit() and doca_log_level_set_global_upper_limit() may not
 * change the values of those backends, according to the value which is being set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for backend or maximum number of
 *                          backends reached.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_create_standard(void);

/**
 * @brief Generates an application log message.
 *
 * This should not be used, please prefer using DOCA_LOG.
 *
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @param[in] source
 * The log source identifier defined by doca_log_register_source.
 * @param[in] fname
 * The file name this log originated from.
 * @param[in] line
 * The line number this log originated from.
 * @param[in] func
 * The function name this log originated from.
 * @param[in] format
 * printf(3) arguments, format and variables.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if source is an invalid id or level is an invalid verbosity level.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources for printing the message.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_log(uint32_t level, int source, const char *fname, int line, const char *func, const char *format, ...)
	__attribute__((format(printf, 6, 7)));

/**
 * @brief Generates an application log message.
 *
 * The DOCA_LOG() is the main log function for logging. This call affects the performance.
 * Consider using the specific level DOCA_LOG for better code readability (i.e. DOCA_LOG_ERR).
 *
 * @param level
 * Log level enum DOCA_LOG_LEVEL (just ERROR, WARNING...).
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG(level, format, ...) doca_log(level, log_source, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @brief Generates a CRITICAL application log message.
 *
 * Will generate critical application log. This call affects the performance.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_CRIT(format, ...) DOCA_LOG(DOCA_LOG_LEVEL_CRIT, format, ##__VA_ARGS__)

/**
 * @brief Generates an ERROR application log message.
 *
 * Will generate error application log. This call affects the performance.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_ERR(format, ...) DOCA_LOG(DOCA_LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

/**
 * @brief Generates a WARNING application log message.
 *
 * Will generate warning application log. This call affects the performance.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_WARN(format, ...) DOCA_LOG(DOCA_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)

/**
 * @brief Generates an INFO application log message.
 *
 * Will generate info application log. This call affects the performance.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_INFO(format, ...) DOCA_LOG(DOCA_LOG_LEVEL_INFO, format, ##__VA_ARGS__)

/**
 * @brief Generates a DEBUG application log message.
 *
 * Will generate debug application log. This call affects the performance.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_DBG(format, ...) DOCA_LOG(DOCA_LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

/**
 * @brief Generates a TRACE application log message.
 *
 * To show the logs define DOCA_LOGGING_ALLOW_TRACE in the compilation variables.
 * This will not effect performance if compiled without DOCA_LOGGING_ALLOW_TRACE, as
 * it will be removed by the compiler.
 *
 * Will generate trace application log. This call affects the performance.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#ifdef DOCA_LOGGING_ALLOW_TRACE
#define DOCA_LOG_TRC(format, ...) DOCA_LOG(DOCA_LOG_LEVEL_TRACE, format, ##__VA_ARGS__)
#else /* DOCA_LOGGING_ALLOW_TRACE */
#define DOCA_LOG_TRC(format, ...) \
	do { \
	} while (0)
#endif /* DOCA_LOGGING_ALLOW_TRACE */

/**
 * @brief Registers log source on program start.
 *
 * Should be used to register the log source.
 * For example:
 *
 * DOCA_LOG_REGISTER(dpi)
 *
 * void foo {
 *       DOCA_LOG_INFO("Message");
 * }
 *
 * @note The macro also takes care of the dtor() logic on teardown.
 *
 * @param source
 * A string representing the source name.
 */

#ifdef __linux__

#if defined(__INTELLISENSE__)
#pragma diag_suppress 1094 /* attribute "constructor" does not take arguments */
#endif

#define DOCA_LOG_REGISTER(source) \
	static int log_source; \
	/* Use the highest priority so other Ctors will be able to use the log */ \
	static void __attribute__((constructor(101), used)) DOCA_LOG_CTOR_##__FILE__(void) \
	{ \
		doca_log_register_source(#source, &log_source); \
	}

#else /* implicit windows */

#ifdef __cplusplus

class doca_log_registrator {
public:
	doca_log_registrator(const char *source_name, int &log_source) noexcept
	{
		doca_log_register_source(source_name, &log_source);
		m_log_source = log_source;
	}

private:
	int m_log_source{0};
};

#define DOCA_LOG_REGISTER(source) \
	static int log_source{0}; \
	static doca_log_registrator g_register_struct(#source, log_source)

#else /* __cplusplus */

/**
 * MSVC CRT Initialization, used for C based Windows applications.
 * CRT$XCU holds pointers to initializers. This is why the macro below contains a pointer to a static function in the
 * data segment.
 * \#pragma section(".CRT$XCU", read) guarantees that doca_log_register_source will be called before any compiler
 * generated C++ dynamic initializer
 */
#pragma section(".CRT$XCU", read)
#define DOCA_LOG_REGISTER(source) \
	static int log_source = 0; \
	static void _log_ctor_func(void); \
	__pragma(data_seg(".CRT$XCU")) static void (*__doca_log_initializer)() = _log_ctor_func; \
	__pragma(data_seg()) static void _log_ctor_func(void) \
	{ \
		doca_log_register_source(#source, &log_source); \
	}

#endif /* __cplusplus */

#endif /* __linux__ */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_LOG_H_ */
