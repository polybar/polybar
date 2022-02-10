#pragma once

#include "common.hpp"
#include "components/types.hpp"
#include "utils/color.hpp"

POLYBAR_NS

namespace tags {

  enum class attribute { NONE = 0, UNDERLINE, OVERLINE };

  enum class attr_activation { NONE, ON, OFF, TOGGLE };

  enum class syntaxtag {
    A, // mouse action
    B, // background color
    F, // foreground color
    T, // font index
    O, // pixel offset
    R, // flip colors
    o, // overline color
    u, // underline color
    P, // Polybar control tag
    l, // Left alignment
    r, // Right alignment
    c, // Center alignment
  };

  /**
   * Values for polybar control tags
   *
   * %{P...} tags are tags for internal polybar control commands, they are not
   * part of the public interface
   */
  enum class controltag {
    NONE = 0,
    R, // Reset all open tags (B, F, T, o, u). Used at module edges
    t  // special tag for trays
  };

  enum class color_type { RESET = 0, COLOR };

  struct color_value {
    /**
     * ARGB color.
     *
     * Only used if type == COLOR
     */
    rgba val{};
    color_type type;
  };

  /**
   * Stores information about an action
   *
   * The actual command string is stored in element.data
   */
  struct action_value {
    /**
     * NONE is only allowed for closing tags
     */
    mousebtn btn;
    bool closing;
  };

  enum class tag_type { ATTR, FORMAT };

  union tag_subtype {
    syntaxtag format;
    attr_activation activation;
  };

  struct tag {
    tag_type type;
    tag_subtype subtype;
    union {
      /**
       * Used for 'F', 'G', 'o', 'u' formatting tags.
       */
      color_value color;
      /**
       * For for 'A' tags
       */
      action_value action;
      /**
       * For for 'T' tags
       */
      int font;
      /**
       * For for 'O' tags
       */
      extent_val offset;
      /**
       * For for 'P' tags
       */
      controltag ctrl;

      /**
       * For attribute activations ((-|+|!)(o|u))
       */
      attribute attr;
    };
  };

  struct element {
    element(){};
    element(const string&& text) : data{std::move(text)}, is_tag{false} {};

    string data{};
    tag tag_data{};
    bool is_tag{false};
  };

  using format_string = vector<element>;

} // namespace tags

POLYBAR_NS_END
