/*
 * Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_devemu_virtio_tlp.h
 * @page doca_devemu_virtio_tlp
 * @defgroup DOCA_DEVEMU_VIRTIO_TLP DOCA Device Emulation - Virtio TLP
 * @ingroup DOCA_DEVEMU_VIRTIO
 *
 * DOCA VIRTIO TLP context
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VIRTIO_TLP_H_
#define DOCA_DEVEMU_VIRTIO_TLP_H_

#include <stdint.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>

#ifdef __cplusplus
extern "C" {
#endif


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
 * DOCA devemu Virtio offload engine API
 *********************************************************************************************************************/

/**
 * @brief Start a Virtio offload engine.
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to start.
 *
 * @details Once started, the Virtio offload engine begins processing operations from the driver when enabled
 * via doca_devemu_virtio_offload_engine_enable().
 * It finishes processing operations when all associated virtqueues have been disabled.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL
 * - DOCA_ERROR_BAD_STATE - If 'offload' is already started.
 * - DOCA_ERROR_NO_MEMORY - Failed to allocate internal resources
 * @note This method upon success disable the ability to configure the DOCA devemu Virtio offload engine.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_start(struct doca_devemu_virtio_offload_engine *offload);

/**
 * @brief Stop a Virtio offload engine.
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to stop. Must be disabled and must not be associated to any Virtio virtqueue or
 * Virtio IO context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL
 * - DOCA_ERROR_BAD_STATE - If 'offload' is already stopped.
 * - DOCA_ERROR_NOT_PERMITTED - If 'offload' is enabled or is associated with a Virtio virtqueue or Virtio IO
 *   context.
 * @note This method upon success re-enable the ability to configure the DOCA devemu Virtio offload engine.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_stop(struct doca_devemu_virtio_offload_engine *offload);

/**
 * @brief This enum defines the states of operation for a started virtio offload engine.
 *
 *
 * @code
 * State machine diagram:
 *
 *     All in-flight requests across all associated virtqueues are drained or flushed (no processing of requests)
 *                            +-----------+
 *                            |           |
 *   +----------------------->| disabled  |<---------------------------------------------------------------------+
 *   |                        |           |                                                                      |
 *   |                        +-----+-----+                                                                      |
 *   |                            |                                                                              |
 *   |                            | doca_devemu_virtio_offload_engine_enable() has been called                   |
 *   |                            |                                                                              |
 *   |                            |                                                                              |
 *   |                            |                                                                              |
 *   |                            |                                                                              |
 *   |                            V                                                                              |
 *   |                       +------------+                                                                      |
 *   |                       |            |                                                                      |
 *   |                       | enabled    |<----------------------------------------------+                      |
 *   |                       |            |                                               |                      |
 *   |                       +-----+------+                                               |                      |
 *   |                            |                                                       |                      |
 *   |                            | doca_devemu_virtio_offload_engine_enable() has been   |                      |
 *   |                            | called while ENABLED                                  |                      |
 *   |                            |                                                       |                      |
 *   |                            |                                                       |                      |
 *   |                            +-------------------------------------------------------+                      |
 *   |                            |                                                                              |
 *   |                            | doca_devemu_virtio_offload_engine_disable() has been called                  |
 *   |                            |                                                                              |
 *   |                            |                                                                              |
 *   |                            | Synchronous: upon doca_devemu_virtio_offload_engine_disable() call,          |
 *   |                            |              change state to disabled and return DOCA_SUCCESS                |
 *   |                            |                                                                              |
 *   |                            +------------------------------------------------------------------------------+
 *   |                            |
 *   |                            | Asynchronous: upon doca_devemu_virtio_offload_engine_disable() call,
 *   |                            |               change state to disabling and return DOCA_ERROR_IN_PROGRESS
 *   |                            |
 *   |                            v
 *   |                       +------------+
 *   |                       |            |
 *   |-----------------------+ disabling  |
 *                           |            |
 *                           +------------+
 * @endcode
 *
 */
