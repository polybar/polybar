#include "x11/extensions/sync.hpp"

#include "errors.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/all.hpp"

POLYBAR_NS

namespace sync_util {
  /**
   * Query for the XSYNC extension
   */
  void query_extension(connection& conn) {
    conn.sync().initialize(XCB_SYNC_MAJOR_VERSION, XCB_SYNC_MINOR_VERSION);

    if (!conn.extension<xpp::sync::extension>()->present) {
      throw application_error("Missing X extension: Sync");
    }
  }
}

POLYBAR_NS_END
