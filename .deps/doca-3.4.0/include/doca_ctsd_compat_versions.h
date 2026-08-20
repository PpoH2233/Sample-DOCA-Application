/*
 *
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

#ifndef DOCA_CTSD_COMPAT_VERSIONS_H
#define DOCA_CTSD_COMPAT_VERSIONS_H

/**
 * @brief DOCA CTSD compatibility versions header
 *
 * This header defines different DOCA compatibility versions, where each version
 * represents a specific alignment of DOCA library versions. Backward Compatibility
 * is achieved by defining the desired DOCA_COMPAT_VERSION to use.
 */

#ifndef DOCA_CTSD_COMPAT_VERSION
#define DOCA_CTSD_COMPAT_VERSION DOCA_CTSD_COMPAT_VERSION_1 // Default version if not provided
#endif

#define DOCA_CTSD_COMPAT_VERSION_1 1

#undef LIB_FLOW_VERSION

#if DOCA_CTSD_COMPAT_VERSION == DOCA_CTSD_COMPAT_VERSION_1
#define LIB_FLOW_VERSION LIB_FLOW_VERSION_1
#endif

#endif /* DOCA_CTSD_COMPAT_VERSIONS_H */
