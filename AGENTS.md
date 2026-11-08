# Project Coding Rules

- Use Simplified Chinese when communicating with the user.
- Keep modules simple. Extract a module only when the functionality is reusable or has a clear hardware/domain boundary.
- Put reusable C modules in matching `.c` and `.h` files.
- Add each public project module header to `main/headfile.h`; application entry files include `headfile.h`.
- Keep one-off test flow and orchestration in the relevant `*_main.c` file.
- Add concise comments for public functions, hardware assumptions, and non-obvious logic. Do not narrate obvious assignments.
- Do not manually edit generated output or third-party code under `build/` or `managed_components/`.
- Do not manually edit `.clangd`, `sdkconfig.old`, or `.vscode/settings.json`.
- Prefer `sdkconfig.defaults` and ESP-IDF configuration tools over hand-editing generated `sdkconfig` values.
