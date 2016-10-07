#include <string>
#include <memory>
#include <vector>
#include <regex>
#include <boost/algorithm/string/replace.hpp>

#include "bar.hpp"
#include "config.hpp"
#include "exception.hpp"
#include "services/builder.hpp"
#include "utils/string.hpp"
#include "utils/math.hpp"


// Private

std::shared_ptr<Options>& Builder::get_opts()
{
  if (!this->opts)
    this->opts = bar_opts();
  return this->opts;
}

void Builder::tag_open(char tag, std::string value) {
  this->append("%{"+ std::string({tag}) + value +"}");
}

void Builder::tag_close(char tag) {
  this->append("%{"+ std::string({tag}) +"-}");
}

void Builder::align_left() {
  this->tag_open('l', "");
}

void Builder::align_center() {
  this->tag_open('c', "");
}

void Builder::align_right() {
  this->tag_open('r', "");
}

// Public

std::string Builder::flush()
{
  if (this->lazy_closing) {
    while (this->A > 0) this->cmd_close(true);
    while (this->B > 0) this->background_close(true);
    while (this->F > 0) this->color_close(true);
    while (this->T > 0) this->font_close(true);
    while (this->U > 0) this->line_color_close(true);
    while (this->u > 0) this->underline_close(true);
    while (this->o > 0) this->overline_close(true);
  }

  std::string output = this->output.data();
  this->output.clear();

  this->alignment = ALIGN_NONE;

  this->A = 0;
  this->B = 0;
  this->F = 0;
  this->T = 0;
  this->U = 0;
  this->o = 0;
  this->u = 0;

  this->B_value = "";
  this->F_value = "";
  this->U_value = "";
  this->T_value = 1;

  return string::replace_all(output, std::string(BUILDER_SPACE_TOKEN), " ");
}

void Builder::append(std::string text)
{
  std::string str(text);
  auto len = str.length();
  if (len > 2 && str[0] == '"' && str[len-1] == '"')
    this->output += str.substr(1, len-2);
  else
    this->output += str;
}

void Builder::append_module_output(Alignment alignment, std::string module_output, bool margin_left, bool margin_right)
{
  if (module_output.empty()) return;

  if (alignment != this->alignment) {
    this->alignment = alignment;
    switch (alignment) {
      case ALIGN_NONE: return;
      case ALIGN_LEFT: this->align_left(); break;
      case ALIGN_CENTER: this->align_center(); break;
      case ALIGN_RIGHT: this->align_right(); break;
    }
  }

  int margin;

  if (margin_left && (margin= this->get_opts()->module_margin_left) > 0)
    this->output += std::string(margin, ' ');

  this->append(module_output);

  if (margin_right && (margin = this->get_opts()->module_margin_right) > 0)
    this->output += std::string(margin, ' ');
}

void Builder::node(std::string str, bool add_space)
{
  std::string::size_type n, m;
  std::string s(str);

  // Parse raw tags
  while (true) {
    if (s.empty()) {
      break;

    } else if ((n = s.find("%{F-}")) == 0) {
      this->color_close(!this->lazy_closing);
      s.erase(0, 5);

    } else if ((n = s.find("%{F#")) == 0 && (m = s.find("}")) != std::string::npos) {
      if (m-n-4 == 2)
        this->color_alpha(s.substr(n+3, m-3));
      else
        this->color(s.substr(n+3, m-3));
      s.erase(n, m+1);

    } else if ((n = s.find("%{B-}")) == 0) {
      this->background_close(!this->lazy_closing);
      s.erase(0, 5);

    } else if ((n = s.find("%{B#")) == 0 && (m = s.find("}")) != std::string::npos) {
      this->background(s.substr(n+3, m-3));
      s.erase(n, m+1);

    } else if ((n = s.find("%{T-}")) == 0) {
      this->font_close(!this->lazy_closing);
      s.erase(0, 5);

    } else if ((n = s.find("%{T")) == 0 && (m = s.find("}")) != std::string::npos) {
      this->font(std::atoi(s.substr(n+3, m-3).c_str()));
      s.erase(n, m+1);

    } else if ((n = s.find("%{U-}")) == 0) {
      this->line_color_close(!this->lazy_closing);
      s.erase(0, 5);

    } else if ((n = s.find("%{U#")) == 0 && (m = s.find("}")) != std::string::npos) {
      this->line_color(s.substr(n+3, m-3));
      s.erase(n, m+1);

    } else if ((n = s.find("%{+u}")) == 0) {
      this->underline();
      s.erase(0, 5);

    } else if ((n = s.find("%{+o}")) == 0) {
      this->overline();
      s.erase(0, 5);

    } else if ((n = s.find("%{-u}")) == 0) {
      this->underline_close(true);
      s.erase(0, 5);

    } else if ((n = s.find("%{-o}")) == 0) {
      this->overline_close(true);
      s.erase(0, 5);

    } else if ((n = s.find("%{A}")) == 0) {
      this->cmd_close(true);
      s.erase(0, 4);

    } else if ((n = s.find("%{")) == 0 && (m = s.find("}")) != std::string::npos) {
      this->append(s.substr(n, m+1));
      s.erase(n, m+1);

    } else if ((n = s.find("%{")) > 0) {
      this->append(s.substr(0, n));
      s.erase(0, n);

    } else break;
  }

  if (!s.empty()) this->append(s);
  if (add_space) this->space();
}

