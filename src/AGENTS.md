## src test-program conventions

These files under `/src` are small interactive test programs. They must compile without errors and run without asserting.

### Plot handles vs geometry types

- Use `cairowindow::cs::*` types for pure geometry/math computations (rectangles, points, sizes, etc.).
- Use `cairowindow::plot::*` types only as plotted-object handles whose purpose is to keep the drawn object alive (their lifetime matters).

### Draggables and redrawing

- Use `Window::register_draggable(..., restriction)` only to *restrict* the new dragged position (e.g. constrain to a line/circle/rectangle).
- Do not use the `restriction` callback for side effects like updating/redrawing other plot objects.
- For objects that depend on draggable state, redraw them in a loop:
  - Optionally bracket drawing with `window.set_send_expose_events(false)` / `window.set_send_expose_events(true)` to avoid flicker.
  - Then block in `window.handle_input_events()` and repeat when a redraw is needed.
  - See `cairowindow/tests/quadratic_bezier.cxx` for the intended pattern.

### Comments

- Prefer more comments over fewer in these test programs, especially around event-loop/redraw structure.
