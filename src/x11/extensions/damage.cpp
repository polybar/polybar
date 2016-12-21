#include "x11/extensions/damage.hpp"
#include "errors.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace damage_util {
  /**
   * Query for the XDAMAGE extension
   */
  void query_extension(connection& conn) {
    conn.damage().query_version(XCB_DAMAGE_MAJOR_VERSION, XCB_DAMAGE_MINOR_VERSION);

    if (!conn.extension<xpp::damage::extension>()->present) {
      throw application_error("Missing X extension: Damage");
    }
  }
}

POLYBAR_NS_END
