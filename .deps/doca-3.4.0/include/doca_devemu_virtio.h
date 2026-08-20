/*
 * Copyright (c) 2022-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_devemu_virtio.h
 * @page doca_devemu_virtio
 * @defgroup DOCA_DEVEMU_VIRTIO DOCA Device Emulation - Virtio Devices
 * @ingroup DOCA_DEVEMU
 *
 * DOCA library for emulated virtio devices logic
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VIRTIO_H_
#define DOCA_DEVEMU_VIRTIO_H_

#include <stdint.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing emulated Virtio pci device.
 * This structure extends the pci core doca_devemu_pci_dev structure.
 * This structure is used by Virtio device emulation applications, libraries and services.
 */
struct doca_devemu_virtio_dev;

/**
 * @brief Opaque structure representing emulated Virtio pci device type.
 * This structure extends the pci core doca_devemu_pci_type structure.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_virtio_type;

/**
 * @brief Opaque structure representing emulated Virtio pci device IO context.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_virtio_io;

/**
 * @brief Opaque structure representing debug state for an emulated Virtio PCI device queue.
 * This structure is used by Virtio device emulation applications, libraries and services.
 */
struct doca_devemu_virtio_queue_dbg_state;

/**
 * @brief Opaque structure representing emulated Virtio device offload engine.
 * This structure is used by Virtio device emulation applications and services.
 */
struct doca_devemu_virtio_offload_engine;

/**
 * @brief Opaque structure representing Virtio virtqueue.
 * This structure is used by Virtio device emulation applications and services.
 */
struct doca_devemu_virtio_vq;

/*********************************************************************************************************************
 * DOCA devemu Virtio device Properties
 *********************************************************************************************************************/

/**
 * @brief Get the Virtio device_feature bits (0-63) according to Virtio specification.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] features
 * The device_feature (bits 0-63) according to Virtio specification.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_device_features_63_0(const struct doca_devemu_virtio_dev *virtio_dev,
		uint64_t *features);

/**
 * @brief Set the Virtio device_feature bits (0-63) according to Virtio specification.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to modify.
 * @param [in] features
 * The device_feature (bits 0-63) according to Virtio specification.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_set_device_features_63_0(struct doca_devemu_virtio_dev *virtio_dev,
		uint64_t features);

/**
 * @brief Get the Virtio driver_feature bits (0-63) according to Virtio specification.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] features
 * The driver_feature (bits 0-63) according to Virtio specification.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_driver_features_63_0(const struct doca_devemu_virtio_dev *virtio_dev,
		uint64_t *features);

/**
 * @brief Get the Virtio config_msix_vector register according to Virtio specification.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] config_msix_vector
 * The value of the config_msix_vector register according to Virtio specification.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_config_msix_vector(const struct doca_devemu_virtio_dev *virtio_dev,
		uint16_t *config_msix_vector);

/**
 * @brief Get the Virtio device num_queues register from common configuration structure according to Virtio specification.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] num_queues
 * The value of the num_queues register.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_num_queues(const struct doca_devemu_virtio_dev *virtio_dev,
		uint16_t *num_queues);

/**
 * @brief Set the Virtio device num_queues register in common configuration structure according to Virtio specification.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to modify.
 * @param [in] num_queues
 * The device common num_queues register.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_set_num_queues(struct doca_devemu_virtio_dev *virtio_dev, uint16_t num_queues);

/**
 * @brief Get the number of enabled Virtio device queues by the driver. The driver enables a queue by setting the
 * corresponding queue index to the queue_select register and setting the queue_enable register to 1. The return value
 * of num_queues is valid only if DRIVER_OK status bit was set by the driver.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] num_queues
 * The number of enable Virtio queues for the virtio device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_num_enabled_queues(const struct doca_devemu_virtio_dev *virtio_dev,
		uint16_t *num_queues);

/**
 * @brief Get the Virtio device_status register from common configuration structure according to Virtio specification.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] device_status
 * The value of the device_status register.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_device_status(const struct doca_devemu_virtio_dev *virtio_dev,
		uint8_t *device_status);

/**
 * @brief Get the Virtio config_generation register from common configuration structure according to Virtio specification.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] config_generation
 * The value of the config_generation register.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_config_generation(const struct doca_devemu_virtio_dev *virtio_dev,
		uint8_t *config_generation);

/**
 * @brief Get the Virtio max queue size for all Virtio queues.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] queue_size
 * The maximal queue size for all Virtio queues.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_queue_size(const struct doca_devemu_virtio_dev *virtio_dev,
		uint16_t *queue_size);

/**
 * @brief Set the Virtio max queue size for all Virtio queues.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [in] queue_size
 * The maximal queue size for all Virtio queues.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_set_queue_size(struct doca_devemu_virtio_dev *virtio_dev, uint16_t queue_size);

/**
 * @brief Get the number of required running Virtio io context's to be bounded to the Virtio device context. The Virtio
 * device context will not move to a "running" state before having this amount of running Virtio IO context's bounded
 * to it.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to query.
 * @param [out] num_virtio_io
 * The number of required running Virtio IO ctx's to be bounded to the device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_get_num_required_running_virtio_io_ctxs(const struct doca_devemu_virtio_dev *virtio_dev,
		uint32_t *num_virtio_io);

/**
 * @brief Set the number of required running Virtio IO context's to be bounded to the Virtio device context. The Virtio
 * device context will not move to a "running" state before having this amount of running Virtio IO context's bounded
 * to it.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio device instance to modify.
 * @param [in] num_virtio_io
 * The number of required running Virtio IO ctx's to be bounded.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_set_num_required_running_virtio_io_ctxs(struct doca_devemu_virtio_dev *virtio_dev,
		uint32_t num_virtio_io);

/**
 * @brief Complete the Virtio device reset handling. Prior to calling this function, the user must ensure that all the
 * resources associated with the Virtio device are flushed back to the ownership of the device.
 *
 * @param [in] virtio_dev
 * DOCA Virtio device instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_reset_complete(struct doca_devemu_virtio_dev *virtio_dev);

/**
 * @brief Convert DOCA Virtio device instance into DOCA context.
 *
 * @param [in] virtio_dev
 * DOCA Virtio device instance. This must remain valid until after the DOCA context is no longer required.
 *
 * @return
 * doca ctx upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_devemu_virtio_dev_as_ctx(struct doca_devemu_virtio_dev *virtio_dev);

/**
 * @brief Convert DOCA Virtio device instance into DOCA devemu PCI device.
 *
 * @param [in] virtio_dev
 * DOCA Virtio device instance. This must remain valid until after the DOCA devemu PCI device is no longer required.
 *
 * @return
 * DOCA devemu pci device upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_pci_dev *doca_devemu_virtio_dev_as_pci_dev(struct doca_devemu_virtio_dev *virtio_dev);

/*********************************************************************************************************************
 * DOCA devemu Virtio device context events API
 *********************************************************************************************************************/

