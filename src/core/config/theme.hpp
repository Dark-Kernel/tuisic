#pragma once

#include <ftxui/screen/color.hpp>
#include <map>
#include <string>
#include <vector>

using namespace ftxui;

// Theme color palette structure
struct ThemeColors {
  // Background colors
  Color background;
  Color surface;
  Color overlay;
  
  // Text colors
  Color text_primary;
  Color text_secondary;
  Color text_muted;
  
  // Accent colors
  Color accent_primary;
  Color accent_secondary;
  
  // Status colors
  Color success;
  Color warning;
  Color error;
  Color info;
  
  // UI element colors
  Color button_bg;
  Color button_text;
  Color button_hover_bg;
  Color button_hover_text;
  
  // Menu colors
  Color menu_bg;
  Color menu_text;
  Color menu_selected_bg;
  Color menu_selected_text;
  
  // Input colors
  Color input_bg;
  Color input_text;
  Color input_focus_bg;
  Color input_focus_text;
  
  // Border colors
  Color border;
  Color border_focus;
  
  // Special colors
  Color visualizer_low;   // For low frequency bars
  Color visualizer_mid;   // For mid frequency bars
  Color visualizer_high;  // For high frequency bars
};

// Theme manager class
class ThemeManager {
private:
  static std::map<std::string, ThemeColors> themes;
  static ThemeColors current_theme;
  static std::string current_theme_name;
  
public:
  // Initialize all built-in themes
  static void initialize_themes();
  
  // Get available theme names
  static std::vector<std::string> get_theme_names();
  
  // Set current theme
  static bool set_theme(const std::string& theme_name);
  
  // Get current theme
  static const ThemeColors& get_current_theme();
  static const std::string& get_current_theme_name();
  
  // Color getter methods for UI components
  static Color background() { return current_theme.background; }
  static Color surface() { return current_theme.surface; }
  static Color overlay() { return current_theme.overlay; }
  
  static Color text_primary() { return current_theme.text_primary; }
  static Color text_secondary() { return current_theme.text_secondary; }
  static Color text_muted() { return current_theme.text_muted; }
  
  static Color accent_primary() { return current_theme.accent_primary; }
  static Color accent_secondary() { return current_theme.accent_secondary; }
  
  static Color success() { return current_theme.success; }
  static Color warning() { return current_theme.warning; }
  static Color error() { return current_theme.error; }
  static Color info() { return current_theme.info; }
  
  static Color button_bg() { return current_theme.button_bg; }
  static Color button_text() { return current_theme.button_text; }
  static Color button_hover_bg() { return current_theme.button_hover_bg; }
  static Color button_hover_text() { return current_theme.button_hover_text; }
  
  static Color menu_bg() { return current_theme.menu_bg; }
  static Color menu_text() { return current_theme.menu_text; }
  static Color menu_selected_bg() { return current_theme.menu_selected_bg; }
  static Color menu_selected_text() { return current_theme.menu_selected_text; }
  
  static Color input_bg() { return current_theme.input_bg; }
  static Color input_text() { return current_theme.input_text; }
  static Color input_focus_bg() { return current_theme.input_focus_bg; }
  static Color input_focus_text() { return current_theme.input_focus_text; }
  
  static Color border() { return current_theme.border; }
  static Color border_focus() { return current_theme.border_focus; }
  
  static Color visualizer_low() { return current_theme.visualizer_low; }
  static Color visualizer_mid() { return current_theme.visualizer_mid; }
  static Color visualizer_high() { return current_theme.visualizer_high; }
  
  // Helper method to get visualizer color based on intensity
  static Color get_visualizer_color(float intensity);
};