enum doca_devemu_virtio_offload_engine_states {
	/**
	 * The initial state of the offload engine after starting.
	 * Indicates that doca_devemu_virtio_offload_engine_disable() has been called, and all associated and started
	 * virtqueues are disabled.
	 * While the engine is in this state, the following cannot be called:
	 * 1. doca_devemu_virtio_vq_enable()
	 * 2. doca_devemu_virtio_vq_disable()
	 * 3. doca_devemu_virtio_vq_group_enable()
	 * 4. doca_devemu_virtio_vq_group_disable()
	 */
	DOCA_DEVEMU_VIRTIO_OFFLOAD_ENGINE_STATE_DISABLED = 0,
	/**
	 * Indicates that doca_devemu_virtio_offload_engine_enable() has been called, and all the associated, started
	 * virtqueues that were previously in the DISABLED state are now moved to the ENABLED state.
	 *
	 * In this state, the following may be called:
	 * 1. doca_devemu_virtio_vq_enable()
	 * 2. doca_devemu_virtio_vq_disable()
	 * 3. doca_devemu_virtio_vq_group_enable()
	 * 4. doca_devemu_virtio_vq_group_disable()
	 */
	DOCA_DEVEMU_VIRTIO_OFFLOAD_ENGINE_STATE_ENABLED,
	/*
	 * Indicates that doca_devemu_virtio_offload_engine_disable() has been called, and the offload engine is
	 * asynchronously transitioning to DISABLED state.
	 * None of the associated virtqueues is enabled, and at least one is in the process of disabling.
	 * The application should call doca_devemu_virtio_offload_engine_get_state() to verify whether the engine has
	 * reached the DISABLED state.
	 * While the engine is in this state, the following cannot be called:
	 * 1. doca_devemu_virtio_vq_enable()
	 * 2. doca_devemu_virtio_vq_disable()
	 * 3. doca_devemu_virtio_vq_group_enable()
	 * 4. doca_devemu_virtio_vq_group_disable()
	 */
	DOCA_DEVEMU_VIRTIO_OFFLOAD_ENGINE_STATE_DISABLING,
};

/**
 * @brief Enable a Virtio offload engine.
 *
 * @details Upon success, this function initiates processing of operations between the driver and all associated,
 * started and disabled virtqueues.
 * This operation is not permitted if the Virtio offload engine is in the DISABLING state.
 * If the engine is already in the ENABLED state, calling this function will enable all associated virtqueues that are
 * started but currently disabled.
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to enable. Must be started and not in the DISABLING state.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL.
 * - DOCA_ERROR_NOT_PERMITTED - If 'offload' is in DISABLING state.
 * - DOCA_ERROR_BAD_STATE - If 'offload' is not started
 * @note During a handover, applications must coordinate the roles configured on the source and destination engines.
 * For each handover pair, at most one engine can be configured as enabled. It is allowed for both engines to be
 * configured as disabled. If both engines are configured as enabled, the behavior is undefined.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_enable(struct doca_devemu_virtio_offload_engine *offload);

/**
 * @brief Disable a Virtio offload engine.
 *
 * @details If this method returns DOCA_SUCCESS, all associated and started virtqueues are disabled (engine disablement
 * is synchronous).
 * If this method returns DOCA_ERROR_IN_PROGRESS, none of the associated virtqueues is enabled, and at least one is in
 * the process of disabling. The engine is transitioning asynchronously to the DISABLED state once all in-flight
 * operations have finished. To verify, use doca_devemu_virtio_offload_engine_get_state().
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to disable. Must be started and enabled.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_IN_PROGRESS - If engine is transitioning to DISABLED (some virtqueues are still disabling).
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'offload' is not enabled.
 * - DOCA_ERROR_NOT_PERMITTED - If 'offload' is not started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_disable(struct doca_devemu_virtio_offload_engine *offload);

/**
 * @brief Get the state of operation for a started Virtio offload engine
 *
 * @param [in] offload
 * The DOCA Virtio offload engine for which to retrieve the state. Must be started.
 * @param [out] state
 * Current offload engine state.
 *
 * @return
 * DOCA_SUCCESS
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE -  If 'offload' or 'state' is NULL.
 * - DOCA_ERROR_NOT_PERMITTED - If 'offload' is not started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_get_state(struct doca_devemu_virtio_offload_engine *offload,
							 enum doca_devemu_virtio_offload_engine_states *state);

/**
 * @brief Set the number of queues for a Virtio offload engine.
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to configure. Must not be started.
 * @param [in] num_queues
 * The num_queues value to set, as defined in the device configuration layout.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'offload' is started. Use doca_devemu_virtio_offload_engine_stop() to stop it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_set_num_queues(struct doca_devemu_virtio_offload_engine *offload,
							      uint16_t num_queues);

/**
 * @brief Check if indirect descriptors are enabled for a virtio offload engine.
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to query.
 * @param [out] enabled
 * 1 if indirect descriptors are enabled for the virtio offload engine, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' or 'enabled' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_get_indir_descs_enabled(struct doca_devemu_virtio_offload_engine *offload, uint8_t *enabled);

/**
 * @brief Enable or disable indirect descriptors for a virtio offload engine.
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to configure. Must not be started.
 * @param [in] enabled
 * Set to 1 to enable indirect descriptors for the virtio offload engine. Otherwise, set to 0.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'offload' is started. Use doca_devemu_virtio_offload_engine_stop() to stop it.
 * - DOCA_ERROR_NOT_SUPPORTED - If 'offload' does not support indirect descriptors processing.
 * @note Indirect descriptors are supported only if the corresponding doca_devemu_*_cap_is_indir_descs_supported() function returns true.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_set_indir_descs_enabled(struct doca_devemu_virtio_offload_engine *offload, uint8_t enabled);

/**
 * @brief Create an empty list of debug state for Virtio offload engine queues.
 *
 * @details Allocates an unpopulated list (array) of queue debug state structures for the started queues of a Virtio offload engine.
 *
 * @param [in] offload
 * The DOCA devemu Virtio offload engine. Must be enabled.
 * @param [out] dbg_list
 * The newly created list of DOCA devemu Virtio queue debug state structures. After a successful call, the list can be
 * accessed as (dbg_list)[idx], where idx ranges from 0 to (list_len) - 1.
 * @param [out] list_len
 * The length of the dbg_list.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload', 'dbg_list' or 'list_len' is NULL.
 * - DOCA_ERROR_NOT_PERMITTED - If 'offload' has no available queues for which debug state can be populated.
 * - DOCA_ERROR_NOT_SUPPORTED - If 'offload' does not support populate queue debug state.
 * - DOCA_ERROR_BAD_STATE - If 'offload' is not enabled.
 * - DOCA_ERROR_NO_MEMORY - Failed to allocate internal resources.
 * @note The returned dbg_list should be populated using doca_devemu_virtio_queue_dbg_state_populate_list(). The returned
 * dbg_list must be deallocated using doca_devemu_virtio_queue_dbg_state_destroy_list() to prevent memory leaks.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_offload_engine_queue_dbg_state_create_list(struct doca_devemu_virtio_offload_engine *offload,
		struct doca_devemu_virtio_queue_dbg_state ***dbg_list, uint32_t *list_len);

/*********************************************************************************************************************
 * DOCA devemu Virtio virtqueue API
 *********************************************************************************************************************/

