// =========================================================================
//   Clipboard paste support for the Auric emulator.
//   Feeds host clipboard text into the Oric keyboard matrix character by
//   character, simulating physical key presses.
// =========================================================================

#include "text_paster.hpp"
#include "machine.hpp"

#include <array>

// ---------------------------------------------------------------------------
// ASCII → Oric key_bits lookup table.
//
// Derived from the scancode_map[] in frontend.cpp which defines the 8×8
// keyboard matrix.  key_bits = (row << 3) | column.
//
// On the Oric, unshifted letter keys produce UPPERCASE.
// SHIFT + letter produces lowercase (opposite of modern convention).
// ---------------------------------------------------------------------------

namespace
{

struct KeyEntry {
    uint8_t key_bits;
    bool    needs_shift;
};

// Sentinel value for "no mapping".
constexpr uint8_t NONE = 0xFF;

// Unshifted letter key_bits (produce uppercase on the Oric).
constexpr uint8_t KEY_A = 53, KEY_B = 18, KEY_C = 23, KEY_D = 15;
constexpr uint8_t KEY_E = 51, KEY_F = 11, KEY_G = 50, KEY_H = 49;
constexpr uint8_t KEY_I = 41, KEY_J =  8, KEY_K = 24, KEY_L = 57;
constexpr uint8_t KEY_M = 16, KEY_N =  1, KEY_O = 42, KEY_P = 43;
constexpr uint8_t KEY_Q = 14, KEY_R = 10, KEY_S = 54, KEY_T =  9;
constexpr uint8_t KEY_U = 40, KEY_V =  3, KEY_W = 55, KEY_X =  6;
constexpr uint8_t KEY_Y = 48, KEY_Z = 21;

constexpr uint8_t KEY_0 = 58, KEY_1 =  5, KEY_2 = 22, KEY_3 =  7;
constexpr uint8_t KEY_4 = 19, KEY_5 =  2, KEY_6 = 17, KEY_7 =  0;
constexpr uint8_t KEY_8 = 56, KEY_9 = 25;

constexpr uint8_t KEY_SPACE     = 32;
constexpr uint8_t KEY_COMMA     = 33;
constexpr uint8_t KEY_PERIOD    = 34;
constexpr uint8_t KEY_SEMICOLON = 26;
constexpr uint8_t KEY_MINUS     = 27;
constexpr uint8_t KEY_SLASH     = 59;
constexpr uint8_t KEY_EQUALS    = 63;
constexpr uint8_t KEY_LBRACKET  = 47;
constexpr uint8_t KEY_RBRACKET  = 46;
constexpr uint8_t KEY_BACKSLASH = 30;
constexpr uint8_t KEY_APOSTR    = 31;
constexpr uint8_t KEY_RETURN    = 61;
constexpr uint8_t KEY_BACKSPACE = 45;

// Build a lookup table indexed by ASCII code (0-127).
// Each entry is { key_bits, needs_shift }.  NONE means unmapped.
constexpr auto build_ascii_table()
{
    std::array<KeyEntry, 128> tbl{};
    for (auto& e : tbl) { e = {NONE, false}; }

    // --- Uppercase letters (unshifted on the Oric) -----------------------
    tbl['A'] = {KEY_A, false};  tbl['B'] = {KEY_B, false};
    tbl['C'] = {KEY_C, false};  tbl['D'] = {KEY_D, false};
    tbl['E'] = {KEY_E, false};  tbl['F'] = {KEY_F, false};
    tbl['G'] = {KEY_G, false};  tbl['H'] = {KEY_H, false};
    tbl['I'] = {KEY_I, false};  tbl['J'] = {KEY_J, false};
    tbl['K'] = {KEY_K, false};  tbl['L'] = {KEY_L, false};
    tbl['M'] = {KEY_M, false};  tbl['N'] = {KEY_N, false};
    tbl['O'] = {KEY_O, false};  tbl['P'] = {KEY_P, false};
    tbl['Q'] = {KEY_Q, false};  tbl['R'] = {KEY_R, false};
    tbl['S'] = {KEY_S, false};  tbl['T'] = {KEY_T, false};
    tbl['U'] = {KEY_U, false};  tbl['V'] = {KEY_V, false};
    tbl['W'] = {KEY_W, false};  tbl['X'] = {KEY_X, false};
    tbl['Y'] = {KEY_Y, false};  tbl['Z'] = {KEY_Z, false};

    // --- Lowercase letters (SHIFT + key on the Oric) ---------------------
    tbl['a'] = {KEY_A, true};   tbl['b'] = {KEY_B, true};
    tbl['c'] = {KEY_C, true};   tbl['d'] = {KEY_D, true};
    tbl['e'] = {KEY_E, true};   tbl['f'] = {KEY_F, true};
    tbl['g'] = {KEY_G, true};   tbl['h'] = {KEY_H, true};
    tbl['i'] = {KEY_I, true};   tbl['j'] = {KEY_J, true};
    tbl['k'] = {KEY_K, true};   tbl['l'] = {KEY_L, true};
    tbl['m'] = {KEY_M, true};   tbl['n'] = {KEY_N, true};
    tbl['o'] = {KEY_O, true};   tbl['p'] = {KEY_P, true};
    tbl['q'] = {KEY_Q, true};   tbl['r'] = {KEY_R, true};
    tbl['s'] = {KEY_S, true};   tbl['t'] = {KEY_T, true};
    tbl['u'] = {KEY_U, true};   tbl['v'] = {KEY_V, true};
    tbl['w'] = {KEY_W, true};   tbl['x'] = {KEY_X, true};
    tbl['y'] = {KEY_Y, true};   tbl['z'] = {KEY_Z, true};

    // --- Digits (unshifted) ----------------------------------------------
    tbl['0'] = {KEY_0, false};  tbl['1'] = {KEY_1, false};
    tbl['2'] = {KEY_2, false};  tbl['3'] = {KEY_3, false};
    tbl['4'] = {KEY_4, false};  tbl['5'] = {KEY_5, false};
    tbl['6'] = {KEY_6, false};  tbl['7'] = {KEY_7, false};
    tbl['8'] = {KEY_8, false};  tbl['9'] = {KEY_9, false};

    // --- Symbols available unshifted -------------------------------------
    tbl[' ']  = {KEY_SPACE,     false};
    tbl[',']  = {KEY_COMMA,     false};
    tbl['.']  = {KEY_PERIOD,    false};
    tbl[';']  = {KEY_SEMICOLON, false};
    tbl['-']  = {KEY_MINUS,     false};
    tbl['/']  = {KEY_SLASH,     false};
    tbl['=']  = {KEY_EQUALS,    false};
    tbl['[']  = {KEY_LBRACKET,  false};
    tbl[']']  = {KEY_RBRACKET,  false};
    tbl['\\'] = {KEY_BACKSLASH, false};
    tbl['\''] = {KEY_APOSTR,    false};

    // --- Shifted symbols (US keyboard layout, matching Oric Atmos ROM) ----
    tbl['!']  = {KEY_1, true};          // SHIFT + 1
    tbl['@']  = {KEY_2, true};          // SHIFT + 2
    tbl['#']  = {KEY_3, true};          // SHIFT + 3
    tbl['$']  = {KEY_4, true};          // SHIFT + 4
    tbl['%']  = {KEY_5, true};          // SHIFT + 5
    tbl['^']  = {KEY_6, true};          // SHIFT + 6
    tbl['&']  = {KEY_7, true};          // SHIFT + 7
    tbl['*']  = {KEY_8, true};          // SHIFT + 8
    tbl['(']  = {KEY_9, true};          // SHIFT + 9
    tbl[')']  = {KEY_0, true};          // SHIFT + 0
    tbl[':']  = {KEY_SEMICOLON, true};  // SHIFT + ;
    tbl['"']  = {KEY_APOSTR, true};     // SHIFT + '
    tbl['+']  = {KEY_EQUALS, true};     // SHIFT + =
    tbl['<']  = {KEY_COMMA, true};      // SHIFT + ,
    tbl['>']  = {KEY_PERIOD, true};     // SHIFT + .
    tbl['?']  = {KEY_SLASH, true};      // SHIFT + /
    tbl['_']  = {KEY_MINUS, true};      // SHIFT + -
    tbl['{']  = {KEY_LBRACKET, true};   // SHIFT + [
    tbl['}']  = {KEY_RBRACKET, true};   // SHIFT + ]
    tbl['|']  = {KEY_BACKSLASH, true};  // SHIFT + backslash

    // --- Control characters ----------------------------------------------
    tbl['\n'] = {KEY_RETURN, false};
    tbl['\r'] = {KEY_RETURN, false};

    return tbl;
}

constexpr auto ascii_table = build_ascii_table();

} // anonymous namespace