void Builder::node(std::string str, int font_index, bool add_space)
{
  this->font(font_index);
    this->node(str, add_space);
  this->font_close();
}

void Builder::node(drawtypes::Bar *bar, float percentage, bool add_space)
{
  this->node(bar->get_output(math::cap<float>(percentage, 0, 100)), add_space);
}

void Builder::node(std::unique_ptr<drawtypes::Bar> &bar, float percentage, bool add_space) {
  this->node(bar.get(), percentage, add_space);
}

void Builder::node(drawtypes::Label *label, bool add_space)
{
  if (!*label) return;

  auto text = label->text;

  if (label->maxlen > 0 && text.length() > label->maxlen)
    text = text.substr(0, label->maxlen) + "...";

  if ((label->ol.empty() && this->o > 0) || (this->o > 0 && label->margin > 0))
    this->overline_close(true);
  if ((label->ul.empty() && this->u > 0) || (this->u > 0 && label->margin > 0))
    this->underline_close(true);

  if (label->margin > 0)
    this->space(label->margin);

  if (!label->ol.empty())
    this->overline(label->ol);
  if (!label->ul.empty())
    this->underline(label->ul);

  this->background(label->bg);
    this->color(label->fg);
      if (label->padding > 0)
        this->space(label->padding);
      this->node(text, label->font, add_space);
      if (label->padding > 0)
        this->space(label->padding);
    this->color_close(lazy_closing && label->margin > 0);
  this->background_close(lazy_closing && label->margin > 0);

  if (!label->ul.empty() || (label->margin > 0 && this->u > 0))
    this->underline_close(lazy_closing && label->margin > 0);
  if (!label->ol.empty() || (label->margin > 0 && this->o > 0))
    this->overline_close(lazy_closing && label->margin > 0);

  if (label->margin > 0)
    this->space(label->margin);
}

void Builder::node(std::unique_ptr<drawtypes::Label> &label, bool add_space) {
  this->node(label.get(), add_space);
}

void Builder::node(drawtypes::Icon *icon, bool add_space)
{
  this->node((drawtypes::Label*) icon, add_space);
}

void Builder::node(std::unique_ptr<drawtypes::Icon> &icon, bool add_space) {
  this->node(icon.get(), add_space);
}

void Builder::node(drawtypes::Ramp *ramp, float percentage, bool add_space) {
  if (*ramp) this->node(ramp->get_by_percentage(math::cap<float>(percentage, 0, 100)), add_space);
}

void Builder::node(std::unique_ptr<drawtypes::Ramp> &ramp, float percentage, bool add_space) {
  this->node(ramp.get(), percentage, add_space);
}

void Builder::node(drawtypes::Animation *animation, bool add_space) {
  if (*animation) this->node(animation->get(), add_space);
}

void Builder::node(std::unique_ptr<drawtypes::Animation> &animation, bool add_space) {
  this->node(animation.get(), add_space);
}

void Builder::offset(int pixels)
{
  if (pixels != 0)
    this->tag_open('O', std::to_string(pixels));
}

void Builder::space(int width)
{
  if (width == DEFAULT_SPACING) width = this->get_opts()->spacing;
  if (width <= 0) return;
  std::string str(width, ' ');
  this->append(str);
}

void Builder::remove_trailing_space(int width)
{
  if (width == DEFAULT_SPACING) width = this->get_opts()->spacing;
  if (width <= 0) return;
  std::string::size_type spacing = width;
  std::string str(spacing, ' ');
  if (this->output.length() >= spacing && this->output.substr(this->output.length()-spacing) == str)
    this->output = this->output.substr(0, this->output.length()-spacing);
}

// void Builder::invert() {
//   this->tag_open('R', "");
// }


// Fonts

