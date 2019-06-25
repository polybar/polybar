find_package_impl("cairo-fc" "CairoFC" "")

if(CairoFC_FOUND AND NOT TARGET Cairo::CairoFC)
  create_imported_target("Cairo::CairoFC" "${CairoFC_INCLUDE_DIR}" "${CairoFC_LIBRARY}")
endif()
