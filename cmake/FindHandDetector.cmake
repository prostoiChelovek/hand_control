find_path(HandDetector_INCLUDE_DIR Finger.h utils.h Hand.h HandDetector.h
        /usr/local/include/handDetector)
find_library(HandDetector_LIBRARY NAMES handDetector PATH /usr/local/lib)

if (HandDetector_INCLUDE_DIR AND HandDetector_LIBRARY)
    SET(HandDetector_FOUND true)
endif (HandDetector_INCLUDE_DIR AND HandDetector_LIBRARY)