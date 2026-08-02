# Installer Identity

The checked-in installers use the NAM Division product metadata. Review the
identity, `license.rtf`, and third-party notices before every public release.

The Windows distribution script calls `scripts/update_installer-win.py`, which accepts these environment variable overrides:

- `INSTALLER_DISPLAY_NAME`
- `INSTALLER_APP_CONTACT`
- `INSTALLER_APP_COPYRIGHT`
- `INSTALLER_APP_PUBLISHER`
- `INSTALLER_APP_PUBLISHER_URL`
- `INSTALLER_APP_SUPPORT_URL`
- `INSTALLER_OUTPUT_BASE_FILENAME`
- `INSTALLER_WELCOME_LABEL`
- `INSTALLER_SETUP_WINDOW_TITLE`
- `INSTALLER_SCRIPT_NAME`

The macOS installer package identifiers default to `com.pedaldivision.*`. Set `INSTALLER_PKG_ID_PREFIX` to use another reverse-DNS prefix, for example `com.example.myproduct`.

`ThirdPartyNotices.txt` is installed with the standalone application and inside the VST3/AU bundle resources. Keep it current when adding, removing, or replacing dependencies. Private/product forks should update any source-availability language that points at this public repository.

For notarized macOS release builds, `scripts/makedist-mac.sh` also accepts:

- `NOTARIZE_BUNDLE_ID`
- `NOTARIZE_BUNDLE_ID_DEMO`
- `APP_SPECIFIC_ID`
- `APP_SPECIFIC_PWD`

These settings only affect installer/package identity. Plug-in identity, DAW compatibility, and saved-session compatibility are controlled elsewhere, including `config.h`, Xcode bundle identifiers, and plug-in format metadata.
