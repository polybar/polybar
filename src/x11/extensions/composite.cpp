#include "x11/extensions/composite.hpp"
#include "errors.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace composite_util {
  /**
   * Query for the XCOMPOSITE extension
   */
  void query_extension(connection& conn) {
    conn.composite().query_version(XCB_COMPOSITE_MAJOR_VERSION, XCB_COMPOSITE_MINOR_VERSION);

    if (!conn.extension<xpp::composite::extension>()->present) {
      throw application_error("Missing X extension: Composite");
    }
  }
}

POLYBAR_NS_END
