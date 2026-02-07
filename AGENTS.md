Paths in this file that start with a '/' are relative to the project root ($REPOBASE).

## Project Structure

This project contains several git submodules. From all existing (so called) "aicxx" submodules,
`transform` is using `cwm4`, `cwds`, `utils`, `enchantum`, `threadsafe`, `math` and `cairowindow`.

A lot of these submodules use the other submodules; for example every submodule uses cwds
and utils (those even use eachother).

### Overview of subdirectories and submodules

- `/cairowindow`: the git submodule that provides drawing capabilities (graphics).
- `/math`: the git submodule under test by this project (transform).
- `/src`: the test code that comprises this project.
  Their requirement is to compile without errors and run without asserting.
  The test code that is being most actively worked on right now is `bounding_box_test.cxx`.

### Remaining subdirectories that are not of interest to AI Agents

- `/cmake`: Contains instructions for gitache on how to download, configure and install libcwd (and any other github repository that might be required by the project).
- `/cwm4`: Contains build system support.
- `/cwds`: This is a git submodule containing debugging support code for C++ projects in general, on top of libcwd.
- `/utils`: This is a stable git submodule containing various C++ utilities that might be used by the project and other git submodules.
- `/enchantum`: Support for printing enums (debug output)
- `/threadsafe`: This submodules is only here because some of the tests in `/src` use them.