/**
 * @brief Function to be executed on Virtio device reset. The event handler will enable users to quiesce, flush and
 * reset the necessary resources associated with the emulated Virtio device.
 * Upon event, all PCI I/O transactions to/from the host memory are disabled.
 * Additionally, the user should flush all the outstanding resources associated with the emulated Virtio device, which
 * were initially owned by the Virtio device and moved the ownership of the user. After flushing all the
 * outstanding resources, the user should call doca_devemu_virtio_dev_reset_complete().
 *
 * @param [in] virtio_dev
 * DOCA Virtio device instance.
 * @param [in] event_user_data
 * Same user data that was provided in doca_devemu_virtio_dev_event_reset_register().
 *
 */
typedef void (*doca_devemu_virtio_dev_event_reset_handler_cb_t)(struct doca_devemu_virtio_dev *virtio_dev,
								union doca_data event_user_data);

/**
 * @brief Register to Virtio device reset event.
 *
 * Registration can be done only if the Virtio device ctx is idle. If called multiple times then only the last call
 * will take effect.
 *
 * @param [in] virtio_dev
 * The DOCA Virtio dev context to be associated to the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 * @param [in] user_data
 * User data that will be provided to the handler once invoked.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'virtio_dev' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - virtio_dev context is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_dev_event_reset_register(struct doca_devemu_virtio_dev *virtio_dev,
		doca_devemu_virtio_dev_event_reset_handler_cb_t handler,
		union doca_data user_data);

/*********************************************************************************************************************
 * DOCA devemu Virtio device debug state
 *********************************************************************************************************************/

/**
 * @brief Function to be executed upon finishing asynchronous population of Virtio queue debug state list.
 *
 * @param [in] dbg_list
 * Populated list of Virtio queue debug state structures.
 * @param [in] user_data
 * Same user data that was provided in doca_devemu_virtio_queue_dbg_state_create_list().
 * @param [in] err
 * DOCA_SUCCESS - in case of successful population. Error code - in case of failure.
 */