void Builder::font(int index)
{
  if (index <= 0 && this->T > 0) this->font_close(true);
  if (index <= 0 || index == this->T_value) return;
  if (this->lazy_closing && this->T > 0) this->font_close(true);

  this->T++;
  this->T_value = index;
  this->tag_open('T', std::to_string(index));
}

void Builder::font_close(bool force)
{
  if ((!force && this->lazy_closing) || this->T <= 0) return;

  this->T--;
  this->T_value = 1;
  this->tag_close('T');
}


// Back- and foreground

void Builder::background(std::string color_)
{
  auto color(color_);

  if (color.length() == 2 || (color.find("#") == 0 && color.length() == 3)) {
    color = "#"+ color.substr(color.length()-2);
    auto bg = this->get_opts()->background;
    color += bg.substr(bg.length()-(bg.length() < 6 ? 3 : 6));
  } else if (color.length() >= 7 && color == "#"+ std::string(color.length()-1, color[1])) {
    color = color.substr(0, 4);
  }

  if (color.empty() && this->B > 0) this->background_close(true);
  if (color.empty() || color == this->B_value) return;
  if (this->lazy_closing && this->B > 0) this->background_close(true);

  this->B++;
  this->B_value = color;
  this->tag_open('B', color);
}

void Builder::background_close(bool force)
{
  if ((!force && this->lazy_closing) || this->B <= 0) return;

  this->B--;
  this->B_value = "";
  this->tag_close('B');
}

void Builder::color(std::string color_)
{
  auto color(color_);
  if (color.length() == 2 || (color.find("#") == 0 && color.length() == 3)) {
    color = "#"+ color.substr(color.length()-2);
    auto bg = this->get_opts()->foreground;
    color += bg.substr(bg.length()-(bg.length() < 6 ? 3 : 6));
  } else if (color.length() >= 7 && color == "#"+ std::string(color.length()-1, color[1])) {
    color = color.substr(0, 4);
  }

  if (color.empty() && this->F > 0) this->color_close(true);
  if (color.empty() || color == this->F_value) return;
  if (this->lazy_closing && this->F > 0) this->color_close(true);

  this->F++;
  this->F_value = color;
  this->tag_open('F', color);
}

void Builder::color_alpha(std::string alpha_)
{
  auto alpha(alpha_);
  std::string val = this->get_opts()->foreground;
  if (alpha.find("#") == std::string::npos ) {
    alpha = "#" + alpha;
  }

  if (alpha.size() == 4) {
    this->color(alpha);
    return;
  }

  if (val.size() < 6 && val.size() > 2) {
    val.append(val.substr(val.size() - 3));
  }

  this->color((alpha.substr(0, 3) + val.substr(val.size() - 6)).substr(0, 9));
}

void Builder::color_close(bool force)
{
  if ((!force && this->lazy_closing) || this->F <= 0) return;

  this->F--;
  this->F_value = "";
  this->tag_close('F');
}


// Under- and overline

void Builder::line_color(std::string color)
{
  if (color.empty() && this->U > 0) this->line_color_close(true);
  if (color.empty() || color == this->U_value) return;
  if (this->lazy_closing && this->U > 0) this->line_color_close(true);

  this->U++;
  this->U_value = color;
  this->tag_open('U', color);
}

void Builder::line_color_close(bool force)
{
  if ((!force && this->lazy_closing) || this->U <= 0) return;

  this->U--;
  this->U_value = "";
  this->tag_close('U');
}

void Builder::overline(std::string color)
{
  if (!color.empty()) this->line_color(color);
  if (this->o > 0) return;

  this->o++;
  this->append("%{+o}");
}

void Builder::overline_close(bool force)
{
  if ((!force && this->lazy_closing) || this->o <= 0) return;

  this->o--;
  this->append("%{-o}");
}

void Builder::underline(std::string color)
{
  if (!color.empty()) this->line_color(color);
  if (this->u > 0) return;

  this->u++;
  this->append("%{+u}");
}

void Builder::underline_close(bool force)
{
  if ((!force && this->lazy_closing) || this->u <= 0) return;

  this->u--;
  this->append("%{-u}");
}


// Command

void Builder::cmd(int button, std::string action, bool condition)
{
  if (!condition || action.empty()) return;

  boost::replace_all(action, ":", "\\:");
  boost::replace_all(action, "$", "\\$");
  boost::replace_all(action, "}", "\\}");
  boost::replace_all(action, "{", "\\{");
  boost::replace_all(action, "%", "\x0025");

  this->append("%{A"+ std::to_string(button) + ":"+ action +":}");
  this->A++;
}

void Builder::cmd_close(bool force)
{
  if (this->A > 0 || force) this->append("%{A}");
  if (this->A > 0) this->A--;
}
