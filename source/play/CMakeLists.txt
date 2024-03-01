file( GLOB APP_SOURCES "*.cpp" )
foreach( sourcefile ${APP_SOURCES} )
    # executable name cut off .cpp
    string( REGEX REPLACE ".+/" "" executablename1 ${sourcefile} )
    string( REPLACE ".cpp" "" executablename ${executablename1} )

    message ("source files: " ${sourcefile} " executable: " ${executablename})
    add_executable( ${executablename} ${sourcefile} ../../src/application/application.cpp )
    # target_link_libraries( ${executablename} ${OpenCL_LIBRARY} rt)
    target_link_libraries(${executablename} PRIVATE lightgbm_objs)
    target_link_libraries(${executablename} PRIVATE ${histograms})
endforeach( sourcefile ${APP_SOURCES} )