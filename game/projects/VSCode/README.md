# VS Code workspace

Open `main.code-workspace` to work from the repository root. Follow the [build instructions](../../../BUILDING.md#3-visual-studio-code) to configure CMake Tools with source directory `${workspaceFolder}/game` and build directory `${workspaceFolder}/build/vscode`.

Build, run and debug the `untitled-ragebait-game` target through CMake Tools. The existing `.vscode/tasks.json` and `.vscode/launch.json` at the repository root invoke the legacy Makefile; they are not part of the current CMake workflow.
