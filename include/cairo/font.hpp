#pragma once

#include <cairo/cairo-ft.h>

#include "cairo/types.hpp"
#include "cairo/utils.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "settings.hpp"
#include "utils/math.hpp"
#include "utils/scope.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace cairo {
  /**
   * @brief Global pointer to the Freetype library handler
   */
  static FT_Library g_ftlib;

  /**
   * @brief Abstract font face
   */
  class font {
   public:
    explicit font(cairo_t* cairo, double offset) : m_cairo(cairo), m_offset(offset) {}
    virtual ~font(){};

    virtual string name() const = 0;
    virtual string file() const = 0;
    virtual double offset() const = 0;
    virtual double size(double dpi) const = 0;

    virtual cairo_font_extents_t extents() = 0;

    virtual void use() {
      cairo_set_font_face(m_cairo, cairo_font_face_reference(m_font_face));
    }

    virtual size_t match(utils::unicode_character& character) = 0;
    virtual size_t match(utils::unicode_charlist& charlist) = 0;
    virtual size_t render(const string& text, double x = 0.0, double y = 0.0) = 0;
    virtual void textwidth(const string& text, cairo_text_extents_t* extents) = 0;

   protected:
    cairo_t* m_cairo;
    cairo_font_face_t* m_font_face{nullptr};
    cairo_font_extents_t m_extents{};
    double m_offset{0.0};
  };

  /**
   * @brief Font based on fontconfig/freetype
   */
  class font_fc : public font {
   public:
    explicit font_fc(cairo_t* cairo, FcPattern* pattern, double offset, double dpi_x, double dpi_y) : font(cairo, offset), m_pattern(pattern) {
      cairo_matrix_t fm;
      cairo_matrix_t ctm;
      cairo_matrix_init_scale(&fm, size(dpi_x), size(dpi_y));
      cairo_get_matrix(m_cairo, &ctm);

      auto fontface = cairo_ft_font_face_create_for_pattern(m_pattern);
      auto opts = cairo_font_options_create();
      m_scaled = cairo_scaled_font_create(fontface, &fm, &ctm, opts);
      cairo_font_options_destroy(opts);
      cairo_font_face_destroy(fontface);

      auto status = cairo_scaled_font_status(m_scaled);
      if (status != CAIRO_STATUS_SUCCESS) {
        throw application_error(sstream() << "cairo_scaled_font_create(): " << cairo_status_to_string(status));
      }

      auto lock = make_unique<utils::ft_face_lock>(m_scaled);
      auto face = static_cast<FT_Face>(*lock);

      if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) == FT_Err_Ok) {
        return;
      } else if (FT_Select_Charmap(face, FT_ENCODING_BIG5) == FT_Err_Ok) {
        return;
      } else if (FT_Select_Charmap(face, FT_ENCODING_SJIS) == FT_Err_Ok) {
        return;
      }

      lock.reset();
    }

    ~font_fc() override {
      if (m_scaled != nullptr) {
        cairo_scaled_font_destroy(m_scaled);
      }
      if (m_pattern != nullptr) {
        FcPatternDestroy(m_pattern);
      }
    }

    cairo_font_extents_t extents() override {
      cairo_scaled_font_extents(m_scaled, &m_extents);
      return m_extents;
    }

    string name() const override {
      return property("family");
    }

    string file() const override {
      return property("file");
    }

    double offset() const override {
      return m_offset;
    }

    double size(double dpi) const override {
      bool scalable;
      double px;
      property(FC_SCALABLE, &scalable);
      if (scalable) {
        // convert from pt to px using the provided dpi
        property(FC_SIZE, &px);
        px = static_cast<int>(px * dpi / 72.0 + 0.5);
      } else {
        property(FC_PIXEL_SIZE, &px);
        px = static_cast<int>(px + 0.5);
      }
      return px;
    }

    void use() override {
      cairo_set_scaled_font(m_cairo, m_scaled);
    }

    size_t match(utils::unicode_character& character) override {
      auto lock = make_unique<utils::ft_face_lock>(m_scaled);
      auto face = static_cast<FT_Face>(*lock);
      return FT_Get_Char_Index(face, character.codepoint) ? 1 : 0;
    }

    size_t match(utils::unicode_charlist& charlist) override {
      auto lock = make_unique<utils::ft_face_lock>(m_scaled);
      auto face = static_cast<FT_Face>(*lock);
      size_t available_chars = 0;
      for (auto&& c : charlist) {
        if (FT_Get_Char_Index(face, c.codepoint)) {
          available_chars++;
        } else {
          break;
        }
      }

      return available_chars;
    }

    size_t render(const string& text, double x = 0.0, double y = 0.0) override {
      cairo_glyph_t* glyphs{nullptr};
      cairo_text_cluster_t* clusters{nullptr};
      cairo_text_cluster_flags_t cf{};
      int nglyphs = 0, nclusters = 0;

      string utf8 = string(text);
      auto status = cairo_scaled_font_text_to_glyphs(
          m_scaled, x, y, utf8.c_str(), utf8.size(), &glyphs, &nglyphs, &clusters, &nclusters, &cf);

      if (status != CAIRO_STATUS_SUCCESS) {
        throw application_error(sstream() << "cairo_scaled_font_text_to_glyphs()" << cairo_status_to_string(status));
      }

      size_t bytes = 0;
      for (int g = 0; g < nglyphs; g++) {
        if (glyphs[g].index) {
          bytes += clusters[g].num_bytes;
        } else {
          break;
        }
      }

      if (bytes && bytes < text.size()) {
        cairo_glyph_free(glyphs);
        cairo_text_cluster_free(clusters);

        utf8 = text.substr(0, bytes);
        auto status = cairo_scaled_font_text_to_glyphs(
            m_scaled, x, y, utf8.c_str(), utf8.size(), &glyphs, &nglyphs, &clusters, &nclusters, &cf);

        if (status != CAIRO_STATUS_SUCCESS) {
          throw application_error(sstream() << "cairo_scaled_font_text_to_glyphs()" << cairo_status_to_string(status));
        }
      }

      if (bytes) {
        // auto lock = make_unique<utils::device_lock>(cairo_surface_get_device(cairo_get_target(m_cairo)));
        // if (lock.get()) {
        //   cairo_glyph_path(m_cairo, glyphs, nglyphs);
        // }

        cairo_text_extents_t extents{};
        cairo_scaled_font_glyph_extents(m_scaled, glyphs, nglyphs, &extents);
        cairo_show_text_glyphs(m_cairo, utf8.c_str(), utf8.size(), glyphs, nglyphs, clusters, nclusters, cf);
        cairo_fill(m_cairo);
        cairo_move_to(m_cairo, x + extents.x_advance, 0.0);
      }

      cairo_glyph_free(glyphs);
      cairo_text_cluster_free(clusters);

      return bytes;
    }

    void textwidth(const string& text, cairo_text_extents_t* extents) override {
      cairo_scaled_font_text_extents(m_scaled, text.c_str(), extents);
    }

   protected:
    string property(string&& property) const {
      FcChar8* file;
      if (FcPatternGetString(m_pattern, property.c_str(), 0, &file) == FcResultMatch) {
        return string(reinterpret_cast<char*>(file));
      } else {
        return "";
      }
    }

    void property(string&& property, bool* dst) const {
      FcBool b;
      FcPatternGetBool(m_pattern, property.c_str(), 0, &b);
      *dst = b;
    }

    void property(string&& property, double* dst) const {
      FcPatternGetDouble(m_pattern, property.c_str(), 0, dst);
    }

    void property(string&& property, int* dst) const {
      FcPatternGetInteger(m_pattern, property.c_str(), 0, dst);
    }

   private:
    cairo_scaled_font_t* m_scaled{nullptr};
    FcPattern* m_pattern{nullptr};
  };

  /**
   * Match and create font from given fontconfig pattern
   */
  inline decltype(auto) make_font(cairo_t* cairo, string&& fontname, double offset, double dpi_x, double dpi_y) {
    static bool fc_init{false};
    if (!fc_init && !(fc_init = FcInit())) {
      throw application_error("Could not load fontconfig");
    } else if (FT_Init_FreeType(&g_ftlib) != FT_Err_Ok) {
      throw application_error("Could not load FreeType");
    }

    static auto fc_cleanup = scope_util::make_exit_handler([] {
      FT_Done_FreeType(g_ftlib);
      FcFini();
    });

    auto pattern = FcNameParse((FcChar8*)fontname.c_str());
    FcDefaultSubstitute(pattern);
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);

    FcResult result;
    FcPattern* match = FcFontMatch(nullptr, pattern, &result);
    FcPatternDestroy(pattern);

    if (match == nullptr) {
      throw application_error("Could not load font \"" + fontname + "\"");
    }

#ifdef DEBUG_FONTCONFIG
    FcPatternPrint(match);
#endif

    return make_shared<font_fc>(cairo, match, offset, dpi_x, dpi_y);
  }
}

POLYBAR_NS_END
