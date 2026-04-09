# Runtime Inspector Fab Submission Checklist

This file is a Fab submission checklist, not the RuntimeInspector agent development authority.

For implementation rules and development workflow authority, use `Docs/AGENT_DEVELOPMENT.md`.

Use this checklist when preparing the UE 5.5 Fab submission package.

## Package

- [x] `RuntimeInspector.uplugin` has release metadata for UE 5.5.
- [x] `README.md` explains installation, activation, scope, and support.
- [x] `Config/FilterPlugin.ini` is present for release packaging.
- [x] `Scripts/PackageFabRelease.ps1` builds and stages a clean release package.
- [x] Clean package output is generated under `Saved/FabRelease/Package/RuntimeInspector_UE55/RuntimeInspector`.
- [x] Final package excludes `.git`, `Docs`, `Saved`, internal logs, and other repo-only residue.
- [x] Final package retains the precompiled `Binaries` required for direct install into a blank UE 5.5 blueprint project.
- [x] Final package retains the validated `Intermediate` precompile data emitted by `BuildPlugin -NoHostPlatform`.

## Listing Copy

- [x] Title, short description, long description, and feature bullets are prepared in `FAB_LISTING.md`.
- [x] Support and documentation links are defined in `RuntimeInspector.uplugin`.
- [ ] Replace `MarketplaceURL` in `RuntimeInspector.uplugin` after Fab listing is created.

## Media

- [x] Capture cover image.
- [x] Capture 3-6 product screenshots based on `FAB_LISTING.md`.
- [ ] Record a short demo video or GIF.
- [ ] Confirm screenshots match the actual shipped UE 5.5 feature set.
- [x] Media staging folder and expected filenames are prepared under `FabMedia/`.
- [x] Automated capture path is available at `Scripts\CaptureFabMedia.cmd` and writes deterministic shots to `Saved\RuntimeInspector\FabMediaCapture\`.

## Validation

- [x] UE 5.5 `BuildPlugin` validation passes through the release script.
- [x] Install the packaged plugin into a clean blank UE 5.5 project and confirm it loads.
  - Use `Scripts\ValidateFabBlankProject.cmd` for the automated load check.
- [x] Use the main `PluginMaker` project, not the blank validation host, for loopback packaged-runtime self-tests and bridge-driven workflow acceptance.
  - Use `Scripts\BuildPackagedRuntimeValidation.cmd`, `Scripts\RunPackagedRuntimeValidation.cmd`, and `Scripts\StopPackagedRuntimeValidation.cmd`.
- [ ] Re-run a minimal smoke path in the packaged install:
  - Open Runtime Inspector.
  - Inspect an actor.
  - Open Changes, Settings, and Tools pages.
  - Use `Scripts\ValidateFabBlankProject.cmd -KeepValidationProject` and `Scripts\OpenFabValidationProject.cmd` to preserve and open the generated host project for manual smoke only.
- [x] Capture Fab screenshots from the main `PluginMaker` editor state, not the blank validation host.
  - Use `Scripts\OpenFabScreenshotState.cmd` to open the clean screenshot presentation state.
  - Use `Scripts\CaptureFabMedia.cmd` to generate the deterministic screenshot staging output.

## Optional Cleanup Before Submission

- [ ] Normalize source file encodings to remove `warning C4828` noise.
- [ ] Replace any remaining temporary support/documentation URLs if needed.
