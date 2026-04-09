# Clipboard Paste Feature

Paste text from the host clipboard into the emulated Oric keyboard.
Useful for entering BASIC programs without typing them character by character.

## Usage

### Keyboard shortcut

Press **Ctrl+V** while the emulator window has focus and the GUI menu is closed.
The clipboard contents are typed into the Oric one character at a time.

### GUI menu

1. Press **F1** to open the GUI overlay.
2. Click the **Paste text** button (under the "Input" section).
3. Press **F1** again to close the GUI and watch the text being typed.

### Cancelling a paste

Press any key during an active paste to cancel it immediately.

### Tips

- Copy a BASIC program (with line numbers) to your clipboard, then Ctrl+V.
  Each newline becomes a RETURN keypress, so multi-line programs work.
- The emulator must be at a prompt or input state that accepts keyboard input.
- Characters not present on the Oric keyboard are silently skipped.
- Text is pasted at roughly 10 characters per second (each character is held
  for 3 frames and followed by a 2-frame gap at the Oric's 50 Hz frame rate).

## Supported characters

| Category          | Characters                                        |
|-------------------|---------------------------------------------------|
| Uppercase letters | A–Z (produced without SHIFT on the Oric)          |
| Lowercase letters | a–z (produced with SHIFT on the Oric)             |
| Digits            | 0–9                                               |
| Symbols           | `` ! @ # $ % ^ & * ( ) - = [ ] \ ; ' , . / `` |
| Shifted symbols   | `` _ + { } | : " < > ? ``                       |
| Whitespace        | Space, newline (→ RETURN)                         |

Characters outside this set (e.g. tabs, Unicode) are silently skipped.
`\r\n` line endings are collapsed to a single RETURN.

## Implementation

### Architecture

The feature is entirely additive — no existing emulation code was modified.

| File | Role |
|------|------|
| `src/frontends/sdl/text_paster.hpp` | `TextPaster` class declaration |
| `src/frontends/sdl/text_paster.cpp` | Implementation: character lookup table and state machine |
| `src/frontends/sdl/frontend.cpp` | Ctrl+V handling and per-frame `update()` call |
| `src/frontends/sdl/frontend.hpp` | `TextPaster` member added to `Frontend` |
| `src/frontends/gui/gui.cpp` | "Paste text" button in the ImGui menu |
| `src/frontends/gui/gui.hpp` | `paste_requested` flag and `consume_paste_request()` |
| `src/frontends/sdl/CMakeLists.txt` | `text_paster.cpp` added to build |

### How it works

1. **Trigger**: Ctrl+V (or the GUI button) reads the clipboard with
   `SDL_GetClipboardText()` and passes the string to `TextPaster::start()`.

2. **Character mapping**: Each ASCII character is mapped to an Oric keyboard
   matrix position (`key_bits`, 0–63) and a `needs_shift` flag via a
   compile-time lookup table.  The mapping follows the US keyboard layout
   used by the Oric Atmos ROM.

   On the Oric, unshifted letter keys produce **uppercase** and SHIFT+letter
   produces **lowercase** — the reverse of modern convention.  The lookup
   table accounts for this.

3. **State machine**: A frame-driven state machine in `TextPaster::update()`
   simulates typing one character at a time:

   ```
   GAP (1 frame) → KEY_DOWN (2 frames) → KEY_UP → GAP → next character
   ```

   The very first character uses a longer initial gap (5 frames / 100 ms)
   to let the cursor settle, ensuring alignment with other emulators like
   Oricutron.

   Each transition calls `Machine::key_press(key_bits, down)` to set or
   clear bits in the keyboard matrix — the same function used by real
   keyboard input.  SHIFT is pressed/released alongside the character key
   when needed.

4. **Cancellation**: Any real keypress during an active paste calls
   `TextPaster::cancel()`, which releases any held keys and clears the queue.

### Design decisions

- **Keyboard matrix injection** was chosen over direct memory writes to the
  BASIC input buffer.  Matrix injection works with any software running on
  the Oric, not just BASIC, and avoids assumptions about ROM internals.

- **Frame-based timing** (not cycle-based) keeps the implementation simple.
  The Oric ROM's keyboard polling runs continuously and picks up matrix
  changes within a frame.

- **Ctrl+V** follows the existing emulator hotkey pattern (Ctrl+W for warp,
  Ctrl+R for NMI, Ctrl+B for break).  It is intercepted before reaching the
  Oric keyboard matrix.

## Revision history

- **v1.1** — Doubled paste speed (HOLD 3→2, GAP 2→1 frames, ~20 chars/sec).
  Added a longer initial gap (5 frames) before the first keystroke to fix a
  cosmetic one-character-left-shift that occurred vs Oricutron.
- **v1.0** — Initial implementation.
