# Abyss styles

`Application::initialise()` in `src/app/app.cc` concatenates these files in
the following order and applies the result once to the application:

1. `core.qss`: palette documentation, global typography and panel surfaces.
2. `chrome.qss`: menus, status bar and splitters.
3. `tabs.qss`: tab strips and panes.
4. `tables.qss`: tables, lists, headers and selection.
5. `controls.qss`: shared inputs and buttons.
6. `scrollbars.qss`: horizontal and vertical scrollbars.
7. `shell.qss`: output and shell-specific input/page overrides.
8. `login.qss`: login title and primary action.
9. `overlays.qss`: message boxes and tooltips.

Qt QSS has no native imports. Add new files to the explicit list in the loader;
later rules can override earlier rules of equal specificity. Keep shared rules
before view-specific overrides. Launch from the repository root, since paths
are relative to the working directory. Missing files are logged with `qWarning()`.