// ---------------------------------------------------------------------------
// TextPaster implementation
// ---------------------------------------------------------------------------

std::optional<TextPaster::KeyAction> TextPaster::char_to_key(char c)
{
    auto uc = static_cast<unsigned char>(c);
    if (uc >= ascii_table.size()) {
        return std::nullopt;
    }
    const auto& entry = ascii_table[uc];
    if (entry.key_bits == NONE) {
        return std::nullopt;
    }
    return KeyAction{entry.key_bits, entry.needs_shift};
}


void TextPaster::start(const std::string& text)
{
    // Clear any previous paste.
    queue = {};
    state = State::IDLE;

    for (char c : text) {
        // Collapse \r\n into a single RETURN.
        if (c == '\r') {
            continue;
        }
        auto action = char_to_key(c);
        if (action.has_value()) {
            queue.push(action.value());
        }
        // Unmapped characters are silently skipped.
    }

    if (!queue.empty()) {
        state = State::GAP;
        frame_counter = 0;
        first_char = true;
    }
}


void TextPaster::update(Machine& machine)
{
    if (state == State::IDLE) {
        return;
    }

    ++frame_counter;

    switch (state) {
        case State::GAP: {
            int gap = first_char ? INITIAL_GAP_FRAMES : GAP_FRAMES;
            if (frame_counter >= gap) {
                first_char = false;
                if (queue.empty()) {
                    state = State::IDLE;
                    return;
                }
                auto action = queue.front();
                queue.pop();
                current_key   = action.key_bits;
                current_shift = action.needs_shift;

                // Press the key (and SHIFT if needed).
                if (current_shift) {
                    machine.key_press(SHIFT_KEY_BITS, true);
                }
                machine.key_press(current_key, true);

                state = State::KEY_DOWN;
                frame_counter = 0;
            }
            break;
        }

        case State::KEY_DOWN:
            if (frame_counter >= HOLD_FRAMES) {
                // Release the key (and SHIFT).
                machine.key_press(current_key, false);
                if (current_shift) {
                    machine.key_press(SHIFT_KEY_BITS, false);
                }

                state = State::KEY_UP;
                frame_counter = 0;
            }
            break;

        case State::KEY_UP:
            // Transition straight into GAP (the release frame counts as gap).
            state = State::GAP;
            frame_counter = 0;
            break;

        case State::IDLE:
            break;
    }
}


void TextPaster::cancel(Machine& machine)
{
    if (state == State::KEY_DOWN) {
        machine.key_press(current_key, false);
        if (current_shift) {
            machine.key_press(SHIFT_KEY_BITS, false);
        }
    }
    queue = {};
    state = State::IDLE;
    frame_counter = 0;
}
