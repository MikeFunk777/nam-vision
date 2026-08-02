# Releasing NAM Division

GitHub releases are created from semantic-version tags by
`.github/workflows/release-native.yml`. The workflow builds the Windows and
macOS installers first and creates an unpublished draft only after both builds
succeed. Publishing the draft remains a manual step.

## Prepare a release

1. Start from a clean `main` branch and run the **Build Native** workflow.
2. Update `PLUG_VERSION_HEX` and `PLUG_VERSION_STR` in
   `NeuralAmpModeler/config.h`. For version `1.2.3`, use hexadecimal
   `0x00010203`.
3. Regenerate the platform metadata:

   ```bash
   cd NeuralAmpModeler/scripts
   python3 update_version-mac.py
   python3 update_version-ios.py
   ```

4. Update `NeuralAmpModeler/installer/changelog.txt`, the installer license,
   and third-party notices when necessary.
5. Commit and push the release preparation to `main`.

## Signing

The automated release currently produces unsigned installers. Before a public
production release, configure Developer ID signing and notarization for macOS
and Authenticode signing for Windows. Do not place certificates or passwords in
the repository; store them as GitHub Actions secrets.

The macOS distribution script accepts `CODESIGN=1` after the Developer ID
certificates and notarization credentials have been installed in the runner.
Windows signing must be enabled in `makedist-win.bat` after a signing
certificate is configured.

## Create the draft

Create the tag only after the release commit is on `main`:

```bash
git switch main
git pull --ff-only origin main
git tag -a v1.0.0 -m "NAM Division v1.0.0"
git push origin v1.0.0
```

The tag must exactly match `PLUG_VERSION_STR` with a leading `v`, or the
workflow will stop before building.

After **Release Native** succeeds, open GitHub **Releases**, inspect both
assets, edit the generated notes, and publish the draft. Expected user-facing
assets are a macOS `.dmg` and a Windows `.zip` containing the installer.

## If a build fails

Fix the problem on `main` and create a new release commit. Before a release is
published, a failed tag may be deleted and recreated deliberately. Never move
a tag after its release has been published; use a new patch version instead.
