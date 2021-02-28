# This module defines an imported target `CURL::libcurl` if libcurl is found
#
# Defines the following Variables (see find_package_impl for more info):
# CURL_FOUND
# CURL_INCLUDE_DIR
# CURL_INCLUDE_DIRS
# CURL_LIBRARY
# CURL_LIBRARIES
# CURL_VERSION
find_package_impl("libcurl" "CURL" "curl/curl.h")

if(CURL_FOUND AND NOT TARGET CURL::libcurl)
  create_imported_target("CURL::libcurl" "${CURL_INCLUDE_DIR}" "${CURL_LIBRARY}")
endif()
