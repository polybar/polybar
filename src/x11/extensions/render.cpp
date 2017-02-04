#include "x11/extensions/render.hpp"
#include "errors.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace render_util {
  /**
   * Query for the XRENDER extension
   */
  void query_extension(connection& conn) {
    conn.render().query_version(XCB_RENDER_MAJOR_VERSION, XCB_RENDER_MINOR_VERSION);

    if (!conn.extension<xpp::render::extension>()->present) {
      throw application_error("Missing X extension: Render");
    }
  }
}

POLYBAR_NS_END
