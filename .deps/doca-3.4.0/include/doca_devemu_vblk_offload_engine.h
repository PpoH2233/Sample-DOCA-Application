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
 * @file doca_devemu_vblk_offload_engine.h
 * @page doca_devemu_vblk_offload_engine
 * @defgroup DOCA_DEVEMU_VBLK_OFFLOAD_ENGINE DOCA Device Emulation - Virtio Block offload engine
 * @ingroup DOCA_DEVEMU_VBLK
 *
 * DOCA Virtio Block offload engine
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VBLK_OFFLOAD_ENGINE_H_
#define DOCA_DEVEMU_VBLK_OFFLOAD_ENGINE_H_

#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>
#include <doca_devemu_vblk.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA devemu Virtio Block offload engine API
 *********************************************************************************************************************/

/**
 * @brief Allocate a DOCA Devemu Virtio Block offload engine.
 *
 * @details The Virtio Block offload engine is responsible for managing the state of all offloaded entities on a device
 * (e.g., configuration, creation, destruction, start, and stop of Virtio Block queues).
 * Each Virtio Block offload engine is associated with a single DOCA Devemu PCI endpoint.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint to be associated with the offload engine.
 * @param [out] offload
 * The created DOCA Virtio Block offload engine.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_offload_engine_create(struct doca_devemu_pci_ep *ep,
						    struct doca_devemu_vblk_offload_engine **offload);

/**
 * @brief Free a Virtio Block offload engine.
 *
 * @param [in] offload
 * The DOCA Virtio Block offload engine to release. Must no be started.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL
 * - DOCA_ERROR_BAD_STATE - If 'offload' is started. Use doca_devemu_virtio_offload_engine_stop() to stop it
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_offload_engine_destroy(struct doca_devemu_vblk_offload_engine *offload);

/**
 * @brief Export the runtime state representation of a Virtio Block offload engine.
 *
 * @details The produced export descriptor is an opaque blob that contains the runtime state representation of the
 * offload engine. It can be transferred to another process and used there with
 * doca_devemu_vblk_offload_engine_create_from_export() to obtain an offload engine that
 * reflects the same runtime state as the exported engine.
 *
 * Typical use cases include:
 * - Handover: the export descriptor of an offload engine is produced by a running source process and imported into a
 *   destination process. It is the responsibility of the user to coordinate the roles configured on the source and
 *   destination offload engines to ensure that the roles are consistent as explained in the documentation of
 *   doca_devemu_virtio_offload_engine_enable().
 * - Takeover (recovery): the export descriptor is produced by a running process, before it crashes, and kept in a
 *   secure location for later recovery. It is later imported into a recovering process to recreate the offload engine.
 *   It is the responsibility of the user to ensure that the export descriptor is not compromised and that the recovering
 *   process uses the same build of the DOCA devemu library as the process that produced the export descriptor,
 *   otherwise the behavior is undefined.
 *
 * The returned export descriptor pointer must not be used after the offload engine is stopped. Once the offload engine is
 * stopped, the export descriptor memory is invalidated. If the export descriptor pointer is used after the offload engine
 * is stopped, the behavior is undefined.
 * If the offload engine is started again, the export descriptor should be re-created by calling this function again.
 *
 * The caller must not release the returned export descriptor pointer.
 *
 * @param [in] offload
 * The DOCA Virtio Block offload engine to export. Must be started.
 * @param [out] export_desc
 * A blob containing the runtime state representation of the offload engine. Valid only upon success.
 * @param [out] export_desc_len
 * Length in bytes of export_desc. Valid only upon success.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload', 'export_desc', or 'export_desc_len' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'offload' is not started.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * @note The exported data contains sensitive information. Pass the export descriptor through a secure channel.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_offload_engine_export(struct doca_devemu_vblk_offload_engine *offload,
						    const void **export_desc,
						    size_t *export_desc_len);

/**
 * @brief Release an export descriptor produced by doca_devemu_vblk_offload_engine_export().
 *
 * @details Frees the memory allocated for the export descriptor. The descriptor must not have been released yet
 * (e.g. by a previous call to this function). Double release is invalid and may lead to undefined behavior.
 * After a successful call, the export descriptor must not be used. It is the responsibility of the user to ensure
 * that the export descriptor is not compromised and that the releasing process uses the same build of the DOCA
 * devemu library as the process that produced the export descriptor, otherwise the behavior is undefined.
 *
 * @param [in] export_desc
 * The export descriptor to release. Must have been returned by doca_devemu_vblk_offload_engine_export() and not
 * yet released.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'export_desc' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_offload_engine_export_release(const void *export_desc);

/**
 * @brief Create a DOCA Virtio Block offload engine from an export descriptor.
 *
 * @details This function is typically used on the destination side during a handover or in the recovered process after
 * a crash. The export_desc parameter represents the runtime state of the exported offload engine produced by
 * doca_devemu_vblk_offload_engine_export(). The created offload engine, if configured correctly, will reflect the same
 * runtime state as the exported engine, once it becomes enabled.
 *
 * @param [in] export_desc
 * Export descriptor produced by doca_devemu_vblk_offload_engine_export(), typically from the source offload engine.
 * @param [in] export_desc_len
 * Length in bytes of export_desc.
 * @param [in] ep
 * The DOCA devemu PCI endpoint to be associated with the offload engine. This must remain valid for the lifetime of
 * the Virtio Block offload engine.
 * @param [out] offload
 * The newly created idle DOCA Virtio Block offload engine.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'export_desc', 'ep', or 'offload' is NULL or 'export_desc_len' is 0.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_FOUND - If 'export_desc' or 'export_desc_len' does not correspond to an exported offload engine.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_offload_engine_create_from_export(const void *export_desc,
								size_t export_desc_len,
								struct doca_devemu_pci_ep *ep,
								struct doca_devemu_vblk_offload_engine **offload);

/**
 * @brief Convert DOCA Virtio Block offload engine instance into DOCA Virtio offload engine.
 *
 * @param [in] offload
 * DOCA Virtio Block offload engine instance. This must remain valid until after the DOCA Virtio offload engine is no
 * longer required.
 *
 * @return
 * doca devemu virtio offload engine upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_virtio_offload_engine *doca_devemu_vblk_offload_engine_as_virtio_offload(
	struct doca_devemu_vblk_offload_engine *offload);

/**
 * @brief Set the seg_max value for a vblk offload engine.
 *
 * @param [in] offload
 * The DOCA vblk offload engine to configure. Must not be started.
 * @param [in] seg_max
 * The value for seg_max.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' is NULL. Or if seg_max does not conform to the Virtio specification.
 * - DOCA_ERROR_BAD_STATE - If 'offload' is started. Use doca_devemu_virtio_offload_engine_stop() to stop it.
 * - DOCA_ERROR_NOT_PERMITTED - If 'seg_max' is greater than the value that was set using doca_devemu_vblk_set_seg_max().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_offload_engine_set_seg_max(struct doca_devemu_vblk_offload_engine *offload,
							     uint16_t seg_max);

/**
 * @brief Get the maximum seg_max value for a vblk offload engine.
 *
 * @param [in] offload
 * The DOCA vblk offload engine to query.
 * @param [out] seg_max
 * The value of seg_max that was set for this offload engine.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' or 'seg_max' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_offload_engine_get_seg_max(struct doca_devemu_vblk_offload_engine *offload,
							     uint16_t *seg_max);

/*********************************************************************************************************************
 * DOCA devemu Virtio Block request virtqueue API
 *********************************************************************************************************************/

