# Define library.

set(sources
    io/Filereader.cpp
    io/FilereaderLp.cpp
    io/FilereaderEms.cpp
    io/FilereaderMps.cpp
    io/HighsIO.cpp
 #   io/HinOut.cpp
    io/HMPSIO.cpp
    io/HToyIO.cpp
    lp_data/HighsLp.cpp
    lp_data/HighsLpUtils.cpp
    lp_data/HighsModel.cpp
 #   simplex/HAPI.cpp
    presolve/Presolve.cpp
    presolve/HPreData.cpp
    simplex/HCrash.cpp
    simplex/HDual.cpp
    simplex/HDualRHS.cpp
    simplex/HDualRow.cpp
    simplex/HDualMulti.cpp
    simplex/HFactor.cpp
    simplex/HMatrix.cpp
    simplex/HModel.cpp
    simplex/HPrimal.cpp
    simplex/HRanging.cpp
 #   io/HinOut.cpp
    io/HMPSIO.cpp
    io/HToyIO.cpp
    io/Filereader.cpp
    io/FilereaderLp.cpp
    io/FilereaderMps.cpp
    io/HighsIO.cpp
    simplex/HVector.cpp
    test/KktCheck.cpp
    test/KktChStep.cpp
    util/HighsSort.cpp
    util/HighsUtils.cpp
    util/stringutil.cpp
)

set(headers
    #HighsOptions.h  # not using at the moment.
    io/Filereader.h
    io/FilereaderLp.h
    io/FilereaderEms.h
    io/FilereaderMps.h
#    io/HinOut.h
    io/HMpsFF.h
    io/HMPSIO.h
    io/HToyIO_C.h
    io/HToyIO.h
    io/HighsIO.h
    io/LoadProblem.h
    lp_data/HConst.h
    lp_data/HighsLp.h
    lp_data/HighsLpUtils.h
    lp_data/HighsModelObject.h
    lp_data/HighsModelObjectUtil.h
    simplex/HApp.h
    simplex/HConst.h
    simplex/HCrash.h
    simplex/HDual.h
    simplex/HDualRow.h
    simplex/HDualRHS.h
    simplex/HFactor.h
    simplex/HMatrix.h
    simplex/HModel.h
    simplex/HPrimal.h
    simplex/HRanging.h
    simplex/HSimplex.h
    simplex/SimplexConst.h
    presolve/Presolve.h
    simplex/HTimerPre.h
    simplex/HVector.h
    test/KktCheck.h
    test/KktChStep.h
    util/stringutil.h
    util/HighsSort.h
    util/HighsTimer.h
    util/HighsUtils.h
)


add_library(libhighs ${sources})

if (Boost_FOUND)
    target_link_libraries(libhighs ${Boost_LIBRARIES})
endif()

# put version information into shared library file
if(${BUILD_SHARED_LIBS})
    set_target_properties(libhighs PROPERTIES
        VERSION ${HIGHS_VERSION_MAJOR}.${HIGHS_VERSION_MINOR}.${HIGHS_VERSION_PATCH}
        SOVERSION ${HIGHS_VERSION_MAJOR}.${HIGHS_VERSION_MINOR})
# create static highs library with pic
else()
    set_target_properties(libhighs PROPERTIES
        POSITION_INDEPENDENT_CODE on)
endif()

# set the install rpath to the installed destination
set_target_properties(highs PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# on UNIX system the 'lib' prefix is automatically added
set_target_properties(libhighs PROPERTIES
    OUTPUT_NAME "highs"
    MACOSX_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# install the header files of highs
install(FILES ${headers} ${CMAKE_BINARY_DIR}/HConfig.h DESTINATION include)

# install the binary and the library to appropriate lcoations and add them to an export group
install(TARGETS libhighs EXPORT highs-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include)

# Add library targets to the build-tree export set
export(TARGETS libhighs
  FILE "${CMAKE_BINARY_DIR}/highs-targets.cmake")

#configure the config file for the build tree
#Either list all the src/* directories here, or put explicit paths in all the #include statements
#M reckons that the latter is more transparent, and I'm incliuned to agree.
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}")
configure_file(${CMAKE_SOURCE_DIR}/highs-config.cmake.in
  "${CMAKE_BINARY_DIR}/highs-config.cmake" @ONLY)

#configure the config file for the install
set(CONF_INCLUDE_DIRS "\${CMAKE_CURRENT_LIST_DIR}/../../../include")
configure_file(${CMAKE_SOURCE_DIR}/highs-config.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/highs-config.cmake" @ONLY)

# install the targets of the highs export group and the config file so that other projects
# can link easily against highs
install(EXPORT highs-targets FILE highs-targets.cmake DESTINATION lib/cmake/highs)
install(FILES "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/highs-config.cmake" DESTINATION lib/cmake/highs)
