FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)

INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../LibThunderVision/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../LibThunderBasic/src)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)