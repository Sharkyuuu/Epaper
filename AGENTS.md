# Project Coding Rules

- Use Simplified Chinese when communicating with the user.
- Keep modules simple. Extract a module only when the functionality is reusable or has a clear hardware/domain boundary.
- Put reusable C modules in matching `.c` and `.h` files.
- Add each public project module header to `main/headfile.h`; application entry files include `headfile.h`.
- Keep one-off test flow and orchestration in the relevant `*_main.c` file.
- Document every public function in its `.h` file with the project's separator style. Include Chinese `@brief`, every `@param`, `@return`, and `Sample usage`. Use `@param void` and `@return void` when applicable.
- Add concise comments for hardware assumptions and non-obvious internal logic. Do not duplicate public API documentation in the `.c` file or narrate obvious assignments.
- Do not manually edit generated output or third-party code under `build/` or `managed_components/`.
- Do not manually edit `.clangd`, `sdkconfig.old`, or `.vscode/settings.json`.
- Prefer `sdkconfig.defaults` and ESP-IDF configuration tools over hand-editing generated `sdkconfig` values.