/**
 * @brief Set the configuration of a DOCA Virtio virtqueue.
 *
 * @param [in] vq
 * The DOCA Virtio virtqueue to configure. Must not be started.
 * @param [in] index
 * The virtqueue index.
 * @param [in] size
 * The virtqueue size.
 * @param [in] msix_vector
 * The MSI-X vector for virtqueue notifications.
 * @param [in] desc_addr
 * The physical address of Descriptor Area associated with the virtqueue.
 * @param [in] driver_addr
 * The physical address of Driver Area associated with the virtqueue.
 * @param [in] device_addr
 * The physical address of Device Area associated with the virtqueue.
 *
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'vq' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'vq' is started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_vq_set_conf(struct doca_devemu_virtio_vq *vq,
					    uint16_t index,
					    uint16_t size,
					    uint16_t msix_vector,
					    uint64_t desc_addr,
					    uint64_t driver_addr,
					    uint64_t device_addr);

/**
 * @brief Start a Virtio virtqueue.
 *
 * @param [in] vq
 * The DOCA Virtio virtqueue to start.
 *
 * @details Once started, the Virtio virtqueue begins processing operations from the driver when enabled
 * via doca_devemu_virtio_vq_enable() or via doca_devemu_virtio_offload_engine_enable().
 * It finishes processing operations when moves to disabled state.
 * This operation is not allowed if the associated Virtio offload engine is in the DISABLING state.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'vq' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'vq' is already started.
 * - DOCA_ERROR_NOT_PERMITTED - If the associated offload engine is in the DISABLING state.
 * @note This method upon success disable the ability to configure the DOCA devemu Virtio virtqueue.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_vq_start(struct doca_devemu_virtio_vq *vq);

/**
 * @brief Stop a Virtio virtqueue.
 *
 * @details The Virtio virtqueue must be disabled and must not be associated with any Virtio IO context before being
 * stopped.
 * This operation is not allowed if the associated Virtio offload engine is in the DISABLING state.
 *
 * @param [in] vq
 * The DOCA Virtio virtqueue to stop. Must be disabled and must not be associated to any Virtio IO context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'vq' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'vq' is already stopped.
 * - DOCA_ERROR_NOT_PERMITTED - If 'vq' is enabled, associated with a Virtio IO context or if the associated offload
 *   engine is in the DISABLING state.
 * @note This method upon success re-enable the ability to configure the DOCA devemu Virtio offload engine.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_vq_stop(struct doca_devemu_virtio_vq *vq);

/**
 * @brief This enum defines the states of operation for a started virtqueue.
 *
 *
 * @code
 * State machine diagram:
 *
 *               All in-flight requests are drained or flushed (no processing of requests)
 *                            +-----------+
 *                            |           |
 *   +----------------------->| disabled  |<---------------------------------------------------------------------+
 *   |                        |           |                                                                      |
 *   |                        +-----+-----+                                                                      |
 *   |                            |                                                                              |
 *   |                            | doca_devemu_virtio_vq_enable()/doca_devemu_virtio_offload_engine_enable()/   |
 *   |                            | doca_devemu_virtio_vq_group_enable() has been called                         |
 *   |                            |                                                                              |
 *   |                            |                                                                              |
 *   |                            |                                                                              |
 *   |                            V                                                                              |
 *   |                       +------------+                                                                      |
 *   |                       |            |                                                                      |
 *   |                       | enabled    |                                                                      |
 *   |                       |            |                                                                      |
 *   |                       +-----+------+                                                                      |
 *   |                            |                                                                              |
 *   |                            | doca_devemu_virtio_vq_disable()/doca_devemu_virtio_offload_engine_disable()/ |
 *   |                            | doca_devemu_virtio_vq_group() has been called                                |
 *   |                            |                                                                              |
 *   |                            |                                                                              |
 *   |                            | Synchronous: Change state to disabled and return DOCA_SUCCESS                |
 *   |                            |                                                                              |
 *   |                            +------------------------------------------------------------------------------+
 *   |                            |
 *   |                            | Asynchronous: Change state to disabling and return DOCA_ERROR_IN_PROGRESS
 *   |                            |
 *   |                            v
 *   |                       +------------+
 *   |                       |            |
 *   |-----------------------+ disabling  |
 *                           |            |
 *                           +------------+
 * @endcode
 *
 */
