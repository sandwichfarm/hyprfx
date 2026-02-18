# Release Procedure

## Versioning

hyprfx follows [semver](https://semver.org/). Bump the version in `CMakeLists.txt` before tagging.

- **patch** (0.1.1): bug fixes, shader tweaks
- **minor** (0.2.0): new effects, config additions
- **major** (1.0.0): breaking config changes, API overhaul

## Steps

1. **Update version** in `CMakeLists.txt`:
   ```cmake
   project(hyprfx VERSION 0.2.0)
   ```

2. **Verify the build** compiles and all effects work:
   ```sh
   cmake -B build && cmake --build build
   ```

3. **Commit the version bump**:
   ```sh
   git add CMakeLists.txt
   git commit -m "release: v0.2.0"
   ```

4. **Tag and push**:
   ```sh
   git tag v0.2.0
   git push origin main --tags
   ```

5. The `release.yml` workflow will automatically:
   - Build `libhyprfx.so` in an Arch container
   - Create a GitHub Release with auto-generated release notes
   - Attach the built `.so` as a release asset

6. **Edit the release** on GitHub if you want to add extra notes beyond the auto-generated changelog.

## Hyprland Compatibility

Each release is built against the Hyprland version available in the Arch `[extra]` repo at tag time. If Hyprland makes breaking API changes, a new hyprfx release is needed.

Users installing via `hyprpm` build from source against their local Hyprland headers, so binary compatibility is less of a concern for that install path.
