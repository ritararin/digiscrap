#pragma once
// Keyboard shortcuts added by the interaction layer (CP3, Option 4) and the
// on-screen help line that advertises them. Header-only and free of GLFW: the
// key numbers are the GLFW_KEY_* values the app already receives as plain ints.
#include <string>

namespace interaction {

enum class Action { None, Duplicate, Undo, Redo, ToggleSnap, ToggleHelp, LayerUp, LayerDown };

namespace key {
constexpr int Comma = 44, Period = 46, LeftBracket = 91, RightBracket = 93;
constexpr int C = 67, G = 71, H = 72, Y = 89, Z = 90;
}  // namespace key

// Which action (if any) a day-page key triggers.
inline Action actionForKey(int k) {
    switch (k) {
        case key::C:                          return Action::Duplicate;
        case key::Z:                          return Action::Undo;
        case key::Y:                          return Action::Redo;
        case key::G:                          return Action::ToggleSnap;
        case key::H:                          return Action::ToggleHelp;
        case key::Period: case key::RightBracket: return Action::LayerUp;
        case key::Comma:  case key::LeftBracket:  return Action::LayerDown;
        default:                              return Action::None;
    }
}

// Text for the status ribbon. Only uses characters the 3x5 bitmap font can draw
// (letters, digits, space , . - ! ? ' : /), and fits the 682 px ribbon at scale 2.
inline std::string shortcutLine(bool snappingOn) {
    return std::string("C COPY   Z UNDO   Y REDO   G SNAP ") + (snappingOn ? "ON" : "OFF")
         + "   , . MOVE LAYER   H MOUSE HELP";
}

}  // namespace interaction
