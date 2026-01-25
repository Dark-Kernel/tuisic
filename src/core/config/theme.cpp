#include "theme.hpp"

// Static member definitions
std::map<std::string, ThemeColors> ThemeManager::themes;
ThemeColors ThemeManager::current_theme;
std::string ThemeManager::current_theme_name = "dracula";

void ThemeManager::initialize_themes() {
  // Dracula theme - Official Dracula color palette
  themes["dracula"] = {
    // Background colors - subtle and dark
    .background = Color::RGB(40, 42, 54),        // #282a36 (main background)
    .surface = Color::RGB(44, 47, 58),           // #2c2f3a (slightly lighter for surfaces)
    .overlay = Color::RGB(55, 58, 71),           // #373a47 (even lighter for overlays)
    
    // Text colors - proper contrast
    .text_primary = Color::RGB(248, 248, 242),   // #f8f8f2 (foreground)
    .text_secondary = Color::RGB(189, 147, 249), // #bd93f9 (purple)
    .text_muted = Color::RGB(105, 112, 134),     // #697086 (comment gray)
    
    // Accent colors - authentic dracula
    .accent_primary = Color::RGB(80, 250, 123),  // #50fa7b (green)
    .accent_secondary = Color::RGB(255, 121, 198), // #ff79c6 (pink)
    
    // Status colors
    .success = Color::RGB(80, 250, 123),         // #50fa7b (green)
    .warning = Color::RGB(241, 250, 140),        // #f1fa8c (yellow)
    .error = Color::RGB(255, 85, 85),            // #ff5555 (red)
    .info = Color::RGB(139, 233, 253),           // #8be9fd (cyan)
    
    // UI elements - subtle backgrounds
    .button_bg = Color::RGB(44, 47, 58),         // #2c2f3a
    .button_text = Color::RGB(248, 248, 242),    // #f8f8f2
    .button_hover_bg = Color::RGB(55, 58, 71),   // #373a47
    .button_hover_text = Color::RGB(248, 248, 242), // #f8f8f2
    
    // Menu colors
    .menu_bg = Color::RGB(40, 42, 54),           // #282a36 (same as background)
    .menu_text = Color::RGB(248, 248, 242),      // #f8f8f2
    .menu_selected_bg = Color::RGB(55, 58, 71),  // #373a47 (subtle selection)
    .menu_selected_text = Color::RGB(80, 250, 123), // #50fa7b (green accent)
    
    // Input colors
    .input_bg = Color::RGB(44, 47, 58),          // #2c2f3a
    .input_text = Color::RGB(248, 248, 242),     // #f8f8f2
    .input_focus_bg = Color::RGB(55, 58, 71),    // #373a47
    .input_focus_text = Color::RGB(139, 233, 253), // #8be9fd (cyan when focused)
    
    // Borders - very subtle
    .border = Color::RGB(55, 58, 71),            // #373a47
    .border_focus = Color::RGB(80, 250, 123),    // #50fa7b (green)
    
    // Visualizer - dracula spectrum
    .visualizer_low = Color::RGB(80, 250, 123),  // #50fa7b (green)
    .visualizer_mid = Color::RGB(241, 250, 140), // #f1fa8c (yellow)
    .visualizer_high = Color::RGB(255, 85, 85),  // #ff5555 (red)
  };

  // Gruvbox theme - Authentic Gruvbox Dark color palette
  themes["gruvbox"] = {
    // Background colors - warm and muted
    .background = Color::RGB(40, 40, 40),        // #282828 (bg0)
    .surface = Color::RGB(50, 48, 47),           // #32302f (bg1)
    .overlay = Color::RGB(60, 56, 54),           // #3c3836 (bg2)
    
    // Text colors - authentic gruvbox
    .text_primary = Color::RGB(235, 219, 178),   // #ebdbb2 (fg1)
    .text_secondary = Color::RGB(213, 196, 161), // #d5c4a1 (fg2)
    .text_muted = Color::RGB(146, 131, 116),     // #928374 (gray)
    
    // Accent colors - gruvbox aqua and orange
    .accent_primary = Color::RGB(142, 192, 124), // #8ec07c (aqua)
    .accent_secondary = Color::RGB(254, 128, 25), // #fe8019 (orange)
    
    // Status colors - gruvbox palette
    .success = Color::RGB(184, 187, 38),         // #b8bb26 (green)
    .warning = Color::RGB(215, 153, 33),         // #d79921 (yellow)
    .error = Color::RGB(204, 36, 29),            // #cc241d (red)
    .info = Color::RGB(142, 192, 124),           // #8ec07c (aqua)
    
    // UI elements - warm backgrounds
    .button_bg = Color::RGB(50, 48, 47),         // #32302f
    .button_text = Color::RGB(235, 219, 178),    // #ebdbb2
    .button_hover_bg = Color::RGB(60, 56, 54),   // #3c3836
    .button_hover_text = Color::RGB(235, 219, 178), // #ebdbb2
    
    // Menu colors
    .menu_bg = Color::RGB(40, 40, 40),           // #282828
    .menu_text = Color::RGB(235, 219, 178),      // #ebdbb2
    .menu_selected_bg = Color::RGB(60, 56, 54),  // #3c3836
    .menu_selected_text = Color::RGB(184, 187, 38), // #b8bb26 (green)
    
    // Input colors
    .input_bg = Color::RGB(50, 48, 47),          // #32302f
    .input_text = Color::RGB(235, 219, 178),     // #ebdbb2
    .input_focus_bg = Color::RGB(60, 56, 54),    // #3c3836
    .input_focus_text = Color::RGB(142, 192, 124), // #8ec07c (aqua)
    
    // Borders
    .border = Color::RGB(60, 56, 54),            // #3c3836
    .border_focus = Color::RGB(142, 192, 124),   // #8ec07c (aqua)
    
    // Visualizer - gruvbox spectrum
    .visualizer_low = Color::RGB(184, 187, 38),  // #b8bb26 (green)
    .visualizer_mid = Color::RGB(215, 153, 33),  // #d79921 (yellow)
    .visualizer_high = Color::RGB(204, 36, 29),  // #cc241d (red)
  };

  // Catppuccin Mocha - Authentic Catppuccin Mocha palette
  themes["catppuccin"] = {
    // Background colors - elegant pastels
    .background = Color::RGB(30, 30, 46),        // #1e1e2e (base)
    .surface = Color::RGB(49, 50, 68),           // #313244 (surface0)
    .overlay = Color::RGB(69, 71, 90),           // #45475a (surface1)
    
    // Text colors - catppuccin hierarchy  
    .text_primary = Color::RGB(205, 214, 244),   // #cdd6f4 (text)
    .text_secondary = Color::RGB(186, 194, 222), // #bac2de (subtext1)
    .text_muted = Color::RGB(166, 173, 200),     // #a6adc8 (subtext0)
    
    // Accent colors - catppuccin signature colors
    .accent_primary = Color::RGB(137, 180, 250), // #89b4fa (blue)
    .accent_secondary = Color::RGB(203, 166, 247), // #cba6f7 (mauve)
    
    // Status colors - catppuccin functional colors
    .success = Color::RGB(166, 227, 161),        // #a6e3a1 (green)
    .warning = Color::RGB(249, 226, 175),        // #f9e2af (yellow)
    .error = Color::RGB(243, 139, 168),          // #f38ba8 (red)
    .info = Color::RGB(116, 199, 236),           // #74c7ec (sky)
    
    // UI elements - subtle surfaces
    .button_bg = Color::RGB(49, 50, 68),         // #313244 (surface0)
    .button_text = Color::RGB(205, 214, 244),    // #cdd6f4 (text)
    .button_hover_bg = Color::RGB(69, 71, 90),   // #45475a (surface1)
    .button_hover_text = Color::RGB(205, 214, 244), // #cdd6f4 (text)
    
    // Menu colors
    .menu_bg = Color::RGB(30, 30, 46),           // #1e1e2e (base)
    .menu_text = Color::RGB(205, 214, 244),      // #cdd6f4 (text)
    .menu_selected_bg = Color::RGB(69, 71, 90),  // #45475a (surface1)
    .menu_selected_text = Color::RGB(137, 180, 250), // #89b4fa (blue)
    
    // Input colors
    .input_bg = Color::RGB(49, 50, 68),          // #313244 (surface0)
    .input_text = Color::RGB(205, 214, 244),     // #cdd6f4 (text)
    .input_focus_bg = Color::RGB(69, 71, 90),    // #45475a (surface1)
    .input_focus_text = Color::RGB(137, 180, 250), // #89b4fa (blue)
    
    // Borders - minimal and elegant
    .border = Color::RGB(69, 71, 90),            // #45475a (surface1)
    .border_focus = Color::RGB(137, 180, 250),   // #89b4fa (blue)
    
    // Visualizer - catppuccin spectrum
    .visualizer_low = Color::RGB(166, 227, 161), // #a6e3a1 (green)
    .visualizer_mid = Color::RGB(249, 226, 175), // #f9e2af (yellow)
    .visualizer_high = Color::RGB(243, 139, 168), // #f38ba8 (red)
  };

  // Tokyo Night theme - Authentic Tokyo Night colors
  themes["tokyo_night"] = {
    // Background colors - night-inspired
    .background = Color::RGB(26, 27, 38),        // #1a1b26 (bg)
    .surface = Color::RGB(36, 40, 59),           // #24283b (bg_highlight)
    .overlay = Color::RGB(52, 59, 88),           // #343e58 (terminal_black)
    
    // Text colors - tokyo night hierarchy
    .text_primary = Color::RGB(192, 202, 245),   // #c0caf5 (fg)
    .text_secondary = Color::RGB(169, 177, 214), // #a9b1d6 (fg_dark)
    .text_muted = Color::RGB(86, 95, 137),       // #565f89 (comment)
    
    // Accent colors - tokyo neon
    .accent_primary = Color::RGB(125, 207, 255), // #7dcfff (cyan)
    .accent_secondary = Color::RGB(187, 154, 247), // #bb9af7 (purple)
    
    // Status colors - tokyo functional
    .success = Color::RGB(158, 206, 106),        // #9ece6a (green)
    .warning = Color::RGB(224, 175, 104),        // #e0af68 (yellow)
    .error = Color::RGB(247, 118, 142),          // #f7768e (red)
    .info = Color::RGB(125, 207, 255),           // #7dcfff (cyan)
    
    // UI elements - tokyo surfaces
    .button_bg = Color::RGB(36, 40, 59),         // #24283b
    .button_text = Color::RGB(192, 202, 245),    // #c0caf5
    .button_hover_bg = Color::RGB(52, 59, 88),   // #343e58
    .button_hover_text = Color::RGB(192, 202, 245), // #c0caf5
    
    // Menu colors
    .menu_bg = Color::RGB(26, 27, 38),           // #1a1b26
    .menu_text = Color::RGB(192, 202, 245),      // #c0caf5
    .menu_selected_bg = Color::RGB(52, 59, 88),  // #343e58
    .menu_selected_text = Color::RGB(125, 207, 255), // #7dcfff (cyan)
    
    // Input colors
    .input_bg = Color::RGB(36, 40, 59),          // #24283b
    .input_text = Color::RGB(192, 202, 245),     // #c0caf5
    .input_focus_bg = Color::RGB(52, 59, 88),    // #343e58
    .input_focus_text = Color::RGB(125, 207, 255), // #7dcfff (cyan)
    
    // Borders
    .border = Color::RGB(52, 59, 88),            // #343e58
    .border_focus = Color::RGB(125, 207, 255),   // #7dcfff (cyan)
    
    // Visualizer - tokyo spectrum
    .visualizer_low = Color::RGB(158, 206, 106), // #9ece6a (green)
    .visualizer_mid = Color::RGB(224, 175, 104), // #e0af68 (yellow)
    .visualizer_high = Color::RGB(247, 118, 142), // #f7768e (red)
  };

  // Nord theme - Authentic Nord color palette  
  themes["nord"] = {
    // Background colors - arctic calm
    .background = Color::RGB(46, 52, 64),        // #2e3440 (nord0)
    .surface = Color::RGB(59, 66, 82),           // #3b4252 (nord1)
    .overlay = Color::RGB(67, 76, 94),           // #434c5e (nord2)
    
    // Text colors - nord snow storm
    .text_primary = Color::RGB(236, 239, 244),   // #eceff4 (nord4)
    .text_secondary = Color::RGB(229, 233, 240), // #e5e9f0 (nord5)
    .text_muted = Color::RGB(143, 188, 187),     // #8fbcbb (nord7)
    
    // Accent colors - nord aurora
    .accent_primary = Color::RGB(136, 192, 208), // #88c0d0 (nord8)
    .accent_secondary = Color::RGB(94, 129, 172), // #5e81ac (nord10)
    
    // Status colors - nord aurora
    .success = Color::RGB(163, 190, 140),        // #a3be8c (nord14)
    .warning = Color::RGB(235, 203, 139),        // #ebcb8b (nord13)
    .error = Color::RGB(191, 97, 106),           // #bf616a (nord11)
    .info = Color::RGB(136, 192, 208),           // #88c0d0 (nord8)
    
    // UI elements - nord polar night
    .button_bg = Color::RGB(59, 66, 82),         // #3b4252 (nord1)
    .button_text = Color::RGB(236, 239, 244),    // #eceff4 (nord4)
    .button_hover_bg = Color::RGB(67, 76, 94),   // #434c5e (nord2)
    .button_hover_text = Color::RGB(236, 239, 244), // #eceff4 (nord4)
    
    // Menu colors
    .menu_bg = Color::RGB(46, 52, 64),           // #2e3440 (nord0)
    .menu_text = Color::RGB(236, 239, 244),      // #eceff4 (nord4)
    .menu_selected_bg = Color::RGB(67, 76, 94),  // #434c5e (nord2)
    .menu_selected_text = Color::RGB(136, 192, 208), // #88c0d0 (nord8)
    
    // Input colors
    .input_bg = Color::RGB(59, 66, 82),          // #3b4252 (nord1)
    .input_text = Color::RGB(236, 239, 244),     // #eceff4 (nord4)
    .input_focus_bg = Color::RGB(67, 76, 94),    // #434c5e (nord2)
    .input_focus_text = Color::RGB(136, 192, 208), // #88c0d0 (nord8)
    
    // Borders
    .border = Color::RGB(67, 76, 94),            // #434c5e (nord2)
    .border_focus = Color::RGB(136, 192, 208),   // #88c0d0 (nord8)
    
    // Visualizer - nord aurora
    .visualizer_low = Color::RGB(163, 190, 140), // #a3be8c (nord14)
    .visualizer_mid = Color::RGB(235, 203, 139), // #ebcb8b (nord13)
    .visualizer_high = Color::RGB(191, 97, 106), // #bf616a (nord11)
  };

  // One Dark theme - Authentic Atom One Dark colors
  themes["one_dark"] = {
    // Background colors - one dark hierarchy
    .background = Color::RGB(40, 44, 52),        // #282c34 (mono-3)
    .surface = Color::RGB(33, 37, 43),           // #21252b (slightly darker)
    .overlay = Color::RGB(60, 64, 73),           // #3c4049 (lighter surface)
    
    // Text colors - one dark grays
    .text_primary = Color::RGB(171, 178, 191),   // #abb2bf (mono-1)
    .text_secondary = Color::RGB(152, 160, 173), // #98a0ad (mono-2)
    .text_muted = Color::RGB(92, 99, 112),       // #5c6370 (comment)
    
    // Accent colors - one dark signature
    .accent_primary = Color::RGB(97, 175, 239),  // #61afef (blue)
    .accent_secondary = Color::RGB(198, 120, 221), // #c678dd (purple)
    
    // Status colors - one dark semantic
    .success = Color::RGB(152, 195, 121),        // #98c379 (green)
    .warning = Color::RGB(229, 192, 123),        // #e5c07b (yellow)
    .error = Color::RGB(224, 108, 117),          // #e06c75 (red)
    .info = Color::RGB(86, 182, 194),            // #56b6c2 (cyan)
    
    // UI elements - one dark surfaces
    .button_bg = Color::RGB(60, 64, 73),         // #3c4049
    .button_text = Color::RGB(171, 178, 191),    // #abb2bf
    .button_hover_bg = Color::RGB(75, 81, 98),   // #4b5162
    .button_hover_text = Color::RGB(171, 178, 191), // #abb2bf
    
    // Menu colors
    .menu_bg = Color::RGB(40, 44, 52),           // #282c34
    .menu_text = Color::RGB(171, 178, 191),      // #abb2bf
    .menu_selected_bg = Color::RGB(60, 64, 73),  // #3c4049
    .menu_selected_text = Color::RGB(97, 175, 239), // #61afef (blue)
    
    // Input colors
    .input_bg = Color::RGB(60, 64, 73),          // #3c4049
    .input_text = Color::RGB(171, 178, 191),     // #abb2bf
    .input_focus_bg = Color::RGB(75, 81, 98),    // #4b5162
    .input_focus_text = Color::RGB(97, 175, 239), // #61afef (blue)
    
    // Borders
    .border = Color::RGB(60, 64, 73),            // #3c4049
    .border_focus = Color::RGB(97, 175, 239),    // #61afef (blue)
    
    // Visualizer - one dark spectrum
    .visualizer_low = Color::RGB(152, 195, 121), // #98c379 (green)
    .visualizer_mid = Color::RGB(229, 192, 123), // #e5c07b (yellow)
    .visualizer_high = Color::RGB(224, 108, 117), // #e06c75 (red)
  };
  
  // Set default theme to Dracula
  set_theme("dracula");
}

std::vector<std::string> ThemeManager::get_theme_names() {
  std::vector<std::string> names;
  for (const auto& [name, _] : themes) {
    names.push_back(name);
  }
  return names;
}

bool ThemeManager::set_theme(const std::string& theme_name) {
  auto it = themes.find(theme_name);
  if (it != themes.end()) {
    current_theme = it->second;
    current_theme_name = theme_name;
    return true;
  }
  return false;
}

const ThemeColors& ThemeManager::get_current_theme() {
  return current_theme;
}

const std::string& ThemeManager::get_current_theme_name() {
  return current_theme_name;
}

Color ThemeManager::get_visualizer_color(float intensity) {
  if (intensity < 0.33f) {
    return visualizer_low();
  } else if (intensity < 0.66f) {
    return visualizer_mid();
  } else {
    return visualizer_high();
  }
}