enum doca_devemu_virtio_vq_states {
	/**
	 * The initial state of the virtqueue after starting.
	 * Indicates that doca_devemu_virtio_vq_disable()/doca_devemu_virtio_offload_engine_disable()/
	 * doca_devemu_virtio_vq_group_disable() has been called.
	 * Processing new operations from the driver and issuing DMA operations is not allowed.
	 */
	DOCA_DEVEMU_VIRTIO_VQ_STATE_DISABLED = 0,
	/**
	 * Indicates that doca_devemu_virtio_vq_enable()/doca_devemu_virtio_offload_engine_enable()/
	 * doca_devemu_virtio_vq_group_enable() has been called.
	 * Processing new operations from the driver and issuing DMA operations is allowed.
	 */
	DOCA_DEVEMU_VIRTIO_VQ_STATE_ENABLED,
	/*
	 * Indicates that doca_devemu_virtio_vq_disable()/doca_devemu_virtio_offload_engine_disable()/
	 * doca_devemu_virtio_vq_group_disable() has been called
	 * and the virtqueue is asynchronously transitioning to DISABLED state.
	 * Processing of new requests from the driver is terminated, while issuing DMA operations by the device for
	 * in-progress requests is still allowed.
	 * The virtqueue will usually move to DISABLED state when the number of in-flight requests reaches 0.
	 * The application should call doca_devemu_virtio_vq_get_state() to verify that the virtqueue is in the
	 * DISABLED state.
	 */
	DOCA_DEVEMU_VIRTIO_VQ_STATE_DISABLING,
};