/**
 * @brief Allocate a DOCA Devemu Virtio Block request virtqueue.
 *
 * This function must be invoked on the same thread that manages the offload engine.
 *
 * @param [in] offload
 * The DOCA Virtio Block offload engine to be associated with the virtqueue. Must be started.
 * @param [out] vq
 * The created DOCA Virtio Block request virtqueue.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_req_vq_create(struct doca_devemu_vblk_offload_engine *offload,
					    struct doca_devemu_vblk_req_vq **vq);

/**
 * @brief Free a Virtio Block request virtqueue.
 *
 * This function must be invoked on the same thread that manages the offload engine.
 *
 * @param [in] vq
 * The DOCA Virtio Block request virtqueue to release. Must not be started.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'vq' is NULL
 * - DOCA_ERROR_BAD_STATE - If 'vq' is started. Use doca_devemu_virtio_vq_stop() to stop it
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_req_vq_destroy(struct doca_devemu_vblk_req_vq *vq);

/**
 * @brief Convert DOCA Virtio Block request virtqueue instance into DOCA Virtio virtqueue.
 *
 * @param [in] vq
 * DOCA Virtio Block request virtqueue instance. This must remain valid until after the DOCA Virtio virtqueue is no
 * longer required.
 *
 * @return
 * DOCA devemu Virtio virtqueue upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_virtio_vq *doca_devemu_vblk_req_vq_as_vq(struct doca_devemu_vblk_req_vq *vq);


#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VBLK_OFFLOAD_ENGINE_H_ */
