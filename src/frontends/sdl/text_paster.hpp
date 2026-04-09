// =========================================================================
//   Clipboard paste support for the Auric emulator.
//   Feeds host clipboard text into the Oric keyboard matrix character by
//   character, simulating physical key presses.
// =========================================================================

#ifndef FRONTENDS_SDL_TEXT_PASTER_H
#define FRONTENDS_SDL_TEXT_PASTER_H

#include <cstdint>
#include <optional>
#include <queue>
#include <string>

class Machine;

class TextPaster
{
public:
    /// Begin pasting the given text into the emulated keyboard.
    void start(const std::string& text);

    /// Advance the paste state machine by one frame.  Call once per
    /// handle_frame() iteration.
    void update(Machine& machine);

    /// True while a paste operation is in progress.
    bool is_active() const { return state != State::IDLE; }

    /// Abort any paste in progress and release held keys.
    void cancel(Machine& machine);

private:
    // Oric LSHIFT position in the keyboard matrix.
    static constexpr uint8_t SHIFT_KEY_BITS = 36;

    // Number of frames to hold a key down / pause between keys.
    static constexpr int HOLD_FRAMES        = 3;   // ~60 ms at 50 Hz
    static constexpr int GAP_FRAMES         = 2;   // ~40 ms at 50 Hz
    static constexpr int INITIAL_GAP_FRAMES = 5;   // ~100 ms — let cursor settle

    struct KeyAction {
        uint8_t key_bits;
        bool    needs_shift;
    };

    enum class State { IDLE, KEY_DOWN, KEY_UP, GAP, SHIFT_SETTLE };

    std::queue<KeyAction> queue;
    State   state         = State::IDLE;
    int     frame_counter = 0;
    bool    first_char    = false;
    uint8_t current_key   = 0;
    bool    current_shift = false;

    /// Map an ASCII character to its Oric keyboard matrix position.
    /// Returns std::nullopt for characters that have no Oric key equivalent.
    static std::optional<KeyAction> char_to_key(char c);
};

#endif // FRONTENDS_SDL_TEXT_PASTER_H
