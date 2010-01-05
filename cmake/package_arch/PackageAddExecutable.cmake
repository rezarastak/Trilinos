
INCLUDE(PackageAddExecutableTestHelpers)
INCLUDE(PackageGeneralMacros)

INCLUDE(PrintVar)
INCLUDE(AppendSet)
INCLUDE(ParseVariableArguments)


#
# PACKAGE_ADD_EXECUTABLE(...): Function that adds a test/example executable.
#
# PACKAGE_ADD_EXECUTABLE(
#   <execName>
#   SOURCES <src1> <src2> ...
#   [ NOEXEPREFIX ]
#   [ DIRECTORY <dir> ]
#   [ DEPLIBS <lib1> <lib2> ... ]
#   [ COMM [serial] [mpi] ]
#   [ LINKER_LANGUAGE [C|CXX|Fortran] ]
#   [ ADD_DIR_TO_NAME ]
#   [ DEFINES <-DSOMEDEFINE>]
#   
#
# 

FUNCTION(PACKAGE_ADD_EXECUTABLE EXE_NAME)
   
  #
  # A) Parse the input arguments
  #

  PARSE_ARGUMENTS(
    #prefix
    PARSE
    #lists
    "SOURCES;DIRECTORY;DEPLIBS;COMM;LINKER_LANGUAGE;DEFINES"
    #options
    "NOEXEPREFIX;ADD_DIR_TO_NAME"
    ${ARGN}
    )

  PACKAGE_PROCESS_COMM_ARGS(ADD_SERIAL_EXE  ADD_MPI_EXE  ${PARSE_COMM})
  IF (NOT ADD_SERIAL_EXE AND NOT ADD_MPI_EXE)
    RETURN()
  ENDIF()

  #
  # B) Add the executable
  #
  
  SET (EXE_SOURCES)
  SET(EXE_BINARY_NAME ${EXE_NAME})
  
  #If requested create a modifier for the name that will be inserted between the package name 
  #and the given name or exe_name for the test
  IF(PARSE_ADD_DIR_TO_NAME)
    SET(DIRECTORY_NAME "")
    PACKAGE_CREATE_NAME_FROM_CURRENT_SOURCE_DIRECTORY(DIRECTORY_NAME)
    SET(EXE_BINARY_NAME ${DIRECTORY_NAME}_${EXE_BINARY_NAME})
  ENDIF()
  
  IF(DEFINED PACKAGE_NAME AND NOT PARSE_NOEXEPREFIX)
    SET(EXE_BINARY_NAME ${PACKAGE_NAME}_${EXE_BINARY_NAME})
  ENDIF()

  # If exe is in subdirectory prepend that dir name to the source files
  IF(PARSE_DIRECTORY ) 
    FOREACH( SOURCE_FILE ${PARSE_SOURCES} )
      IF(IS_ABSOLUTE ${SOURCE_FILE})
        SET (EXE_SOURCES ${EXE_SOURCES} ${SOURCE_FILE})
      ELSE()
        SET (EXE_SOURCES ${EXE_SOURCES} ${PARSE_DIRECTORY}/${SOURCE_FILE})
      ENDIF()
    ENDFOREACH( )
  ELSE()
    FOREACH( SOURCE_FILE ${PARSE_SOURCES} )
      SET (EXE_SOURCES ${EXE_SOURCES} ${SOURCE_FILE})
    ENDFOREACH( )
  ENDIF()

  IF(${PROJECT_NAME}_VERBOSE_CONFIGURE)
    MESSAGE("")
    MESSAGE("PACKAGE_ADD_EXECUTABLE: ${EXE_BINARY_NAME}")
  ENDIF()

  FOREACH(DEPLIB ${PARSE_DEPLIBS})
    IF (${DEPLIB}_INCLUDE_DIRS)
      IF (${PROJECT_NAME}_VERBOSE_CONFIGURE)
        MESSAGE(STATUS "Adding include directories ${DEPLIB}_INCLUDE_DIRS ...")
        #PRINT_VAR(${DEPLIB}_INCLUDE_DIRS)
      ENDIF()
      INCLUDE_DIRECTORIES(${${DEPLIB}_INCLUDE_DIRS})
    ENDIF()
  ENDFOREACH()

  IF (PARSE_DEFINES)
    ADD_DEFINITIONS(${PARSE_DEFINES})
  ENDIF()

  ADD_EXECUTABLE(${EXE_BINARY_NAME} ${EXE_SOURCES})
  APPEND_GLOBAL_SET(${PACKAGE_NAME}_ALL_TARGETS ${EXE_BINARY_NAME})

  IF(PARSE_LINKER_LANGUAGE)
    SET(LINKER_LANGUAGE ${PARSE_LINKER_LANGUAGE})
  ELSEIF (${PROJECT_NAME}_ENABLE_CXX)
    SET(LINKER_LANGUAGE CXX)
  ELSEIF(${PROJECT_NAME}_ENABLE_C)
    SET(LINKER_LANGUAGE C)
  ELSE()
    SET(LINKER_LANGUAGE)
  ENDIF()

  IF (LINKER_LANGUAGE)
    SET_PROPERTY(TARGET ${EXE_BINARY_NAME} APPEND PROPERTY
      LINKER_LANGUAGE ${LINKER_LANGUAGE})
  ENDIF()

  SET(LINK_LIBS)

  # First, add in the passed in dependent libraries
  IF (PARSE_DEPLIBS)
    APPEND_SET(LINK_LIBS ${PARSE_DEPLIBS})
  ENDIF()
  # 2009/01/09: rabartl: Above, I moved the list of dependent
  # libraries first to get around a problem with test-only libraries
  # creating multiple duplicate libraries on the link line with
  # CMake.

  # Second, add the package's own regular libraries
  APPEND_SET(LINK_LIBS ${${PACKAGE_NAME}_LIBRARIES})

  # Third, add test dependent package libraries
  PACKAGE_GATHER_ENABLED_ITEMS(${PACKAGE_NAME} TEST PACKAGES ALL_DEP_PACKAGES)
  PACKAGE_SORT_AND_APPEND_PATHS_LIBS("${${PROJECT_NAME}_REVERSE_PACKAGES}"
    "${ALL_DEP_PACKAGES}" "" LINK_LIBS)
  
  # Fourth, add dependent test TPL libraries
  PACKAGE_GATHER_ENABLED_ITEMS(${PACKAGE_NAME} TEST TPLS ALL_TPLS)
  PACKAGE_SORT_AND_APPEND_PATHS_LIBS("${${PROJECT_NAME}_REVERSE_TPLS}" "${ALL_TPLS}"
    TPL_ LINK_LIBS)

  # Last, add last_lib to get extra link options on the link line
  IF (${PROJECT_NAME}_EXTRA_LINK_FLAGS)
    APPEND_SET(LINK_LIBS last_lib)
  ENDIF()

  IF (${PROJECT_NAME}_VERBOSE_CONFIGURE)
    PRINT_VAR(LINK_LIBS)
  ENDIF()

  TARGET_LINK_LIBRARIES(${EXE_BINARY_NAME} ${LINK_LIBS})

  IF(PARSE_DIRECTORY)
    SET_TARGET_PROPERTIES( ${EXE_BINARY_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${PARSE_DIRECTORY} )
  ENDIF()

  SET_PROPERTY(TARGET ${EXE_BINARY_NAME} APPEND PROPERTY
    LABELS ${PACKAGE_NAME})

ENDFUNCTION()


#
# Setup include directories and library dependencies
#

IF (${PROJECT_NAME}_VERBOSE_CONFIGURE)
  MESSAGE("PackageAddExecutable.cmake")
  PRINT_VAR(${PACKAGE_NAME}_INCLUDE_DIRS)
  PRINT_VAR(${PACKAGE_NAME}_TEST_INCLUDE_DIRS)
  PRINT_VAR(${PACKAGE_NAME}_LIBRARY_DIRS)
  PRINT_VAR(${PACKAGE_NAME}_TEST_LIBRARY_DIRS)
ENDIF()

INCLUDE_DIRECTORIES(${${PACKAGE_NAME}_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${${PACKAGE_NAME}_TEST_INCLUDE_DIRS})

LINK_DIRECTORIES(${${PACKAGE_NAME}_LIBRARY_DIRS})
LINK_DIRECTORIES(${${PACKAGE_NAME}_TEST_LIBRARY_DIRS})
