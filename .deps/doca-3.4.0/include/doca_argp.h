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
 * @file doca_argp.h
 * @page doca_argp
 * @defgroup DOCA_ARGP DOCA Arg Parser
 * DOCA Arg Parser library. For more details please refer to the user guide on DOCA DevZone.
 *
 * @{
 */

#ifndef DOCA_ARGP_H_
#define DOCA_ARGP_H_

#include <stdarg.h>

#include <doca_compat.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_log.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Flag callback function type */
typedef doca_error_t (*doca_argp_param_cb_t)(void *, void *);

/** @brief Cmd callback function type */
typedef doca_error_t (*doca_argp_cmd_cb_t)(void *);

/** @brief DPDK flags callback function type */
typedef doca_error_t (*doca_argp_dpdk_cb_t)(int argc, char **argv);

/** @brief Program validation callback function type */
typedef doca_error_t (*doca_argp_validation_cb_t)(void *);

/** @brief Device open callback function type */
typedef doca_error_t (*doca_argp_dev_open_cb_t)(struct doca_devinfo *devinfo, void *usr_ctx, struct doca_dev **dev);

/** @brief (Operational) Logger callback function type */
typedef doca_error_t (*doca_argp_logger_cb_t)(enum doca_log_level level, const char *format, va_list args);

/**
 * @brief Flag input type
 */
enum doca_argp_type {
	DOCA_ARGP_TYPE_UNKNOWN = 0,
	DOCA_ARGP_TYPE_STRING,	   /**< Input type is a string */
	DOCA_ARGP_TYPE_INT,	   /**< Input type is an integer */
	DOCA_ARGP_TYPE_BOOLEAN,	   /**< Input type is a boolean */
	DOCA_ARGP_TYPE_DEVICE,	   /**< Input type is a DOCA device */
	DOCA_ARGP_TYPE_DEVICE_REP, /**< Input type is a DOCA device representor */
	DOCA_ARGP_TYPE_DOUBLE,	   /**< Input type is a double */
};

/**
 * @brief Program flag information
 *
 * @note It is the programmer's responsibility to ensure the callback will copy the content of the param passed to it.
 * The pointer pointing to the param is owned by doca_argp, and it is only valid in the scope of the called callback.
 */
struct doca_argp_param;

/**
 * @brief Program command information
 */
struct doca_argp_cmd;

/**
 * @brief Device context, possibly with additional device arguments
 */
struct doca_argp_device_ctx {
	struct doca_dev *dev; /**< Pointer to the device context */
	const char *devargs;  /**< Pointer to the device arguments (if present) */
};

/**
 * @brief Device representor context, possibly with additional device arguments
 */
struct doca_argp_device_rep_ctx {
	struct doca_argp_device_ctx dev_ctx; /**< The device context for which we have a representor */
	struct doca_dev_rep *dev_rep;	     /**< Pointer to the device representor context */
};

/**
 * @brief Print usage instructions.
 */
DOCA_EXPERIMENTAL
void doca_argp_usage(void);

/**
 * @brief Initialize the parser interface.
 *
 * @param [in] program_name
 * (Optional) Name of current program, used during the usage print.
 * @param [in] program_config
 * Program configuration struct.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module was already initialized earlier.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate enough space.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * @note After a successful call to this function, one must also invoke doca_argp_destroy() during the program cleanup.
 * @note If no program name is passed, name will be extracted from argv as passed to doca_argp_start().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_init(const char *program_name, void *program_config);

/**
 * @brief Register a program flag.
 *
 * @param [in] input_param
 * Program flag details.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed, or param was already registered.
 * - DOCA_ERROR_INITIALIZATION - received param with missing mandatory fields.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * @note Value of is_cli_only field may be changed in this function.
 * @note ARGP takes ownership of the pointer in ALL flows, and is responsible for later destroying it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_register_param(struct doca_argp_param *input_param);

/**
 * @brief Register a program command.
 *
 * @param [in] input_cmd
 * Program cmd/mode details.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed, or cmd was already registered.
 * - DOCA_ERROR_INITIALIZATION - received command with missing mandatory fields initialization.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * @note ARGP takes ownership of the pointer in ALL flows, and is responsible for later destroying it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_register_cmd(struct doca_argp_cmd *input_cmd);

/**
 * @brief Register an alternative version callback.
 *
 * @param [in] callback
 * Program-specific version callback.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * @note: When calling the version callback, if the program will exit, should ensure that doca_argp_destroy is
 * called to clean up doca_argp before exiting
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_register_version_callback(doca_argp_param_cb_t callback);

/**
 * @brief Register program validation callback function.
 *
 * @param [in] callback
 * Program validation callback.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * @note Validation callback will be invoked with a single argument, which is the program's configuration struct.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_register_validation_callback(doca_argp_validation_cb_t callback);

/**
 * @brief Register a device open callback.
 *
 * @param [in] callback
 * Device open callback.
 * @param [in] usr_ctx
 * User context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * @note The callback will be invoked per DOCA device, and user is responsible for the teardown of the device context.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_register_dev_open_callback(doca_argp_dev_open_cb_t callback, void *usr_ctx);

/**
 * @brief Register a logger callback to be used for user-facing events.
 *
 * @param [in] callback logging callback for operational log events.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed.
 * - DOCA_ERROR_ALREADY_EXIST - callback was already registered earlier.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_register_logger_callback(doca_argp_logger_cb_t callback);

/**
 * @brief Disable a previously registered logger callback, reverting back to default stderr backend.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized, was already destroyed or no callback was registered.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_disable_logger_callback(void);

/**
 * @brief Parse incoming arguments (cmd line/json).
 *
 * @param [in] argc
 * Number of program command line arguments.
 * @param [in] argv
 * Program command line arguments.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed.
 * - DOCA_ERROR_INITIALIZATION - initialization error.
 * - DOCA_ERROR_IO_FAILED - internal errors about JSON API, reading JSON content.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input
 * - DOCA_ERROR_NO_MEMORY - failed to allocate enough space.
 * - DOCA_ERROR_NOT_SUPPORTED - received unsupported program flag.
 * @note: if the program is based on DPDK API, DPDK flags will be forwarded to it by calling the registered callback.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_start(int argc, char **argv);

/**
 * @brief ARG Parser destroy.
 *
 * Cleanup all resources, including the parsed DPDK flags. Once called, the module is no
 * longer usable.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_destroy(void);

/**
 * @brief Mark the program as based on DPDK API.
 *
 * @param [in] callback
 * Once ARGP finishes parsing, the DPDK flags will be forwarded to the program through this callback.
 *
 * @note doca_argp_init must be invoked before invoking this function.
 * @note If the program is based on DPDK API, DPDK flags array will be sent using the callback,
 * and the array will be released when calling doca_argp_destroy.
 */
DOCA_EXPERIMENTAL
void doca_argp_set_dpdk_program(doca_argp_dpdk_cb_t callback);

/**
 * @brief Create new program param.
 *
 * @param [out] param
 * Created program param instance. Valid only on success.
 *
 * @note Param fields should be set through the setter functions.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate enough space.
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_param_create(struct doca_argp_param **param);

/**
 * @brief Set the short name of the program param.
 *
 * @param [in] param
 * The program param.
 * @param [in] name
 * The param's short name
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note At least one of param name (short/long) must be set.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_short_name(struct doca_argp_param *param, const char *name);

/**
 * @brief Set the long name of the program param.
 *
 * @param [in] param
 * The program param.
 * @param [in] name
 * The param's long name.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note At least one of param name (short/long) must be set.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_long_name(struct doca_argp_param *param, const char *name);

/**
 * @brief Set the description of the expected arguments of the program param, used during program usage
 *
 * @param [in] param
 * The program param.
 * @param [in] arguments
 * The param's arguments.
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_arguments(struct doca_argp_param *param, const char *arguments);

/**
 * @brief Set the description of the program param, used during program usage.
 *
 * @param [in] param
 * The program param.
 * @param [in] description
 * The param's description.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Setting the param description is mandatory.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_description(struct doca_argp_param *param, const char *description);

/**
 * @brief Set the callback function of the program param.
 *
 * @param [in] param
 * The program param.
 * @param [in] callback
 * The param's callback function.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Once ARGP identifies this param in CLI, it will call the callback function with the param argument value
 * as first argument followed by the program configuration struct. Program must copy the argument value and shouldn't
 * use it directly once the callback finished.
 * @note Setting the param callback is mandatory.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_callback(struct doca_argp_param *param, doca_argp_param_cb_t callback);

/**
 * @brief Set the type of the param arguments.
 *
 * @param [in] param
 * The program param.
 * @param [in] type
 * The param arguments type.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Setting the param arguments type is mandatory.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_type(struct doca_argp_param *param, enum doca_argp_type type);

/**
 * @brief Mark the program param as mandatory.
 *
 * @param [in] param
 * The program param.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Parameters are optional by default.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_mandatory(struct doca_argp_param *param);

/**
 * @brief Mark the program param as supported only in CLI mode and unavailable through a JSON file.
 *
 * @param [in] param
 * The program param.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Parameters are by default available in both modes, as long as they have a long name.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_cli_only(struct doca_argp_param *param);

/**
 * @brief Mark the program param as supporting multiple appearances.
 *
 * @param [in] param
 * The program param.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Since JSON file doesn't support keys multiplicity, the multi values will be expected to be in
 * an array, and param argument type will indicate the values type.
 * @note Parameters can only be used once by default.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_multiplicity(struct doca_argp_param *param);

/**
 * @brief Mark the program param as singular.
 *
 * @param [in] param
 * The program param.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Singular parameters must not be passed alongside any other parameter.
 * @note Parameters can be used alongside others by default.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_singular(struct doca_argp_param *param);

/**
 * @brief Mark the program param as inherited through the command chain.
 *
 * @param [in] param
 * The program param.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Parameters are only valid for the current command by default.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_inherited(struct doca_argp_param *param);

/**
 * @brief Mark the program param as hidden.
 *
 * @param [in] param
 * The program param.
 *
 * @note Passing a "param" value of NULL will result in an undefined behavior.
 * @note Parameters are only valid for the current command by default.
 */
DOCA_EXPERIMENTAL
void doca_argp_param_set_hidden(struct doca_argp_param *param);

/**
 * @brief Destroy a program param.
 *
 * @param [in] param
 * The program param to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - parameter was already registered.
 * - DOCA_ERROR_INVALID_VALUE - invalid input received.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_param_destroy(struct doca_argp_param *param);

/**
 * @brief Create new program command.
 *
 * @param [out] cmd
 * Create program command instance on success. Valid on success only.
 *
 * @note Need to set command fields by setter functions.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate enough space.
 * - DOCA_ERROR_BAD_STATE - module wasn't yet initialized or was already destroyed.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_cmd_create(struct doca_argp_cmd **cmd);

/**
 * @brief Set the name of the program command.
 *
 * @param [in] cmd
 * The program command.
 * @param [in] name
 * The command's name.
 *
 * @note Passing a "cmd" value of NULL will result in an undefined behavior.
 * @note Setting the command name is mandatory.
 */
DOCA_EXPERIMENTAL
void doca_argp_cmd_set_name(struct doca_argp_cmd *cmd, const char *name);

/**
 * @brief Set the description of the program command, used during the program usage.
 *
 * @param [in] cmd
 * The program command.
 * @param [in] description
 * The command's description.
 *
 * @note Passing a "cmd" value of NULL will result in an undefined behavior.
 * @note Setting the command description is mandatory.
 */
DOCA_EXPERIMENTAL
void doca_argp_cmd_set_description(struct doca_argp_cmd *cmd, const char *description);

/**
 * @brief Set the callback function of the program command.
 *
 * @param [in] cmd
 * The program command.
 * @param [in] callback
 * The command's callback function.
 *
 * @note Passing a "cmd" value of NULL will result in an undefined behavior.
 * @note Once ARGP identifies this command in CLI, it will call the callback function with the program
 * configuration struct.
 * @note Providing a callback is optional, and is recommended for the leaves in the cmd tree.
 */
DOCA_EXPERIMENTAL
void doca_argp_cmd_set_callback(struct doca_argp_cmd *cmd, doca_argp_cmd_cb_t callback);

/**
 * @brief Mark the program command as hidden.
 *
 * @param [in] cmd
 * The program command.
 *
 * @note Passing a "cmd" value of NULL will result in an undefined behavior.
 */
DOCA_EXPERIMENTAL
void doca_argp_cmd_set_hidden(struct doca_argp_cmd *cmd);

/**
 * @brief Register a program flag for the given program command.
 *
 * @param [in] cmd
 * The program command.
 * @param [in] input_param
 * Program flag details.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - command or parameter were already registered.
 * - DOCA_ERROR_INITIALIZATION - received param with missing mandatory fields.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * @note Value of is_cli_only field may be changed in this function.
 * @note ARGP takes ownership of the pointer in ALL flows, and is responsible for later destroying it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_cmd_register_param(struct doca_argp_cmd *cmd, struct doca_argp_param *input_param);

/**
 * @brief Register an inner program command for the given program command.
 *
 * @param [in] cmd
 * The program command.
 * @param [in] input_cmd
 * Inner program command details.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - one of the commands was already registered.
 * - DOCA_ERROR_INITIALIZATION - received command with missing mandatory fields initialization.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * @note ARGP takes ownership of the pointer in ALL flows, and is responsible for later destroying it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_cmd_register_cmd(struct doca_argp_cmd *cmd, struct doca_argp_cmd *input_cmd);

/**
 * @brief Destroy a program command.
 *
 * @param [in] cmd
 * The program command to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - command was already registered.
 * - DOCA_ERROR_INVALID_VALUE - invalid input received.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_cmd_destroy(struct doca_argp_cmd *cmd);

/**
 * @brief Get the log level the user inserted it.
 *
 * @param [out] log_level
 * The log level if passed by the user, otherwise the global value of the program.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - invalid input received.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_get_log_level(int *log_level);

/**
 * @brief Get the SDK log level as passed by the user.
 *
 * @param [out] log_level
 * The log level if passed by the user, otherwise the global value of the program.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - invalid input received.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_get_sdk_log_level(int *log_level);

/**
 * @brief Disable JSON file pedantic arguments check, unknown fields will be ignored.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - invalid argp state.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_argp_disable_pedantic_config_parsing(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_ARGP_H_ */
