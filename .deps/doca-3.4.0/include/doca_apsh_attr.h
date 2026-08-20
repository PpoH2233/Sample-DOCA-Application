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
 * @file doca_apsh_attr.h
 * @page apsh_attr
 * @defgroup DOCA_APSH_ATTR DOCA App Shield Attributes
 * @ingroup DOCA_APSH
 *
 * DOCA App Shield attributes to query with get functions, see doca_apsh.h
 *
 * @{
 */

#ifndef _DOCA_APSH_ATTR__H_
#define _DOCA_APSH_ATTR__H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief system os types
 */
enum doca_apsh_system_os {
	DOCA_APSH_SYSTEM_LINUX = 0,   /**< linux */
	DOCA_APSH_SYSTEM_WINDOWS = 1, /**< windows */
};

/**
 * @brief doca app shield configuration attributes
 */
enum doca_apsh_system_config_attr {
	DOCA_APSH_OS_SYMBOL_MAP = 0,	     /**< os symbol map path */
	DOCA_APSH_MEM_REGION = 1,	     /**< memory region path */
	DOCA_APSH_KPGD_FILE = 2,	     /**< kpgd file path */
	DOCA_APSH_VHCA_ID = 3,		     /**< vhca id */
	DOCA_APSH_OS_TYPE = 4,		     /**< os type */
	DOCA_APSH_SCAN_WIN_SIZE = 5,	     /**< yara scan window size */
	DOCA_APSH_SCAN_WIN_STEP = 6,	     /**< yara scan window step */
	DOCA_APSH_HASHTEST_LIMIT = 7,	     /**< limit of vm areas to attest */
	DOCA_APSH_MODULES_LIMIT = 8,	     /**< limit of modules number */
	DOCA_APSH_PROCESS_LIMIT = 9,	     /**< limit of processes number */
	DOCA_APSH_THREADS_LIMIT = 10,	     /**< limit of threads number */
	DOCA_APSH_LDRMODULES_LIMIT = 11,     /**< limit of ldrmodules number on windows */
	DOCA_APSH_LIBS_LIMIT = 12,	     /**< limit of libs number */
	DOCA_APSH_VADS_LIMIT = 13,	     /**< limit of vads number */
	DOCA_APSH_WINDOWS_ENVARS_LIMIT = 14, /**< length limit of envars for windows */
	DOCA_APSH_HANDLES_LIMIT = 15,	     /**< limit of handles/FDs/sockets number */
	DOCA_APSH_STRING_LIMIT = 16,	     /**< length limit of apsh_read_str */
	DOCA_APSH_OS_SYMBOL_MAP_FOLDER = 17, /**< os symbol map files folder  */
	DOCA_APSH_FILESIZE_LIMIT = 18,	     /**< length limit of file size parsing */
};

/** @brief dma dev name */
typedef struct doca_dev *DOCA_APSH_DMA_DEV_TYPE;
/** @brief os symbol map path */
typedef char *DOCA_APSH_OS_SYMBOL_MAP_TYPE;
/** @brief memory region path */
typedef char *DOCA_APSH_MEM_REGION_TYPE;
/** @brief kpgd file path */
typedef char *DOCA_APSH_KPGD_FILE_TYPE;
/** @brief vhca id */
typedef struct doca_dev_rep *DOCA_APSH_VHCA_ID_TYPE;
/** @brief os type */
typedef enum doca_apsh_system_os DOCA_APSH_OS_TYPE_TYPE;
/** @brief yara scan window size */
typedef uint32_t DOCA_APSH_SCAN_WIN_SIZE_TYPE;
/** @brief yara scan window step */
typedef uint32_t DOCA_APSH_SCAN_WIN_STEP_TYPE;
/** @brief limit of vm areas to attest */
typedef int DOCA_APSH_HASHTEST_LIMIT_TYPE;
/** @brief limit of modules number */
typedef int DOCA_APSH_MODULES_LIMIT_TYPE;
/** @brief limit of processes number */
typedef int DOCA_APSH_PROCESS_LIMIT_TYPE;
/** @brief limit of threads number */
typedef int DOCA_APSH_THREADS_LIMIT_TYPE;
/** @brief limit of libs number */
typedef int DOCA_APSH_LIBS_LIMIT_TYPE;
/** @brief limit of vads number */
typedef int DOCA_APSH_VADS_LIMIT_TYPE;
/** @brief length limit of envars for windows */
typedef int DOCA_APSH_WINDOWS_ENVARS_LIMIT_TYPE;
/** @brief length limit of apsh_read_str */
typedef int DOCA_APSH_STRING_LIMIT_TYPE;
/** @brief os symbol map folder path */
typedef char *DOCA_APSH_OS_SYMBOL_MAP_FOLDER_TYPE;
/** @brief limit of parsed files size */
typedef int DOCA_APSH_FILESIZE_LIMIT_TYPE;

/**
 * @brief doca app shield process attributes
 */
enum doca_apsh_process_attr {
	DOCA_APSH_PROCESS_PID = 0,		    /**< process id */
	DOCA_APSH_PROCESS_PPID = 1,		    /**< process parent id */
	DOCA_APSH_PROCESS_COMM = 2,		    /**< process executable name */
	DOCA_APSH_PROCESS_CPU_TIME = 3,		    /**< process cpu time [ps]*/
	DOCA_APSH_PROCESS_WINDOWS_OFFSET = 1000,    /**< process offset */
	DOCA_APSH_PROCESS_WINDOWS_THREADS = 1001,   /**< process thread count */
	DOCA_APSH_PROCESS_WINDOWS_EXIT_TIME = 1002, /**< process exit time */
	DOCA_APSH_PROCESS_LINUX_GID = 2000,	    /**< process group id */
	DOCA_APSH_PROCESS_LINUX_UID = 2001,	    /**< process user id */
	DOCA_APSH_PROCESS_LINUX_STATE = 2002,	    /**< process state */
	DOCA_APSH_PROCESS_LINUX_NS_PID = 2003,	    /**< process PID namespace */
	DOCA_APSH_PROCESS_LINUX_NS_MNT = 2004,	    /**< process mount namespace */
	DOCA_APSH_PROCESS_LINUX_NS_NET = 2005,	    /**< process network namespace */
};

/** @brief process pid type */
typedef uint32_t DOCA_APSH_PROCESS_PID_TYPE;
/** @brief process pid type */
typedef uint32_t DOCA_APSH_PROCESS_PPID_TYPE;
/** @brief process comm type */
typedef const char *DOCA_APSH_PROCESS_COMM_TYPE;
/** @brief process cpu time type */
typedef uint64_t DOCA_APSH_PROCESS_CPU_TIME_TYPE;
/** @brief process offset type */
typedef uint64_t DOCA_APSH_PROCESS_WINDOWS_OFFSET_TYPE;
/** @brief process threads type */
typedef uint32_t DOCA_APSH_PROCESS_WINDOWS_THREADS_TYPE;
/** @brief process exit time type */
typedef uint64_t DOCA_APSH_PROCESS_WINDOWS_EXIT_TIME_TYPE;
/** @brief process gid type */
typedef uint32_t DOCA_APSH_PROCESS_LINUX_GID_TYPE;
/** @brief process uid type */
typedef uint32_t DOCA_APSH_PROCESS_LINUX_UID_TYPE;
/** @brief process state type */
typedef uint64_t DOCA_APSH_PROCESS_LINUX_STATE_TYPE;
/** @brief process PID namespace type */
typedef uint32_t DOCA_APSH_PROCESS_LINUX_NS_PID_TYPE;
/** @brief process mount namespace type */
typedef uint32_t DOCA_APSH_PROCESS_LINUX_NS_MNT_TYPE;
/** @brief process network namespace type */
typedef uint32_t DOCA_APSH_PROCESS_LINUX_NS_NET_TYPE;

/**
 * @brief doca app shield thread attributes
 */
enum doca_apsh_thread_attr {
	DOCA_APSH_THREAD_PID = 0,		       /**< thread process id */
	DOCA_APSH_THREAD_TID = 1,		       /**< thread id */
	DOCA_APSH_THREAD_STATE = 2,		       /**< thread state */
	DOCA_APSH_THREAD_WINDOWS_WAIT_REASON = 1000,   /**< thread wait reason */
	DOCA_APSH_THREAD_WINDOWS_OFFSET = 1001,	       /**< thread offset */
	DOCA_APSH_THREAD_WINDOWS_SUSPEND_COUNT = 1002, /**< thread suspend count */
	DOCA_APSH_THREAD_LINUX_PROC_NAME = 2000,       /**< thread process name */
	DOCA_APSH_THREAD_LINUX_THREAD_NAME = 2001,     /**< thread name */
};

/** @brief thread pid type */
typedef uint32_t DOCA_APSH_THREAD_PID_TYPE;
/** @brief thread tid type */
typedef uint32_t DOCA_APSH_THREAD_TID_TYPE;
/** @brief thread state type */
typedef uint64_t DOCA_APSH_THREAD_STATE_TYPE;
/** @brief thread wait reason type */
typedef uint8_t DOCA_APSH_THREAD_WINDOWS_WAIT_REASON_TYPE;
/** @brief thread offset type */
typedef uint64_t DOCA_APSH_THREAD_WINDOWS_OFFSET_TYPE;
/** @brief thread suspend count type */
typedef uint8_t DOCA_APSH_THREAD_WINDOWS_SUSPEND_COUNT_TYPE;
/** @brief thread proc name type */
typedef const char *DOCA_APSH_THREAD_LINUX_PROC_NAME_TYPE;
/** @brief thread thread name type */
typedef const char *DOCA_APSH_THREAD_LINUX_THREAD_NAME_TYPE;

/**
 * @brief doca app shield lib attributes
 */
enum doca_apsh_lib_attr {
	DOCA_APSH_LIB_PID = 0,			    /**< lib pid */
	DOCA_APSH_LIB_LIBRARY_PATH = 2,		    /**< lib loaded library path */
	DOCA_APSH_LIB_LOAD_ADDRESS = 3,		    /**< lib load address for both Windows and Linux*/
	DOCA_APSH_LIB_WINDOWS_DLL_NAME = 1000,	    /**< lib dll name */
	DOCA_APSH_LIB_WINDOWS_SIZE_OF_IMAGE = 1001, /**< lib size of image */
	DOCA_APSH_LIB_LINUX_LOAD_ADDRESS = 2000,    /**< lib load address for Linux. It's kept for backwards
						     * compatibility, use DOCA_APSH_LIB_LOAD_ADDRESS instead-
						     */
};

/** @brief lib pid type */
typedef uint32_t DOCA_APSH_LIB_PID_TYPE;
/** @brief lib loaded library path type */
typedef const char *DOCA_APSH_LIB_LIBRARY_PATH_TYPE;
/** @brief lib load address for both Windows and Linux */
typedef uint64_t DOCA_APSH_LIB_LOAD_ADDRESS_TYPE;
/** @brief lib dll name type */
typedef const char *DOCA_APSH_LIB_WINDOWS_DLL_NAME_TYPE;
/** @brief lib size of image type */
typedef uint32_t DOCA_APSH_LIB_WINDOWS_SIZE_OF_IMAGE_TYPE;
/** @brief lib load address for Linux */
typedef uint64_t DOCA_APSH_LIB_LINUX_LOAD_ADDRESS_TYPE;

/**
 * @brief doca app shield virtual address descriptor attributes
 */
enum doca_apsh_vad_attr {
	DOCA_APSH_VMA_PID = 0,			     /**< vma process id */
	DOCA_APSH_VMA_OFFSET = 1,		     /**< vma offset */
	DOCA_APSH_VMA_PROTECTION = 2,		     /**< vma protection */
	DOCA_APSH_VMA_VM_START = 3,		     /**< vma vm start */
	DOCA_APSH_VMA_VM_END = 4,		     /**< vma vm end */
	DOCA_APSH_VMA_PROCESS_NAME = 5,		     /**< vma process name */
	DOCA_APSH_VMA_FILE_PATH = 6,		     /**< vma file path */
	DOCA_APSH_VMA_WINDOWS_COMMIT_CHARGE = 1000,  /**< vma commit charge */
	DOCA_APSH_VMA_WINDOWS_PRIVATE_MEMORY = 1001, /**< vma private memory */
	DOCA_APSH_VMA_WINDOWS_TAG = 1002,	     /**< vma pool tag */
};

/** @brief vma pid type */
typedef uint32_t DOCA_APSH_VMA_PID_TYPE;
/** @brief vma offset type */
typedef uint64_t DOCA_APSH_VMA_OFFSET_TYPE;
/** @brief vma protection type */
typedef const char *DOCA_APSH_VMA_PROTECTION_TYPE;
/** @brief vma vm start type */
typedef uint64_t DOCA_APSH_VMA_VM_START_TYPE;
/** @brief vma vm end type */
typedef uint64_t DOCA_APSH_VMA_VM_END_TYPE;
/** @brief vma file path type */
typedef const char *DOCA_APSH_VMA_PROCESS_NAME_TYPE;
/** @brief vma file path type */
typedef const char *DOCA_APSH_VMA_FILE_PATH_TYPE;
/** @brief vma commit charge type */
typedef uint32_t DOCA_APSH_VMA_WINDOWS_COMMIT_CHARGE_TYPE;
/** @brief vma private memory type */
typedef uint32_t DOCA_APSH_VMA_WINDOWS_PRIVATE_MEMORY_TYPE;
/** @brief vma tag type */
typedef const char *DOCA_APSH_VMA_WINDOWS_TAG_TYPE;

/**
 * @brief doca app shield attestation attributes
 */
enum doca_apsh_attestation_attr {
	DOCA_APSH_ATTESTATION_PID = 0,			/**< attestation process id */
	DOCA_APSH_ATTESTATION_COMM = 1,			/**< attestation process name */
	DOCA_APSH_ATTESTATION_PATH_OF_MEMORY_AREA = 2,	/**< attestation path of memory area */
	DOCA_APSH_ATTESTATION_PROTECTION = 3,		/**< attestation protection */
	DOCA_APSH_ATTESTATION_START_ADDRESS = 4,	/**< attestation start address */
	DOCA_APSH_ATTESTATION_END_ADDRESS = 5,		/**< attestation end address */
	DOCA_APSH_ATTESTATION_PAGES_NUMBER = 6,		/**< attestation process pages count in binary file */
	DOCA_APSH_ATTESTATION_PAGES_PRESENT = 7,	/**< attestation pages present in memory */
	DOCA_APSH_ATTESTATION_MATCHING_HASHES = 8,	/**< attestation pages hash match count from pages in memory */
	DOCA_APSH_ATTESTATION_HASH_DATA_IS_PRESENT = 9, /**< attestation hash data is present */
};

/** @brief attestation pid type */
typedef uint32_t DOCA_APSH_ATTESTATION_PID_TYPE;
/** @brief attestation comm type */
typedef const char *DOCA_APSH_ATTESTATION_COMM_TYPE;
/** @brief attestation path of memory area type */
typedef const char *DOCA_APSH_ATTESTATION_PATH_OF_MEMORY_AREA_TYPE;
/** @brief attestation protection type */
typedef const char *DOCA_APSH_ATTESTATION_PROTECTION_TYPE;
/** @brief attestation start address type */
typedef uint64_t DOCA_APSH_ATTESTATION_START_ADDRESS_TYPE;
/** @brief attestation end address type */
typedef uint64_t DOCA_APSH_ATTESTATION_END_ADDRESS_TYPE;
/** @brief attestation pages number type */
typedef int DOCA_APSH_ATTESTATION_PAGES_NUMBER_TYPE;
/** @brief attestation pages present type */
typedef int DOCA_APSH_ATTESTATION_PAGES_PRESENT_TYPE;
/** @brief attestation matching hashes type */
typedef int DOCA_APSH_ATTESTATION_MATCHING_HASHES_TYPE;
/** @brief attestation hash data is present type */
typedef bool DOCA_APSH_ATTESTATION_HASH_DATA_IS_PRESENT_TYPE;

/**
 * @brief doca app shield module attributes
 */
enum doca_apsh_module_attr {
	DOCA_APSH_MODULES_OFFSET = 0, /**< module offset */
	DOCA_APSH_MODULES_NAME = 1,   /**< module name */
	DOCA_APSH_MODULES_SIZE = 2,   /**< module size */
};

/** @brief module offset type */
typedef uint64_t DOCA_APSH_MODULES_OFFSET_TYPE;
/** @brief module name type */
typedef const char *DOCA_APSH_MODULES_NAME_TYPE;
/** @brief module size type */
typedef uint32_t DOCA_APSH_MODULES_SIZE_TYPE;

/**
 * @brief doca app shield privileges attributes
 * windows privilege list can be found on:
 * https://docs.microsoft.com/en-us/windows/win32/secauthz/privilege-constants
 */
enum doca_apsh_privilege_attr {
	DOCA_APSH_PRIVILEGES_PID = 0,		     /**< privilege process pid */
	DOCA_APSH_PRIVILEGES_NAME = 2,		     /**< privilege name, for example: SeTcbPrivilege */
	DOCA_APSH_PRIVILEGES_IS_ON = 3,		     /**< is the privilege turned on or off.
						      * For Windows this is the outcome of
						      * get(PRESENT) && (get(ENABLED) || get(DEFAULT))
						      */
	DOCA_APSH_PRIVILEGES_WINDOWS_PRESENT = 1000, /**< privilege present flag */
	DOCA_APSH_PRIVILEGES_WINDOWS_ENABLED = 1001, /**< privilege enabled flag */
	DOCA_APSH_PRIVILEGES_WINDOWS_DEFAULT = 1002, /**< privilege enabledbydefault flag */
};

/** @brief privilege process pid */
typedef uint32_t DOCA_APSH_PRIVILEGES_PID_TYPE;
/** @brief privilege name type */
typedef const char *DOCA_APSH_PRIVILEGES_NAME_TYPE;
/** @brief privilege is on type */
typedef bool DOCA_APSH_PRIVILEGES_IS_ON_TYPE;
/** @brief privilege windows present type */
typedef bool DOCA_APSH_PRIVILEGES_WINDOWS_PRESENT_TYPE;
/** @brief privilege windows enabled type */
typedef bool DOCA_APSH_PRIVILEGES_WINDOWS_ENABLED_TYPE;
/** @brief privilege windows enabled by default type */
typedef bool DOCA_APSH_PRIVILEGES_WINDOWS_DEFAULT_TYPE;

/**
 * @brief doca app shield envars attributes
 */
enum doca_apsh_envar_attr {
	DOCA_APSH_ENVARS_PID = 0,	       /**< envars pid */
	DOCA_APSH_ENVARS_VARIABLE = 2,	       /**< envars variable */
	DOCA_APSH_ENVARS_VALUE = 3,	       /**< envars value */
	DOCA_APSH_ENVARS_WINDOWS_BLOCK = 1000, /**< envars windows environment block address */
};

/** @brief envars pid type */
typedef uint32_t DOCA_APSH_ENVARS_PID_TYPE;
/** @brief envars variable type */
typedef const char *DOCA_APSH_ENVARS_VARIABLE_TYPE;
/** @brief envars value type */
typedef const char *DOCA_APSH_ENVARS_VALUE_TYPE;
/** @brief envars windows block address type */
typedef uint64_t DOCA_APSH_ENVARS_WINDOWS_BLOCK_TYPE;

/**
 * @brief doca app shield LDR-Modules attributes
 */
enum doca_apsh_ldrmodule_attr {
	DOCA_APSH_LDRMODULE_PID = 0,			  /**< ldrmodule process pid */
	DOCA_APSH_LDRMODULE_BASE_ADDRESS = 2,		  /**< ldrmodule base address */
	DOCA_APSH_LDRMODULE_LIBRARY_PATH = 3,		  /**< ldrmodule loaded library path */
	DOCA_APSH_LDRMODULE_WINDOWS_DLL_NAME = 1000,	  /**< ldrmodule dll name */
	DOCA_APSH_LDRMODULE_WINDOWS_SIZE_OF_IMAGE = 1001, /**< ldrmodule size of image */
	DOCA_APSH_LDRMODULE_WINDOWS_INLOAD = 1002,	  /**< ldrmodule appear in inload list */
	DOCA_APSH_LDRMODULE_WINDOWS_INMEM = 1003,	  /**< ldrmodule appear in inmem list */
	DOCA_APSH_LDRMODULE_WINDOWS_ININIT = 1004,	  /**< ldrmodule appear in ininit list */
};

/** @brief ldrmodule pid type */
typedef uint32_t DOCA_APSH_LDRMODULE_PID_TYPE;
/** @brief ldrmodule base address type */
typedef uint64_t DOCA_APSH_LDRMODULE_BASE_ADDRESS_TYPE;
/** @brief ldrmodule library path type */
typedef const char *DOCA_APSH_LDRMODULE_LIBRARY_PATH_TYPE;
/** @brief ldrmodule windows dll name type */
typedef const char *DOCA_APSH_LDRMODULE_WINDOWS_DLL_NAME_TYPE;
/** @brief ldrmodule size of image type */
typedef uint32_t DOCA_APSH_LDRMODULE_WINDOWS_SIZE_OF_IMAGE_TYPE;
/** @brief ldrmodule inload type */
typedef bool DOCA_APSH_LDRMODULE_WINDOWS_INLOAD_TYPE;
/** @brief ldrmodule inmem type */
typedef bool DOCA_APSH_LDRMODULE_WINDOWS_INMEM_TYPE;
/** @brief ldrmodule ininit type */
typedef bool DOCA_APSH_LDRMODULE_WINDOWS_ININIT_TYPE;

/**
 * @brief doca app shield handle attributes
 */
enum doca_apsh_handle_attr {
	DOCA_APSH_HANDLE_PID = 0,	  /**< handle process id */
	DOCA_APSH_HANDLE_VALUE = 2,	  /**< handle value */
	DOCA_APSH_HANDLE_TABLE_ENTRY = 3, /**< handle table entry */
	DOCA_APSH_HANDLE_TYPE = 4,	  /**< handle type */
	DOCA_APSH_HANDLE_ACCESS = 5,	  /**< handle access */
	DOCA_APSH_HANDLE_NAME = 6,	  /**< handle name */
};

/** @brief handle pid type */
typedef uint32_t DOCA_APSH_HANDLE_PID_TYPE;
/** @brief handle value type */
typedef uint64_t DOCA_APSH_HANDLE_VALUE_TYPE;
/** @brief handle table entry type */
typedef uint64_t DOCA_APSH_HANDLE_TABLE_ENTRY_TYPE;
/** @brief handle type type */
typedef const char *DOCA_APSH_HANDLE_TYPE_TYPE;
/** @brief handle access type */
typedef uint64_t DOCA_APSH_HANDLE_ACCESS_TYPE;
/** @brief handle name type */
typedef const char *DOCA_APSH_HANDLE_NAME_TYPE;

/**
 * @brief doca app shield process-parameters attributes
 */
enum doca_apsh_process_parameters_attr {
	DOCA_APSH_PROCESS_PARAMETERS_PID = 0,		  /**< process-parameters pid */
	DOCA_APSH_PROCESS_PARAMETERS_CMD_LINE = 1,	  /**< process-parameters command line */
	DOCA_APSH_PROCESS_PARAMETERS_IMAGE_BASE_ADDR = 2, /**< process-parameters image base address */
	DOCA_APSH_PROCESS_PARAMETERS_IMAGE_FULL_PATH = 3, /**< process-parameters image full path */
};

/** @brief process-parameters pid */
typedef uint32_t DOCA_APSH_PROCESS_PARAMETERS_PID_TYPE;
/** @brief process-parameters command line */
typedef const char *DOCA_APSH_PROCESS_PARAMETERS_CMD_LINE_TYPE;
/** @brief process-parameters image base address */
typedef uint64_t DOCA_APSH_PROCESS_PARAMETERS_IMAGE_BASE_ADDR_TYPE;
/** @brief process-parameters image full path */
typedef const char *DOCA_APSH_PROCESS_PARAMETERS_IMAGE_FULL_PATH_TYPE;

/**
 * @brief doca app shield SID (security identifiers) attributes
 */
enum doca_apsh_sid_attr {
	DOCA_APSH_PROCESS_SID_PID = 0,	      /**< SID process id */
	DOCA_APSH_PROCESS_SID_STRING = 1,     /**< SID string */
	DOCA_APSH_PROCESS_SID_ATTRIBUTES = 2, /**< SID attributes flag */
};

/** @brief SID process id */
typedef uint32_t DOCA_APSH_PROCESS_SID_PID_TYPE;
/** @brief SID strings */
typedef const char *DOCA_APSH_PROCESS_SID_STRING_TYPE;
/** @brief SID attributes flag */
typedef uint32_t DOCA_APSH_PROCESS_SID_ATTRIBUTES_TYPE;

/**
 * @brief doca app shield netscan attributes
 */
enum doca_apsh_netscan_attr {
	DOCA_APSH_NETSCAN_PID = 0,	       /**< netscan connection process id */
	DOCA_APSH_NETSCAN_COMM = 1,	       /**< netscan connection process name */
	DOCA_APSH_NETSCAN_PROTOCOL = 2,	       /**< netscan connection protocol */
	DOCA_APSH_NETSCAN_LOCAL_ADDR = 3,      /**< netscan connection local address */
	DOCA_APSH_NETSCAN_REMOTE_ADDR = 4,     /**< netscan connection remote address */
	DOCA_APSH_NETSCAN_LOCAL_PORT = 5,      /**< netscan connection local port */
	DOCA_APSH_NETSCAN_REMOTE_PORT = 6,     /**< netscan connection remote port */
	DOCA_APSH_NETSCAN_STATE = 7,	       /**< netscan connection state */
	DOCA_APSH_NETSCAN_TIME = 8,	       /**< netscan connection creation time - windows only. deprecated - use
						  DOCA_APSH_NETSCAN_WINDOWS_TIME instead */
	DOCA_APSH_NETSCAN_WINDOWS_TIME = 1000, /**< netscan windows connection creation time */
	DOCA_APSH_NETSCAN_LINUX_FD = 2000,     /**< netscan linux connection file descriptor */
	DOCA_APSH_NETSCAN_LINUX_SOCKET_OFFSET = 2001,	   /**< netscan linux connection socket offset */
	DOCA_APSH_NETSCAN_LINUX_FAMILY = 2002,		   /**< netscan linux connection Family */
	DOCA_APSH_NETSCAN_LINUX_TYPE = 2003,		   /**< netscan linux connection Type */
	DOCA_APSH_NETSCAN_LINUX_FILTER = 2004,		   /**< netscan linux connection filter */
	DOCA_APSH_NETSCAN_LINUX_NET_NAMESPACE = 2005,	   /**< netscan linux connection net namespace */
	DOCA_APSH_NETSCAN_LINUX_TCP_BYTES_SENT = 2006,	   /**< netscan linux connection TCP sent bytes */
	DOCA_APSH_NETSCAN_LINUX_TCP_BYTES_ACKED = 2007,	   /**< netscan linux connection TCP acknowledged bytes */
	DOCA_APSH_NETSCAN_LINUX_TCP_BYTES_RECEIVED = 2008, /**< netscan linux connection TCP received bytes */
	DOCA_APSH_NETSCAN_LINUX_TCP_SEGS_IN = 2009,	   /**< netscan linux connection TCP segments in */
	DOCA_APSH_NETSCAN_LINUX_TCP_SEGS_OUT = 2010,	   /**< netscan linux connection TCP segments out */
	DOCA_APSH_NETSCAN_LINUX_TCP_DATA_SEGS_IN = 2011,   /**< netscan linux connection TCP data segments in */
	DOCA_APSH_NETSCAN_LINUX_TCP_DATA_SEGS_OUT = 2012,  /**< netscan linux connection TCP data segments out */

	DOCA_APSH_NETSCAN_LINUX_INTERFACE_NAME = 2013,		/**< netscan linux interface name */
	DOCA_APSH_NETSCAN_LINUX_INTERFACE_IPV4_ARR = 2014,	/**< netscan linux interface IPV4 address array */
	DOCA_APSH_NETSCAN_LINUX_INTERFACE_IPV4_ARR_SIZE = 2015, /** netscan linux interface IPV4 array size */
	DOCA_APSH_NETSCAN_LINUX_INTERFACE_MAC_ARR = 2016,	/**< netscan linux interface mac address array */
	DOCA_APSH_NETSCAN_LINUX_INTERFACE_MAC_ARR_SIZE = 2017,	/** netscan linux interface mac array size */
	DOCA_APSH_NETSCAN_LINUX_INTERFACE_IPV6_ARR = 2018,	/**< netscan linux interface IPV6 address array */
	DOCA_APSH_NETSCAN_LINUX_INTERFACE_IPV6_ARR_SIZE = 2019, /** netscan linux interface IPV6 array size */
};

/** @brief netscan process id */
typedef uint32_t DOCA_APSH_NETSCAN_PID_TYPE;
/** @brief netscan process name */
typedef const char *DOCA_APSH_NETSCAN_COMM_TYPE;
/** @brief netscan connection protocol */
typedef const char *DOCA_APSH_NETSCAN_PROTOCOL_TYPE;
/** @brief netscan connection local address */
typedef const char *DOCA_APSH_NETSCAN_LOCAL_ADDR_TYPE;
/** @brief netscan connection remote address */
typedef const char *DOCA_APSH_NETSCAN_REMOTE_ADDR_TYPE;
/** @brief netscan connection local port */
typedef uint64_t DOCA_APSH_NETSCAN_LOCAL_PORT_TYPE;
/** @brief netscan connection remote port */
typedef uint64_t DOCA_APSH_NETSCAN_REMOTE_PORT_TYPE;
/** @brief netscan connection state */
typedef const char *DOCA_APSH_NETSCAN_STATE_TYPE;
/** @brief netscan windows connection creation time - deprecated */
typedef const char *DOCA_APSH_NETSCAN_TIME_TYPE;
/** @brief netscan windows connection creation time */
typedef const char *DOCA_APSH_NETSCAN_WINDOWS_TIME_TYPE;
/** @brief netscan linux connection file descriptor */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_FD_TYPE;
/** @brief netscan linux connection socket offset */
typedef uint64_t DOCA_APSH_NETSCAN_LINUX_SOCKET_OFFSET_TYPE;
/** @brief netscan linux connection Family */
typedef const char *DOCA_APSH_NETSCAN_LINUX_FAMILY_TYPE;
/** @brief netscan linux connection Type */
typedef const char *DOCA_APSH_NETSCAN_LINUX_TYPE_TYPE;
/** @brief netscan linux connection filter */
typedef const char *DOCA_APSH_NETSCAN_LINUX_FILTER_TYPE;
/** @brief netscan linux connection net namespace */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_NET_NAMESPACE_TYPE;
/** @brief netscan linux connection TCP sent bytes */
typedef uint64_t DOCA_APSH_NETSCAN_LINUX_TCP_BYTES_SENT_TYPE;
/** @brief netscan linux connection TCP acknowledged bytes */
typedef uint64_t DOCA_APSH_NETSCAN_LINUX_TCP_BYTES_ACKED_TYPE;
/** @brief netscan linux connection TCP received bytes */
typedef uint64_t DOCA_APSH_NETSCAN_LINUX_TCP_BYTES_RECEIVED_TYPE;
/** @brief netscan linux connection TCP segments in */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_TCP_SEGS_IN_TYPE;
/** @brief netscan linux connection TCP segments out */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_TCP_SEGS_OUT_TYPE;
/** @brief netscan linux connection TCP data segments in */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_TCP_DATA_SEGS_IN_TYPE;
/** @brief netscan linux connection TCP data segments out */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_TCP_DATA_SEGS_OUT_TYPE;
/** @brief netscan linux interface name */
typedef char *DOCA_APSH_NETSCAN_LINUX_INTERFACE_NAME_TYPE;
/** @brief netscan interface IPV4 array*/
typedef char **DOCA_APSH_NETSCAN_LINUX_INTERFACE_IPV4_ARR_TYPE;
/** @brief netscan linux interface IPV4 array size */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_INTERFACE_IPV4_ARR_SIZE_TYPE;
/** @brief netscan linux interface MAC array*/
typedef char **DOCA_APSH_NETSCAN_LINUX_INTERFACE_MAC_ARR_TYPE;
/** @brief netscan linux interface MAC array size */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_INTERFACE_MAC_ARR_SIZE_TYPE;
/** @brief netscan linux interface IPV6 array*/
typedef char **DOCA_APSH_NETSCAN_LINUX_INTERFACE_IPV6_ARR_TYPE;
/** @brief netscan linux interface IPV6 array size */
typedef uint32_t DOCA_APSH_NETSCAN_LINUX_INTERFACE_IPV6_ARR_SIZE_TYPE;

/**
 * @brief doca app shield interface attributes
 */
enum doca_apsh_interface_attr {
	DOCA_APSH_LINUX_INTERFACE_NAME = 3000,		      /**< linux interface name */
	DOCA_APSH_LINUX_INTERFACE_IPV4_ARR = 3001,	      /**< linux interface IPV4 address array */
	DOCA_APSH_LINUX_INTERFACE_IPV4_PREFIX_LEN_ARR = 3002, /**< linux interface IPV4 prefix_len array */
	DOCA_APSH_LINUX_INTERFACE_IPV4_ARR_SIZE = 3003,	      /**< linux interface IPV4 address array size */
	DOCA_APSH_LINUX_INTERFACE_MAC_ARR = 3004,	      /**< linux interface mac address array */
	DOCA_APSH_LINUX_INTERFACE_MAC_ARR_SIZE = 3005,	      /**< linux interface mac address array size */
	DOCA_APSH_LINUX_INTERFACE_IPV6_ARR = 3006,	      /**< linux interface IPV6 address array */
	DOCA_APSH_LINUX_INTERFACE_IPV6_PREFIX_LEN_ARR = 3007, /**< linux interface IPV6 prefix_len array */
	DOCA_APSH_LINUX_INTERFACE_IPV6_ARR_SIZE = 3008,	      /**< linux interface IPV6 address array size */
	DOCA_APSH_LINUX_INTERFACE_NAMESPACE = 3009	      /**< linux interface namespace */
};

/** @brief linux interface name */
typedef const char *DOCA_APSH_LINUX_INTERFACE_NAME_TYPE;
/** @brief linux interface IPV4 */
typedef char **DOCA_APSH_LINUX_INTERFACE_IPV4_ARR_TYPE;
/** @brief linux interface IPV4 prefix len */
typedef unsigned char *DOCA_APSH_LINUX_INTERFACE_IPV4_PREFIX_LEN_ARR_TYPE;
/** @brief linux IPV4 address array size */
typedef uint32_t DOCA_APSH_LINUX_INTERFACE_IPV4_ARR_SIZE_TYPE;
/** @brief linux interface mac address array */
typedef char **DOCA_APSH_LINUX_INTERFACE_MAC_ARR_TYPE;
/** @brief linux interface mac address array size */
typedef uint32_t DOCA_APSH_LINUX_INTERFACE_MAC_ARR_SIZE_TYPE;
/** @brief linux interface IPV6 */
typedef char **DOCA_APSH_LINUX_INTERFACE_IPV6_ARR_TYPE;
/** @brief linux IPV6 prefix len */
typedef uint32_t *DOCA_APSH_LINUX_INTERFACE_IPV6_PREFIX_LEN_ARR_TYPE;
/** @brief linux IPV6 address array size */
typedef uint32_t DOCA_APSH_LINUX_INTERFACE_IPV6_ARR_SIZE_TYPE;
/** @brief linux namespace */
typedef uint32_t DOCA_APSH_LINUX_INTERFACE_NAMESPACE_TYPE;

/**
 * @brief available doca app shield yara rules
 */
enum doca_apsh_yara_rule {
	DOCA_APSH_YARA_RULE_HELLO_WORLD = 0,		  /**< yara rule that scans for "Hello World". Rule name is
							   *  "Hello_World". */
	DOCA_APSH_YARA_RULE_REFLECTIVE_DLL_INJECTION = 1, /**< yara rule that scans for Reflective Dll Injection
							   *  attack. Rule name is "Reflective_Dll_Injection". */
	DOCA_APSH_YARA_RULE_MIMIKATZ = 2,		  /**< yara rule that scans for Mimikatz process running on
							   *  the system. Rule name is "Mimikatz". */
};

/**
 * @brief doca app shield yara scan type bitmask
 */
enum doca_apsh_yara_scan_type {
	DOCA_APSH_YARA_SCAN_VMA = 1,	   /**< scan all vma tree, override all others */
	DOCA_APSH_YARA_SCAN_HEAP = 1 << 1, /**< scan heap vads */
};

/**
 * @brief doca app shield yara attributes
 */
enum doca_apsh_yara_attr {
	DOCA_APSH_YARA_PID = 0,		      /**< pid of the process */
	DOCA_APSH_YARA_COMM = 1,	      /**< name of the process */
	DOCA_APSH_YARA_RULE = 2,	      /**< rule name */
	DOCA_APSH_YARA_MATCH_WINDOW_ADDR = 3, /**< virtual address of the scan window of the match */
	DOCA_APSH_YARA_MATCH_WINDOW_LEN = 4,  /**< length of the scan window of the match */
};

/** @brief pid of the process */
typedef uint32_t DOCA_APSH_YARA_PID_TYPE;
/** @brief name of the process */
typedef const char *DOCA_APSH_YARA_COMM_TYPE;
/** @brief rule name */
typedef const char *DOCA_APSH_YARA_RULE_TYPE;
/** @brief virtual address of the scan window of the match */
typedef uint64_t DOCA_APSH_YARA_MATCH_WINDOW_ADDR_TYPE;
/** @brief length of the scan window of the match */
typedef uint64_t DOCA_APSH_YARA_MATCH_WINDOW_LEN_TYPE;

/**
 * @brief doca app shield injection detect attributes
 */
enum doca_apsh_injection_detect_attr {
	DOCA_APSH_INJECTION_DETECT_PID,			 /**< suspected injection process id */
	DOCA_APSH_INJECTION_DETECT_VAD_START,		 /**< suspected injection VAD start address */
	DOCA_APSH_INJECTION_DETECT_VAD_END,		 /**< suspected injection VAD end address */
	DOCA_APSH_INJECTION_DETECT_VAD_PROTECTION,	 /**< suspected injection VAD protection */
	DOCA_APSH_INJECTION_DETECT_VAD_TAG,		 /**< suspected injection VAD pool tag */
	DOCA_APSH_INJECTION_DETECT_VAD_FILE_PATH,	 /**< suspected injection VAD file path */
	DOCA_APSH_INJECTION_DETECT_SUSPECTED_AREA_START, /**< suspected injection suspected area start */
	DOCA_APSH_INJECTION_DETECT_SUSPECTED_AREA_END,	 /**< suspected injection suspected area end */
};

/** @brief injection detect pid type */
typedef uint32_t DOCA_APSH_INJECTION_DETECT_PID_TYPE;
/** @brief injection detect VAD start address type */
typedef uint64_t DOCA_APSH_INJECTION_DETECT_VAD_START_TYPE;
/** @brief injection detect VAD end address type */
typedef uint64_t DOCA_APSH_INJECTION_DETECT_VAD_END_TYPE;
/** @brief injection detect VAD protection type */
typedef const char *DOCA_APSH_INJECTION_DETECT_VAD_PROTECTION_TYPE;
/** @brief injection detect VAD pool tag type */
typedef const char *DOCA_APSH_INJECTION_DETECT_VAD_TAG_TYPE;
/** @brief injection detect VAD file path type */
typedef const char *DOCA_APSH_INJECTION_DETECT_VAD_FILE_PATH_TYPE;
/** @brief injection detect suspected area start type */
typedef uint64_t DOCA_APSH_INJECTION_DETECT_SUSPECTED_AREA_START_TYPE;
/** @brief injection detect suspected area end type */
typedef uint64_t DOCA_APSH_INJECTION_DETECT_SUSPECTED_AREA_END_TYPE;

/**
 * @brief doca app shield process attributes
 */
enum doca_apsh_container_attr {
	DOCA_APSH_CONTAINER_ID = 0, /**< container id */
};

/** @brief container id type */
typedef const char *DOCA_APSH_CONTAINER_ID_TYPE;

/**
 * @brief doca app shield process file details attributes
 */
enum doca_apsh_proc_file_details_attr {
	DOCA_APSH_PROCESS_FILE_DETAILS_PID = 0,		  /**< process file details pid */
	DOCA_APSH_PROCESS_FILE_DETAILS_PATH = 1,	  /**< process file details path */
	DOCA_APSH_PROCESS_FILE_DETAILS_SHA1 = 2,	  /**< process file details sha1 */
	DOCA_APSH_PROCESS_FILE_DETAILS_SHA256 = 3,	  /**< process file details sha256 */
	DOCA_APSH_PROCESS_FILE_DETAILS_INODE = 4,	  /**< process file details inode number */
	DOCA_APSH_PROCESS_FILE_DETAILS_SIZE = 5,	  /**< process file details file size */
	DOCA_APSH_PROCESS_FILE_DETAILS_ELF_TYPE = 6,	  /**< process file details elf type */
	DOCA_APSH_PROCESS_FILE_DETAILS_INODE_ADDRESS = 7, /**< process file details inode address */
};

/** @brief process file details pid type */
typedef uint32_t DOCA_APSH_PROCESS_FILE_DETAILS_PID_TYPE;
/** @brief process file details path type */
typedef const char *DOCA_APSH_PROCESS_FILE_DETAILS_PATH_TYPE;
/** @brief process file details sha1 type */
typedef const char *DOCA_APSH_PROCESS_FILE_DETAILS_SHA1_TYPE;
/** @brief process file details sha256 type */
typedef const char *DOCA_APSH_PROCESS_FILE_DETAILS_SHA256_TYPE;
/** @brief process file details inode number type */
typedef uint64_t DOCA_APSH_PROCESS_FILE_DETAILS_INODE_TYPE;
/** @brief process file details file size type */
typedef uint64_t DOCA_APSH_PROCESS_FILE_DETAILS_SIZE_TYPE;
/** @brief process file details elf type type */
typedef const char *DOCA_APSH_PROCESS_FILE_DETAILS_ELF_TYPE_TYPE;
/** @brief process file details inode address type */
typedef uint64_t DOCA_APSH_PROCESS_FILE_DETAILS_INODE_ADDRESS_TYPE;

#ifdef __cplusplus
}
#endif

/** @} */

#endif