typedef void (*doca_devemu_virtio_queue_dbg_state_list_populate_done_cb_t)(struct doca_devemu_virtio_queue_dbg_state **dbg_list,
                union doca_data user_data, doca_error_t err);

/**
 * @brief Create an empty list of debug state for Virtio device queues.
 *
 * @details Allocates an unpopulated list (array) of queue debug state structures for the queues of a Virtio device.
 *
 * @param [in] virtio_dev
 * The DOCA devemu Virtio device. Must be started.
 * @param [in] done
 * The callback function which is optional. If provided, it will be invoked upon finishing population of Virtio queue
 * debug state list.
 * If no callback is given, the user must periodically query using doca_devemu_virtio_queue_dbg_state_is_populated() to determine
 * when the debug list population is finished.
 * @param [in] user_data
 * User data that will be provided to the done callback once invoked.
 * @param [out] dbg_list
 * The newly created list of DOCA devemu Virtio queue debug state structures. After a successful call, the list can be
 * accessed as (*dbg_list)[idx], where idx ranges from 0 to (*list_len) - 1.
 * @param [out] list_len
 * The length of the dbg_list.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'virtio_dev', 'dbg_list' or 'list_len' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'virtio_dev' is not in running state.
 * - DOCA_ERROR_NOT_PERMITTED - If 'virtio_dev' has no available queues for which debug state can be populated.
 * - DOCA_ERROR_NO_MEMORY - Failed to allocate internal resources.
 * @note The returned dbg_list should be populated using doca_devemu_virtio_queue_dbg_state_populate_list(). The returned
 * dbg_list must be deallocated using doca_devemu_virtio_queue_dbg_state_destroy_list() to prevent memory leaks.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_create_list(struct doca_devemu_virtio_dev *virtio_dev,
		doca_devemu_virtio_queue_dbg_state_list_populate_done_cb_t done, union doca_data user_data,
		struct doca_devemu_virtio_queue_dbg_state ***dbg_list, uint32_t *list_len);

/**
 * @brief Destroy a list of Virtio queue debug state structures.
 *
 * @details Deallocates the memory associated with a list of Virtio queue debug state structures.
 *
 * @param [in] dbg_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_destroy_list(struct doca_devemu_virtio_queue_dbg_state **dbg_list);

/**
 * @brief Issue an asynchronous population of a list of Virtio queue debug state structures.
 *
 * @details Initiates the asynchronous population of a list of Virtio queue debug state structures. Only one population
 * operation can be in progress at any given time for either the associated DOCA DevEmu Virtio device, which must be
 * started, or the associated Virtio offload engine, which must be enabled.
 * The callback provided in doca_devemu_virtio_queue_dbg_state_create_list() will be invoked upon finishing population. If a callback
 * wasn't given in doca_devemu_virtio_queue_dbg_state_create_list(), user need query periodically if the list finished its population
 * using doca_devemu_virtio_queue_dbg_state_is_populated().
 *
 * @param [in] dbg_list
 * The list to be populated.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'dbg_list' is NULL.
 * - DOCA_ERROR_IN_PROGRESS - If population is already in progress.
 * - DOCA_ERROR_BAD_STATE - If the associated Virtio device is not started, or the associated Virtio offload engine is not enabled.
 *   In this case, the 'dbg_list' should be destroyed using doca_devemu_virtio_queue_dbg_state_destroy_list() and re-created at a
 *   later stage.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_populate_list(struct doca_devemu_virtio_queue_dbg_state **dbg_list);

/**
 * @brief Checks if the data for a debug list was populated.
 *
 * @details Checks for a Virtio queue debug state list if it was populated and if all the required data
 * was already updated in it for all the VQs.
 *
 * @param [in] dbg_list
 * The list to check.
 * @param [out] is_populated
 * 1 if the list is populated, 0 otherwise. This field is valid only if the function returned DOCA_SUCCESS.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'dbg_list' or 'is_populated' is NULL.
 * - DOCA_ERROR_BAD_STATE - If during population, the associated virtio_dev is no longer started or
 *   the associated offload_engine is no longer enabled. In this case, 'dbg_list' should be destroyed
 *   using doca_devemu_virtio_queue_dbg_state_destroy_list() and re-created at a later stage.
 * - DOCA_ERROR_NOT_PERMITTED - If there is no population process going on.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_is_populated(struct doca_devemu_virtio_queue_dbg_state **dbg_list, uint8_t *is_populated);

/**
 * @brief Retrieve the Virtio queue index from its debug state structure.
 *
 * @param [in] state
 * The DOCA devemu Virtio queue debug state instance to query. It must have been previously successfully populated.
 * @param [out] id
 * The index of the Virtio queue.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'stats' or 'id' is NULL
 * - DOCA_ERROR_BAD_STATE - 'stats' is not populated successfully
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_get_id(struct doca_devemu_virtio_queue_dbg_state *state, uint16_t *id);

/**
 * @brief Retrieve the Virtio queue size (depth) from its debug state structure.
 *
 * @param [in] state
 * The DOCA devemu Virtio queue debug state instance to query. It must have been previously successfully populated.
 * @param [out] size
 * The size of the Virtio queue.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'stats' or 'size' is NULL
 * - DOCA_ERROR_BAD_STATE - 'stats' is not populated successfully
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_get_size(struct doca_devemu_virtio_queue_dbg_state *state, uint16_t *size);

/**
 * @brief Retrieve the number of in-flight requests for a Virtio queue from its debug state structure.
 *
 * @param [in] state
 * The DOCA devemu Virtio queue debug state instance to query. It must have been previously successfully populated.
 * @param [out] inflights
 * The amount of in-flight requests associated with Virtio queue.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'stats' or 'inflights' is NULL
 * - DOCA_ERROR_BAD_STATE - 'stats' is not populated successfully
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_get_inflights(struct doca_devemu_virtio_queue_dbg_state *state,
		uint16_t *inflights);

/**
 * @brief Retrieve the available index of a Virtio queue as seen by the device from its debug state structure.
 *
 * @param [in] state
 * The DOCA devemu Virtio queue debug state instance to query. It must have been previously successfully populated.
 * @param [out] idx
 * The available index of a Virtio queue as seen by the device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'stats' or 'idx' is NULL
 * - DOCA_ERROR_BAD_STATE - 'stats' is not populated successfully
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_get_hw_avail_idx(struct doca_devemu_virtio_queue_dbg_state *state,
		uint16_t *idx);

/**
 * @brief Retrieve the available index of a Virtio queue as seen by the driver from its debug state structure.
 *
 * @param [in] state
 * The DOCA devemu Virtio queue debug state instance to query. It must have been previously successfully populated.
 * @param [out] idx
 * The available index of a Virtio queue as seen by the driver.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'stats' or 'idx' is NULL
 * - DOCA_ERROR_BAD_STATE - 'stats' is not populated successfully
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_get_driver_avail_idx(struct doca_devemu_virtio_queue_dbg_state *state, uint16_t *idx);

/**
 * @brief Retrieve the used index of a Virtio queue as seen by the device from its debug state structure.
 *
 * @param [in] state
 * The DOCA devemu Virtio queue debug state instance to query. It must have been previously successfully populated.
 * @param [out] idx
 * The used index of a Virtio queue as seen by the device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'stats' or 'idx' is NULL
 * - DOCA_ERROR_BAD_STATE - 'stats' is not populated successfully
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_get_hw_used_idx(struct doca_devemu_virtio_queue_dbg_state *state, uint16_t *idx);

/**
 * @brief Retrieve the used index of a Virtio queue as seen by the driver from its debug state structure.
 *
 * @param [in] state
 * The DOCA devemu Virtio queue debug state instance to query. It must have been previously successfully populated.
 * @param [out] idx
 * The used index of a Virtio queue as seen by the driver.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'stats' or 'idx' is NULL
 * - DOCA_ERROR_BAD_STATE - 'stats' is not populated successfully
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_get_driver_used_idx(struct doca_devemu_virtio_queue_dbg_state *state, uint16_t *idx);

/**
 * @brief Retrieve the enabled status of a Virtio queue from its debug state structure.
 *
 * @param [in] state
 * The DOCA devemu Virtio queue debug state instance to query. It must have been previously successfully populated.
 * @param [out] enabled
 * 1 if the Virtio queue is enabled, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'stats' or 'enabled' is NULL
 * - DOCA_ERROR_BAD_STATE - 'stats' is not populated successfully
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_queue_dbg_state_get_enabled(struct doca_devemu_virtio_queue_dbg_state *state, uint8_t *enabled);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VIRTIO_H_ */
