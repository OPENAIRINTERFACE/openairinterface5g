# Declare variables for path prefixes for different types of files.
# NB: depends on 'multi_kernel' and 'uninstall_target'.

# fill_install_prefixes(<project_name> <project_prefix>
#     [BASE_INSTALL_PREFIX <base_install_prefix>]
#     [KERNEL]
# )
#
# Setup variables <project_prefix>_* to the install prefixes for
# different project components.
# Precisely, variables with next sufficies are set:
#  INSTALL_PREFIX_EXEC - executables
#  INSTALL_PREFIX_READONLY - readonly files
#  INSTALL_PREFIX_GLOBAL_CONF - global configuration files
#  INSTALL_PREFIX_PREFIX_LIB - libraries
#  INSTALL_INCLUDE_DIR - include directory(for flags to compiler)
#  INSTALL_PREFIX_INCLUDE - include files
#  INSTALL_PREFIX_TEMP_SESSION - temporary files(exists until system restarts)
#  INSTALL_PREFIX_TEMP - temporary files(preserved even when system restats)
#  INSTALL_PREFIX_STATE - files which describe current state of the project.
#  INSTALL_PREFIX_CACHE - cache files
#  INSTALL_PREFIX_VAR - other modifiable files
#  INSTALL_PREFIX_DOC - documentation files
#  INSTALL_PREFIX_EXAMPLES - documentation files
#
# With 'KERNEL' option enabled paths for kernel-related files also set:
#  INSTALL_KINCLUDE_DIR - directory for include when build kernel components
#  INSTALL_PREFIX_KINCLUDE - include files for the kernel.
#
# Additionally, with 'KERNEL' option enabled, several variables are set to
# paths, which contains "%kernel%" pattern.
# These paths are intended for kernel-dependent files; for make paths
# complete one should replace "%kernel%" substring with version of the
# kernel.
# Next kernel-dependend paths are set(sufficies only):
#  KERNEL_INSTALL_PREFIX_KMODULE - directory for install kernel modules
#  KERNEL_INSTALL_PREFIX_KSYMVERS - directory for install kernel modules' symvers files.
#  KERNEL_INSTALL_INCLUDE_KERNEL_DIR - include directory with kernel-dependent headers.
#  KERNEL_INSTALL_PREFIX_INCLUDE_KERNEL - include files, which depends from kernel.
#
# Iff option 'COMMON_INSTALL_PREFIX' is given, all paths above are
# calculated using <base_install_prefix> as base prefix.
# Otherwise, CMAKE_INSTALL_PREFIX is used for that purpose.
#
# Additionally,
#  INSTALL_TYPE
# variable(suffix) is set to one of:
#  - "GLOBAL_OPT" - install into "/opt",
#  - "GLOBAL" - global installation except one into "/opt",
#  - "LOCAL" - local installation.
function(fill_install_prefixes project_name project_prefix)
    cmake_parse_arguments(fip "KERNEL" "BASE_INSTALL_PREFIX" "" ${ARGN})
    if(fip_UNPARSED_ARGUMENTS)
    list(GET fip_UNPARSED_ARGUMENTS 0 exceeded_arg)
    message(SEND_ERROR "Exceeded argument: ${exceeded_arg}")
    endif(fip_UNPARSED_ARGUMENTS)
    if(NOT fip_BASE_INSTALL_PREFIX)
    set(fip_BASE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    endif(NOT fip_BASE_INSTALL_PREFIX)

    # Follow conventions about paths listed in
    #   devel-docs/general/path_conventions.txt
    # in kedr-devel package.

    # Determine type of installation
    if(fip_BASE_INSTALL_PREFIX MATCHES "^/opt")
	set(fip_INSTALL_TYPE "GLOBAL_OPT")
    elseif(fip_BASE_INSTALL_PREFIX MATCHES "^/usr"
	OR fip_BASE_INSTALL_PREFIX STREQUAL "/"
    )
	set(fip_INSTALL_TYPE "GLOBAL")
    else()
	set(fip_INSTALL_TYPE "LOCAL")
    endif()

    # 1
    if(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
	set(fip_INSTALL_PREFIX_EXEC "/opt/${project_name}/bin")
    else(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
	set(fip_INSTALL_PREFIX_EXEC "${fip_BASE_INSTALL_PREFIX}/bin")
    endif(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
    # 2
    set(fip_INSTALL_PREFIX_EXEC_AUX
	"${fip_BASE_INSTALL_PREFIX}/lib/${project_name}"
    )
    # 3
    set(fip_INSTALL_PREFIX_READONLY
        "${fip_BASE_INSTALL_PREFIX}/share/${project_name}"
    )
    # 4
    set(fip_INSTALL_PREFIX_MANPAGE
        "${fip_BASE_INSTALL_PREFIX}/share/man"
    )
    # 5
    if(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
	set(fip_INSTALL_PREFIX_GLOBAL_CONF "/etc/opt/${project_name}")
    elseif(fip_INSTALL_TYPE STREQUAL "GLOBAL")
	set(fip_INSTALL_PREFIX_GLOBAL_CONF "/etc/${project_name}")
    else(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
	set(fip_INSTALL_PREFIX_GLOBAL_CONF
	    "${fip_BASE_INSTALL_PREFIX}/etc/${project_name}"
	)
    endif(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
    # 6
    set(fip_INSTALL_PREFIX_LIB "${fip_BASE_INSTALL_PREFIX}/lib")
    # 7
    set(fip_INSTALL_PREFIX_LIB_AUX
        "${fip_BASE_INSTALL_PREFIX}/lib/${project_name}"
    )
    # 8
    set(fip_INSTALL_INCLUDE_DIR "${fip_BASE_INSTALL_PREFIX}/include")

    set(fip_INSTALL_PREFIX_INCLUDE
        "${fip_INSTALL_INCLUDE_DIR}/${project_name}"
    )
    # 9
    set(fip_INSTALL_PREFIX_TEMP_SESSION "/tmp/${project_name}")
    # 10
    if(fip_INSTALL_TYPE MATCHES "GLOBAL")
	set(fip_INSTALL_PREFIX_TEMP "/var/tmp/${project_name}")
    else(fip_INSTALL_TYPE MATCHES "GLOBAL")
	set(fip_INSTALL_PREFIX_TEMP
            "${fip_BASE_INSTALL_PREFIX}/var/tmp/${project_name}"
	)
    endif(fip_INSTALL_TYPE MATCHES "GLOBAL")
    # 11
    if(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
        set(fip_INSTALL_PREFIX_STATE
	    "/var/opt/${project_name}/lib/${project_name}"
	)
    elseif(fip_INSTALL_TYPE STREQUAL "GLOBAL")
        set(fip_INSTALL_PREFIX_STATE "/var/lib/${project_name}")
    else(fip_INSTALL_TYPE STREQUAL "GLOBAL")
	set(fip_INSTALL_PREFIX_STATE
	    "${fip_BASE_INSTALL_PREFIX}/var/lib/${project_name}"
	)
    endif(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
    # 12
    if(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
        set(fip_INSTALL_PREFIX_CACHE
	    "/var/opt/${project_name}/cache/${project_name}"
	)
    elseif(fip_INSTALL_TYPE STREQUAL "GLOBAL")
	set(fip_INSTALL_PREFIX_CACHE "/var/cache/${project_name}")
    else(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
	set(fip_INSTALL_PREFIX_CACHE
	    "${fip_BASE_INSTALL_PREFIX}/var/cache/${project_name}"
	)
    endif(fip_INSTALL_TYPE STREQUAL "GLOBAL_OPT")
    # 13
    if(fip_INSTALL_TYPE MATCHES "GLOBAL")
        set(fip_INSTALL_PREFIX_VAR "/var/opt/${project_name}")
    else(fip_INSTALL_TYPE MATCHES "GLOBAL")
	set(fip_INSTALL_PREFIX_VAR
	    "${fip_BASE_INSTALL_PREFIX}/var/${project_name}"
	)
    endif(fip_INSTALL_TYPE MATCHES "GLOBAL")
    # 14
    set(fip_INSTALL_PREFIX_DOC
	"${fip_BASE_INSTALL_PREFIX}/share/doc/${project_name}"
    )

    # Set derivative install path and prefixes

    # additional, 4
    set(fip_INSTALL_PREFIX_EXAMPLES
    "${fip_INSTALL_PREFIX_READONLY}/examples")

    # Export symbols to the outer scope
    foreach(suffix
	INSTALL_TYPE
	INSTALL_PREFIX_EXEC
	INSTALL_PREFIX_EXEC_AUX
	INSTALL_PREFIX_READONLY
	INSTALL_PREFIX_MANPAGE
	INSTALL_PREFIX_GLOBAL_CONF
	INSTALL_PREFIX_LIB
	INSTALL_PREFIX_LIB_AUX
	INSTALL_INCLUDE_DIR
	INSTALL_PREFIX_INCLUDE
	INSTALL_PREFIX_TEMP_SESSION
	INSTALL_PREFIX_TEMP
	INSTALL_PREFIX_STATE
	INSTALL_PREFIX_CACHE
	INSTALL_PREFIX_VAR
	INSTALL_PREFIX_DOC
	INSTALL_PREFIX_EXAMPLES
    )
	set(${project_prefix}_${suffix} "${fip_${suffix}}" PARENT_SCOPE)
    endforeach(suffix)
    
    if(fip_KERNEL)
	# Set derivative install path and prefixes
	# additional, 1
	if(fip_INSTALL_TYPE MATCHES GLOBAL)
	    set(fip_KERNEL_INSTALL_PREFIX_KMODULE
		"/lib/modules/%kernel%/extra"
	    )
	else(fip_INSTALL_TYPE MATCHES GLOBAL)
	    set(fip_KERNEL_INSTALL_PREFIX_KMODULE
		"${fip_INSTALL_PREFIX_LIB}/modules/%kernel%/extra"
	    )
	endif(fip_INSTALL_TYPE MATCHES GLOBAL)

	# additional, 2
	set(fip_KERNEL_INSTALL_PREFIX_KSYMVERS
	    "${fip_INSTALL_PREFIX_LIB}/modules/%kernel%/symvers"
	)
	# additional, 3
	set(fip_INSTALL_KINCLUDE_DIR "${fip_INSTALL_INCLUDE_DIR}")
	set(fip_INSTALL_PREFIX_KINCLUDE "${fip_INSTALL_PREFIX_INCLUDE}")

	# Kernel include files, which depends from kernel version.
	# This prefix is not listed in path conventions.
	set(fip_KERNEL_INSTALL_INCLUDE_KERNEL_DIR
	    "${fip_BASE_INSTALL_PREFIX}/include-kernel/%kernel%"
	)

	set(fip_KERNEL_INSTALL_PREFIX_INCLUDE_KERNEL
	    "${fip_KERNEL_INSTALL_INCLUDE_KERNEL_DIR}/${project_name}-kernel"
	)

	# Export symbols to the outer scope
	foreach(suffix
	    KERNEL_INSTALL_PREFIX_KMODULE
	    KERNEL_INSTALL_PREFIX_KSYMVERS
	    INSTALL_KINCLUDE_DIR
	    INSTALL_PREFIX_KINCLUDE
	    KERNEL_INSTALL_INCLUDE_KERNEL_DIR
	    KERNEL_INSTALL_PREFIX_INCLUDE_KERNEL
	)
	    set(${project_prefix}_${suffix} "${fip_${suffix}}" PARENT_SCOPE)
	endforeach(suffix)
    endif(fip_KERNEL)
endfunction(fill_install_prefixes project_name project_prefix)


########################################################################
# Kernel-dependent paths.
#
# Some deliverables may depends on linux kernel.
#
# For make "one user installation for several kernels" paradigm works,
# installation directory for that deliverables should include
# kernel-version part(like "3.10.2-generic").
#
# So, components installed by user installation can determine at runtime,
# which kernel-dependent deliverable should be used on currently loaded system.
# They do selection using 'uname -r' request.
#
# For define variables represented kernel-dependent directories,
# we use strings containing "%kernel%" stem.

#  kernel_path(kernel_version RESULT_VARIABLE pattern ...)
#
# Form concrete path representation from kernel-dependent pattern(s).
# Replace occurence of %kernel% in pattern(s) with given @kernel_version string.
# Result is stored in the RESULT_VARIABLE.
#
# @kernel_version may be concrete version of the kernel,
# or variable reference in some language.
macro(kernel_path kernel_version RESULT_VARIABLE pattern)
    string(REPLACE "%kernel%" "${kernel_version}" ${RESULT_VARIABLE} ${pattern} ${ARGN})
endmacro(kernel_path kernel_version RESULT_VARIABLE pattern)