/**
 * @brief Enable a Virtio virtqueue.
 *
 * @details Some virtqueue types are not fully offloaded by their associated offload engine and must be bound to
 * one or more Virtio IO contexts before enabling the virtqueue.
 * This operation is not allowed if the associated offload engine is not in the ENABLED state.
 *
 * @param [in] vq
 * The DOCA Virtio virtqueue to enable. Must be started.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'vq' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'vq' is already enabled.
 * - DOCA_ERROR_NOT_PERMITTED - If 'vq' is not started, must be bound to at least one Virtio IO context before being
 *   enabled, or if the associated offload engine is not in the ENABLED state.
 * @note This method upon success initiates processing of operations from/to the driver. For VQ types requiring IO
 * context binding, the VQ must be bound before enabling.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_vq_enable(struct doca_devemu_virtio_vq *vq);

/**
 * @brief Disable a Virtio virtqueue.
 *
 * @details If this method returns DOCA_SUCCESS, processing of new requests from/to the driver is terminated and
 * issuing new DMA operations by the device is not allowed (virtqueue disablement is synchronous).
 * If this method returns DOCA_ERROR_IN_PROGRESS, processing of new requests from the driver is terminated while
 * allowing the device to issue new DMA operations for in-progress requests. The virtqueue will become fully
 * disabled once all outstanding operations have completed. To verify, use doca_devemu_virtio_vq_get_state().
 *
 * This operation is not allowed if the associated offload engine is not in the ENABLED state.
 *
 * @param [in] vq
 * The DOCA Virtio virtqueue to disable. Must be started and enabled.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_IN_PROGRESS - If operations are still in progress. The virtqueue moved to disabling state.
 * - DOCA_ERROR_INVALID_VALUE - If 'vq' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'vq' is not enabled.
 * - DOCA_ERROR_NOT_PERMITTED - If 'vq' is not started or if the associated offload engine is not in the ENABLED state.
 * @note For VQ types requiring IO context binding, after this function returns DOCA_ERROR_IN_PROGRESS, no new
 * operations will be fetched from the driver, though events may still be sent to the corresponding application via the
 * bound IO context. To flush all pending events to the corresponding application, use
 * doca_devemu_virtio_io_flush_vq() on the same core that is progressing the Virtio IO context.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_vq_disable(struct doca_devemu_virtio_vq *vq);

/**
 * @brief Enable group of Virtio virtqueues.
 *
 * @details Some virtqueue types are not fully offloaded by their associated offload engine and must be bound to
 * one or more Virtio IO contexts before enabling the virtqueue.
 * This operation is not allowed if the associated offload engine is not in the ENABLED state.
 * This operation enables virtqueues in the supplied range. All the VQs in the given range must be started.
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to configure. Must be enabled.
 * @param [in] start_vq_index
 * The start index of the group of DOCA virtio virtqueues to enable.
 * @param [in] end_vq_index
 * The end index of the group of DOCA virtio virtqueues to enable.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL or any of the virtqueues in the range is invalid.
 * - DOCA_ERROR_BAD_STATE - If any of the virtqueues in the specified range is already enabled.
 * - DOCA_ERROR_NOT_PERMITTED - If any of the virtqueues in the specified range is not started, must be bound to at least one Virtio IO context before being
 *   enabled, or if the associated offload engine is not in the ENABLED state.
 * @note This method upon success initiates processing of operations from/to the driver. For VQ types requiring IO
 * context binding, all the VQs in the given range must be bound before enabling.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_vq_group_enable(struct doca_devemu_virtio_offload_engine *offload,
						uint16_t start_vq_index, uint16_t end_vq_index);

/**
 * @brief Disable group of Virtio virtqueues.
 *
 * @details If this method returns DOCA_SUCCESS, processing of new requests from/to the driver is terminated and
 * issuing new DMA operations by the device is not allowed (virtqueue disablement is synchronous).
 * If this method returns DOCA_ERROR_IN_PROGRESS, processing of new requests from the driver is terminated while
 * allowing the device to issue new DMA operations for in-progress requests. The virtqueues in the given range
 * will become fully disabled once all outstanding operations have completed. To verify, use
 * doca_devemu_virtio_vq_get_state() for each of the VQ in the given range. All the VQs in the given range
 * must be started and enabled.
 * This operation is not allowed if the associated offload engine is not in the ENABLED state.
 *
 * @param [in] offload
 * The DOCA Virtio offload engine to configure. Must be enabled.
 * @param [in] start_vq_index
 * The start index of the group of DOCA virtio virtqueues to disable.
 * @param [in] end_vq_index
 * The end index of the group of DOCA virtio virtqueues to disable.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_IN_PROGRESS - If operations are still in progress. The virtqueues moved to disabling state.
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL or any of the virtqueues in the range is invalid.
 * - DOCA_ERROR_BAD_STATE - If any of the virtqueues in the specified range is not enabled.
 * - DOCA_ERROR_NOT_PERMITTED - If any of the virtqueues in the specified range is not enabled or if the associated
 *   offload engine is not in the ENABLED state.
 * @note For VQ types requiring IO context binding, after this function returns DOCA_ERROR_IN_PROGRESS, no new
 * operations will be fetched from the driver, though events may still be sent to the corresponding application via the
 * bound IO context. To flush all pending events to the corresponding application, use
 * doca_devemu_virtio_io_flush_vq() for all the VQs in the given range on the same core that is progressing the
 * Virtio IO context.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_vq_group_disable(struct doca_devemu_virtio_offload_engine *offload,
						 uint16_t start_vq_index, uint16_t end_vq_index);

/**
 * @brief Get Virtio virtqueue state
 *
 * @param [in] vq
 * The DOCA Virtio virtqueue for which to retrieve the state.
 * @param [out] state
 * Current virtqueue state.
 *
 * @return
 * DOCA_SUCCESS
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE -  If 'vq' or 'state' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_vq_get_state(const struct doca_devemu_virtio_vq *vq,
					     enum doca_devemu_virtio_vq_states *state);

/*********************************************************************************************************************
 * DOCA devemu TLP Virtio IO context API
 *********************************************************************************************************************/

