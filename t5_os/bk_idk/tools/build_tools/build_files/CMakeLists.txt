cmake_minimum_required(VERSION 3.5)
project(beken-armino C CXX ASM)

if(CMAKE_CURRENT_LIST_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Current directory '${CMAKE_CURRENT_LIST_DIR}' is not buildable. "
    "Change directories to one of the example projects in '${CMAKE_CURRENT_LIST_DIR}/examples' and try "
    "again.")
endif()

unset(compile_options)
unset(c_compile_options)
unset(cxx_compile_options)
unset(compile_definitions)
unset(link_options)

# Add the following build specifications here, since these seem to be dependent
# on config values on the root Kconfig.
# TODO add compile optimization to Kconfig
if(NOT BOOTLOADER_BUILD)
    if(CONFIG_COMPILER_OPTIMIZATION_SIZE)
        list(APPEND compile_options "-Os")
        list(APPEND compile_options "-freorder-blocks")
    elseif(CONFIG_COMPILER_OPTIMIZATION_DEFAULT)
        list(APPEND compile_options "-Og")
    elseif(CONFIG_COMPILER_OPTIMIZATION_NONE)
        list(APPEND compile_options "-O0")
    elseif(CONFIG_COMPILER_OPTIMIZATION_PERF)
        list(APPEND compile_options "-O2")
    endif()

else()  # BOOTLOADER_BUILD

    if(CONFIG_BOOTLOADER_COMPILER_OPTIMIZATION_SIZE)
        list(APPEND compile_options "-Os")
        list(APPEND compile_options "-freorder-blocks")
    elseif(CONFIG_BOOTLOADER_COMPILER_OPTIMIZATION_DEBUG)
        list(APPEND compile_options "-Og")
    elseif(CONFIG_BOOTLOADER_COMPILER_OPTIMIZATION_NONE)
        list(APPEND compile_options "-O0")
    elseif(CONFIG_BOOTLOADER_COMPILER_OPTIMIZATION_PERF)
        list(APPEND compile_options "-O2")
    endif()

endif()

# TODO add C++ exception to Kconfig
if(CONFIG_COMPILER_CXX_EXCEPTIONS)
    list(APPEND cxx_compile_options "-fexceptions")
else()
    list(APPEND cxx_compile_options "-fno-exceptions")
endif()

if(CONFIG_COMPILER_CXX_RTTI)
    list(APPEND cxx_compile_options "-frtti")
else()
    list(APPEND cxx_compile_options "-fno-rtti")
    list(APPEND link_options "-fno-rtti")           # used to invoke correct multilib variant (no-rtti) during linking
endif()

if(CONFIG_COMPILER_DISABLE_GCC8_WARNINGS)
    list(APPEND compile_options "-Wno-parentheses"
                                "-Wno-sizeof-pointer-memaccess"
                                "-Wno-clobbered")

    list(APPEND compile_options "-Wno-format-overflow"
                                "-Wno-stringop-truncation"
                                "-Wno-misleading-indentation"
                                "-Wno-cast-function-type"
                                "-Wno-implicit-fallthrough"
                                "-Wno-unused-const-variable"
                                "-Wno-switch-unreachable"
                                "-Wno-format-truncation"
                                "-Wno-memset-elt-size"
                                "-Wno-int-in-bool-context")
endif()

if(CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_DISABLE)
    list(APPEND compile_definitions "-DNDEBUG")
endif()

if(CONFIG_COMPILER_STACK_CHECK_MODE_NORM)
    list(APPEND compile_options "-fstack-protector")
elseif(CONFIG_COMPILER_STACK_CHECK_MODE_STRONG)
    list(APPEND compile_options "-fstack-protector-strong")
elseif(CONFIG_COMPILER_STACK_CHECK_MODE_ALL)
    list(APPEND compile_options "-fstack-protector-all")
endif()

if(CONFIG_COMPILER_DUMP_RTL_FILES)
    list(APPEND compile_options "-fdump-rtl-expand")
endif()

list(APPEND link_options "-fno-lto")

list(APPEND compile_options "-Wno-builtin-macro-redefined")

armino_build_set_property(COMPILE_OPTIONS "${compile_options}" APPEND)
armino_build_set_property(C_COMPILE_OPTIONS "${c_compile_options}" APPEND)
armino_build_set_property(CXX_COMPILE_OPTIONS "${cxx_compile_options}" APPEND)
armino_build_set_property(COMPILE_DEFINITIONS "${compile_definitions}" APPEND)
armino_build_set_property(LINK_OPTIONS "${link_options}" APPEND)

armino_build_get_property(build_component_targets __BUILD_COMPONENT_TARGETS)

# Add each component as a subdirectory, processing each component's CMakeLists.txt
foreach(component_target ${build_component_targets})
    __component_get_property(dir ${component_target} COMPONENT_DIR)
    __component_get_property(_name ${component_target} COMPONENT_NAME)
    __component_get_property(prefix ${component_target} __PREFIX)
    __component_get_property(alias ${component_target} COMPONENT_ALIAS)
    set(COMPONENT_NAME ${_name})
    set(COMPONENT_DIR ${dir})
    set(COMPONENT_ALIAS ${alias})
    set(COMPONENT_PATH ${dir}) # for backward compatibility only, COMPONENT_DIR is preferred
    armino_build_get_property(build_prefix __PREFIX)
    armino_build_get_property(include_prefix __INCLUDE_PREFIX)
    set(__armino_component_context 1)
	__component_get_property(component_add_subdired ${component_target} __COMPONENT_ADD_SUBDIRED)
	if(NOT "${component_add_subdired}" STREQUAL 1)
		if(NOT prefix STREQUAL build_prefix AND NOT prefix STREQUAL __INCLUDE_PREFIX)
			add_subdirectory(${dir} ${prefix}_${_name})
		else()
			add_subdirectory(${dir} ${_name})
		endif()
		__component_set_property(${component_target} __COMPONENT_ADD_SUBDIRED 1)
	endif()
    set(__armino_component_context 0)
endforeach()

#TODO put all public API includes to module