/**
 * @brief Bind a DOCA Virtio virtqueue to a DOCA Virtio device IO context.
 *
 * This function must be invoked on the same thread that manages the IO context.
 *
 * @details Binding a virtqueue to an IO context allows the context to process requests associated with the virtqueue
 * and generate events for the user once the virtqueue is enabled. Some virtqueue types are fully offloaded by their
 * associated offload engine and don't require binding to an IO context.
 * The vq_user_data parameter is optional, and should be provided only if a user context needs to be associated with
 * the virtqueue and the specific IO context. If a virtqueue is bound to more than one IO context (if supported), it
 * is recommended to associate a different vq_user_data value for each binding.
 *
 * @param [in] io
 * The DOCA Virtio device IO context to bind to. Must be started.
 * @param [in] vq
 * The DOCA Virtio virtqueue to associate with the IO context. Must be disabled, unbound and started.
 * @param [in] vq_user_data
 * The user data to associate with the bound virtqueue on the specified IO context. May be NULL.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'io' or 'vq' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'io' or 'vq' is not started.
 * - DOCA_ERROR_NOT_PERMITTED - If 'vq' is already bound, is enabled, or is fully offloaded and cannot be bound.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_io_bind_vq(struct doca_devemu_virtio_io *io, struct doca_devemu_virtio_vq *vq,
		void *vq_user_data);

/**
 * @brief Unbind a DOCA Virtio virtqueue from a DOCA Virtio device IO context.
 *
 * This function must be invoked on the same thread that manages the IO context.
 *
 * @details Before unbinding, the corresponding application must:
 *     1. Call doca_devemu_virtio_vq_disable()/doca_devemu_virtio_offload_engine_disable()/
 *        doca_devemu_virtio_vq_group_disable().
 *     2. Wait until all pending events from the IO context associated with the virtqueue have been flushed, or
 *        explicitly flush them using doca_devemu_virtio_io_flush_vq().
 *     3. Complete all in-flight IO context events associated with the virtqueue.
 *     4. Verify virtqueue is in disabled state.
 *
 * @param [in] io
 * The DOCA Virtio device IO context to unbind.
 * @param [in] vq
 * The DOCA Virtio virtqueue to disassociate from the IO context. Must be bounded and disabled.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'io' or 'vq' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'vq' is not bound.
 * - DOCA_ERROR_NOT_PERMITTED - If 'vq' is not disabled or 'io' has pending/in-flight events associated with the 'vq'.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_io_unbind_vq(struct doca_devemu_virtio_io *io, struct doca_devemu_virtio_vq *vq);

/**
 * @brief Flush all pending events of a DOCA Virtio IO context which are associated with the DOCA Virtio virtqueue.
 *
 * This function must be invoked on the same thread that manages the IO context.
 *
 * @param [in] io
 * The DOCA Virtio device IO context.
 * @param [in] vq
 * The DOCA Virtio virtqueue whose events are to be flushed. Must be bounded and not enabled.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'io' or 'vq' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'vq' is not bound.
 * - DOCA_ERROR_NOT_PERMITTED - If 'vq' is enabled.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_virtio_io_flush_vq(struct doca_devemu_virtio_io *io, struct doca_devemu_virtio_vq *vq);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VIRTIO_TLP_H_ */
