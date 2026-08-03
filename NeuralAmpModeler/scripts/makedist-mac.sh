#!/bin/bash

# this script requires xcpretty https://github.com/xcpretty/xcpretty
set -o pipefail

BASEDIR=$(dirname $0)

cd $BASEDIR/..

if [ -d build-mac ]; then
  rm -f -R build-mac
fi

mkdir -p build-mac

#---------------------------------------------------------------------------------------------------------
#variables

IPLUG2_ROOT=../iPlug2
XCCONFIG=$IPLUG2_ROOT/../common-mac.xcconfig
SCRIPTS=$IPLUG2_ROOT/Scripts

if [ -z "$DEVELOPER_DIR" ] && [ -d "/Applications/Xcode.app/Contents/Developer" ]; then
  export DEVELOPER_DIR="/Applications/Xcode.app/Contents/Developer"
fi

# CODESIGN is disabled by default. Set CODESIGN=1 in a configured release
# environment after importing the required Developer ID certificates.
CODESIGN=${CODESIGN:-0}

# macOS codesigning/notarization
INSTALLER_PKG_ID_PREFIX=${INSTALLER_PKG_ID_PREFIX:-com.pedaldivision}
APP_SPECIFIC_ID=${APP_SPECIFIC_ID:-TODO}
APP_SPECIFIC_PWD=${APP_SPECIFIC_PWD:-TODO}

DEMO=0
if [ "$1" == "demo" ]; then
  DEMO=1
fi

BUILD_INSTALLER=1
if [ "$2" == "zip" ]; then
  BUILD_INSTALLER=0
fi

REQUIRE_ALL_FORMATS=${REQUIRE_ALL_FORMATS:-1}

VERSION=`echo | grep "^#define PLUG_VERSION_HEX " config.h`
VERSION=${VERSION//\#define PLUG_VERSION_HEX }
VERSION=${VERSION//\'}
MAJOR_VERSION=$(($VERSION & 0xFFFF0000))
MAJOR_VERSION=$(($MAJOR_VERSION >> 16))
MINOR_VERSION=$(($VERSION & 0x0000FF00))
MINOR_VERSION=$(($MINOR_VERSION >> 8))
BUG_FIX=$(($VERSION & 0x000000FF))

FULL_VERSION=$MAJOR_VERSION"."$MINOR_VERSION"."$BUG_FIX

PLUGIN_NAME=`echo | grep "^#define BUNDLE_NAME " config.h`
PLUGIN_NAME=${PLUGIN_NAME//\#define BUNDLE_NAME }
PLUGIN_NAME=${PLUGIN_NAME//\"}

DISPLAY_NAME=`echo | grep "^#define PLUG_NAME " config.h`
DISPLAY_NAME=${DISPLAY_NAME//\#define PLUG_NAME }
DISPLAY_NAME=${DISPLAY_NAME//\"}

PROJECT_NAME=${PROJECT_NAME:-$(basename "$PWD")}
PROJECT_FILE=${PROJECT_FILE:-./projects/$PROJECT_NAME-macOS.xcodeproj}
PROJECT_XCCONFIG=${PROJECT_XCCONFIG:-./config/$PROJECT_NAME-mac.xcconfig}

NOTARIZE_BUNDLE_ID=${NOTARIZE_BUNDLE_ID:-${INSTALLER_PKG_ID_PREFIX}.${PLUGIN_NAME}}
NOTARIZE_BUNDLE_ID_DEMO=${NOTARIZE_BUNDLE_ID_DEMO:-${INSTALLER_PKG_ID_PREFIX}.${PLUGIN_NAME}.DEMO}

ARCHIVE_NAME=$PLUGIN_NAME-v$FULL_VERSION-mac
THIRD_PARTY_NOTICES="./installer/ThirdPartyNotices.txt"
EXAMPLE_COLLECTION="../distribution/nam-division-collection"
README_PDF="manual/NAM Division README.pdf"
AMP_TEMPLATE="../docs/images/amp-template.jpg"
CAB_TEMPLATE="../docs/images/cab-template.jpg"

stage_example_collection()
{
  target_directory=$1

  if [ ! -d "$EXAMPLE_COLLECTION" ]; then
    echo "ERROR: Example collection not found at $EXAMPLE_COLLECTION"
    exit 1
  fi

  mkdir -p "$target_directory"
  cp -R "$EXAMPLE_COLLECTION" "$target_directory/" || exit 1
  find "$target_directory/$(basename "$EXAMPLE_COLLECTION")" -type f -name ".DS_Store" -delete || exit 1
}

stage_thumbnail_templates()
{
  target_directory=$1

  for template in "$AMP_TEMPLATE" "$CAB_TEMPLATE"; do
    if [ ! -f "$template" ]; then
      echo "ERROR: Thumbnail template not found at $template"
      exit 1
    fi

    cp "$template" "$target_directory/" || exit 1
  done
}

copy_third_party_notices()
{
  bundle_path=$1

  if [ -d "$bundle_path" ] && [ -f "$THIRD_PARTY_NOTICES" ]; then
    mkdir -p "$bundle_path/Contents/Resources"
    cp "$THIRD_PARTY_NOTICES" "$bundle_path/Contents/Resources/"
  fi
}

stage_for_installer()
{
  rm -f -R "build-mac/$PLUGIN_NAME.app" "build-mac/$PLUGIN_NAME.component" "build-mac/$PLUGIN_NAME.vst3"

  if [ -d "$APP" ]; then
    cp -R "$APP" "build-mac/$PLUGIN_NAME.app"
  fi

  if [ -d "$AU" ]; then
    cp -R "$AU" "build-mac/$PLUGIN_NAME.component"
  fi

  if [ -d "$VST3" ]; then
    cp -R "$VST3" "build-mac/$PLUGIN_NAME.vst3"
  fi

}

VST3_SDK_READY=0
if [ -f "$IPLUG2_ROOT/Dependencies/IPlug/VST3_SDK/public.sdk/source/vst/vstrepresentation.cpp" ]; then
  VST3_SDK_READY=1
fi

if [ $DEMO == 1 ]; then
  ARCHIVE_NAME=$ARCHIVE_NAME-demo
fi

# TODO: use get_archive_name script
# if [ $DEMO == 1 ]; then
#   ARCHIVE_NAME=`python3 ${SCRIPTS}/get_archive_name.py ${PLUGIN_NAME} mac demo`
# else
#   ARCHIVE_NAME=`python3 ${SCRIPTS}/get_archive_name.py ${PLUGIN_NAME} mac full`
# fi

STAGE_ROOT="$PWD/build-mac/stage"
APP_STAGE_PATH="$STAGE_ROOT/app"
AU_STAGE_PATH="$STAGE_ROOT/au"
VST3_STAGE_PATH="$STAGE_ROOT/vst3"

VST3="$VST3_STAGE_PATH/$PLUGIN_NAME.vst3"
AU="$AU_STAGE_PATH/$PLUGIN_NAME.component"
APP="$APP_STAGE_PATH/$PLUGIN_NAME.app"

PKG="build-mac/installer/$PLUGIN_NAME Installer.pkg"
PKG_US="build-mac/installer/$PLUGIN_NAME Installer.unsigned.pkg"

CERT_ID=`echo | grep CERTIFICATE_ID $XCCONFIG`
CERT_ID=${CERT_ID//\CERTIFICATE_ID = }
DEV_ID_APP_STR="Developer ID Application: ${CERT_ID}"
DEV_ID_INST_STR="Developer ID Installer: ${CERT_ID}"

echo $VST3
echo $AU
echo $APP

if [ $DEMO == 1 ]; then
 echo "making $DISPLAY_NAME ($PLUGIN_NAME) version $FULL_VERSION DEMO mac distribution..."
#   cp "resources/img/AboutBox_Demo.png" "resources/img/AboutBox.png"
else
 echo "making $DISPLAY_NAME ($PLUGIN_NAME) version $FULL_VERSION mac distribution..."
#   cp "resources/img/AboutBox_Registered.png" "resources/img/AboutBox.png"
fi

sleep 2

echo "touching source to force recompile"
echo ""
touch *.cpp

#---------------------------------------------------------------------------------------------------------
#remove existing binaries

echo "remove existing binaries"
echo ""

rm -f -R "$STAGE_ROOT"
mkdir -p "$APP_STAGE_PATH" "$AU_STAGE_PATH" "$VST3_STAGE_PATH"

if [ -d $APP ]; then
  rm -f -R -f $APP
fi

if [ -d $AU ]; then
 rm -f -R $AU
fi

if [ -d $VST3 ]; then
  rm -f -R $VST3
fi

#---------------------------------------------------------------------------------------------------------
# build xcode project. The distributable formats are only built when their SDKs are available.

BUILD_TARGETS=("APP" "AU")

if [ $VST3_SDK_READY == 1 ]; then
  BUILD_TARGETS+=("VST3")
else
  if [ $REQUIRE_ALL_FORMATS == 1 ]; then
    echo "ERROR: VST3 SDK not found."
    echo "Install it with:"
    echo "  cd ../iPlug2/Dependencies/IPlug"
    echo "  ./download-vst3-sdk.sh"
    echo ""
    echo "For a local AU/App-only build, rerun with REQUIRE_ALL_FORMATS=0."
    exit 1
  else
    echo "VST3 SDK not found; skipping VST3 build"
  fi
fi

rm -f build-mac.log
for BUILD_TARGET in "${BUILD_TARGETS[@]}"; do
  echo "building $BUILD_TARGET"
  BUILD_DSTROOT="$STAGE_ROOT/$BUILD_TARGET"
  case "$BUILD_TARGET" in
    APP)
      BUILD_DSTROOT="$APP_STAGE_PATH"
      ;;
    AU)
      BUILD_DSTROOT="$AU_STAGE_PATH"
      ;;
    VST3)
      BUILD_DSTROOT="$VST3_STAGE_PATH"
      ;;
  esac

  if command -v xcpretty >/dev/null 2>&1; then
    xcodebuild -project "$PROJECT_FILE" -xcconfig "$PROJECT_XCCONFIG" DEMO_VERSION=$DEMO APP_PATH="$APP_STAGE_PATH" AU_PATH="$AU_STAGE_PATH" VST3_PATH="$VST3_STAGE_PATH" DSTROOT="$BUILD_DSTROOT" -target "$BUILD_TARGET" -UseModernBuildSystem=NO -configuration Release | tee -a build-mac.log | xcpretty
    BUILD_RESULT=${PIPESTATUS[0]}
  else
    echo "xcpretty not found; using raw xcodebuild output"
    xcodebuild -project "$PROJECT_FILE" -xcconfig "$PROJECT_XCCONFIG" DEMO_VERSION=$DEMO APP_PATH="$APP_STAGE_PATH" AU_PATH="$AU_STAGE_PATH" VST3_PATH="$VST3_STAGE_PATH" DSTROOT="$BUILD_DSTROOT" -target "$BUILD_TARGET" -UseModernBuildSystem=NO -configuration Release | tee -a build-mac.log
    BUILD_RESULT=${PIPESTATUS[0]}
  fi

  if [ "$BUILD_RESULT" -ne "0" ]; then
    echo "ERROR: $BUILD_TARGET build failed, aborting"
    echo ""
    exit 1
  fi
done
rm build-mac.log

#---------------------------------------------------------------------------------------------------------
# set bundle icons - http://www.hamsoftengineering.com/codeSharing/SetFileIcon/SetFileIcon.html

echo "setting icons"
echo ""

if [ -d $AU ]; then
  ./$SCRIPTS/SetFileIcon -image resources/$PLUGIN_NAME.icns -file $AU
fi

if [ -d $VST3 ]; then
  ./$SCRIPTS/SetFileIcon -image resources/$PLUGIN_NAME.icns -file $VST3
fi

#---------------------------------------------------------------------------------------------------------
#strip symbols from binaries

echo "stripping binaries"
echo ""

if [ -d $APP ]; then
  strip -x $APP/Contents/MacOS/$PLUGIN_NAME
fi

if [ -d $AU ]; then
  strip -x $AU/Contents/MacOS/$PLUGIN_NAME
fi

if [ -d $VST3 ]; then
  strip -x $VST3/Contents/MacOS/$PLUGIN_NAME
fi

echo "copying third-party notices"
echo ""

copy_third_party_notices "$APP"
copy_third_party_notices "$AU"
copy_third_party_notices "$VST3"

if [ $CODESIGN == 1 ]; then
  #---------------------------------------------------------------------------------------------------------
  echo "code-sign binaries"
  echo ""

  codesign --force -s "${DEV_ID_APP_STR}" -v $APP --deep --strict --options=runtime #hardened runtime for app
  xattr -cr $AU 
  codesign --force -s "${DEV_ID_APP_STR}" -v $AU --deep --strict
  xattr -cr $VST3 
  codesign --force -s "${DEV_ID_APP_STR}" -v $VST3 --deep --strict
  #---------------------------------------------------------------------------------------------------------
fi

if [ $BUILD_INSTALLER == 1 ]; then
  #---------------------------------------------------------------------------------------------------------
  # installer

  rm -R -f build-mac/$PLUGIN_NAME-*.dmg
  stage_for_installer

  echo "building installer"
  echo ""

  PRODUCT_NAME="$PLUGIN_NAME" INSTALLER_DISPLAY_NAME="$DISPLAY_NAME" ./scripts/makeinstaller-mac.sh $FULL_VERSION

  if [ $CODESIGN == 1 ]; then
    echo "code-sign installer for Gatekeeper on macOS 10.8+"
    echo ""
    mv "${PKG}" "${PKG_US}"
    productsign --sign "${DEV_ID_INST_STR}" "${PKG_US}" "${PKG}"
    rm -R -f "${PKG_US}"
  fi

  #set installer icon
  ./$SCRIPTS/SetFileIcon -image resources/$PLUGIN_NAME.icns -file "${PKG}"

  echo "adding example collection"
  echo ""
  stage_example_collection "build-mac/installer"
  stage_thumbnail_templates "build-mac/installer"

  #---------------------------------------------------------------------------------------------------------
  # make dmg, can use dmgcanvas http://www.araelium.com/dmgcanvas/ to make a nice dmg, fallback to hdiutil
  echo "building dmg"
  echo ""

  if [ -d installer/$PLUGIN_NAME.dmgCanvas ]; then
    dmgcanvas installer/$PLUGIN_NAME.dmgCanvas build-mac/$ARCHIVE_NAME.dmg
  else
    cp installer/changelog.txt build-mac/installer/
    cp installer/known-issues.txt build-mac/installer/
    if [ ! -f "$README_PDF" ]; then
      echo "ERROR: Rendered README not found at $README_PDF"
      exit 1
    fi
    cp "$README_PDF" build-mac/installer/
    hdiutil create build-mac/$ARCHIVE_NAME.dmg -format UDZO -srcfolder build-mac/installer/ -ov -anyowners -volname $PLUGIN_NAME
  fi

  rm -R -f build-mac/installer/

  if [ $CODESIGN == 1 ]; then
    #---------------------------------------------------------------------------------------------------------
    #notarize dmg
    echo "notarizing"
    echo ""
    # you need to create an app-specific id/password https://support.apple.com/en-us/HT204397
    # arg 1 Set to the dmg path
    # arg 2 Set to a bundle ID (doesn't have to match your )
    # arg 3 Set to the app specific Apple ID username/email
    # arg 4 Set to the app specific Apple password  
    PWD=`pwd`

    if [ $DEMO == 1 ]; then
      ./$SCRIPTS/notarise.sh "${PWD}/build-mac" "${PWD}/build-mac/${ARCHIVE_NAME}.dmg" $NOTARIZE_BUNDLE_ID_DEMO $APP_SPECIFIC_ID $APP_SPECIFIC_PWD
    else
      ./$SCRIPTS/notarise.sh "${PWD}/build-mac" "${PWD}/build-mac/${ARCHIVE_NAME}.dmg" $NOTARIZE_BUNDLE_ID $APP_SPECIFIC_ID $APP_SPECIFIC_PWD
    fi

    if [ "${PIPESTATUS[0]}" -ne "0" ]; then
      echo "ERROR: notarize script failed, aborting"
      exit 1
    fi

  fi
else
  #---------------------------------------------------------------------------------------------------------
  # zip

  if [ -d build-mac/zip ]; then
    rm -R build-mac/zip
  fi

  mkdir -p build-mac/zip
  stage_example_collection "build-mac/zip"
  stage_thumbnail_templates "build-mac/zip"

  if [ -d $APP ]; then
    cp -R $APP build-mac/zip/$PLUGIN_NAME.app
  fi

  if [ -d $AU ]; then
    cp -R $AU build-mac/zip/$PLUGIN_NAME.component
  fi

  if [ -d $VST3 ]; then
    cp -R $VST3 build-mac/zip/$PLUGIN_NAME.vst3
  fi

  echo "zipping binaries..."
  echo ""
  ditto -c -k build-mac/zip build-mac/$ARCHIVE_NAME.zip
  rm -R build-mac/zip
fi

#---------------------------------------------------------------------------------------------------------
# dSYMs
rm -R -f build-mac/*-dSYMs.zip

echo "packaging dSYMs"
echo ""
zip -r ./build-mac/$ARCHIVE_NAME-dSYMs.zip ./build-mac/*.dSYM

#---------------------------------------------------------------------------------------------------------

# prepare out folder for CI

echo "preparing output folder"
echo ""
mkdir -p ./build-mac/out
if [ -f ./build-mac/$ARCHIVE_NAME.dmg ]; then
  mv ./build-mac/$ARCHIVE_NAME.dmg ./build-mac/out
fi
mv ./build-mac/*.zip ./build-mac/out

#---------------------------------------------------------------------------------------------------------

#if [ $DEMO == 1 ]
#then
#  git checkout installer/NeuralAmpModeler.iss
#  git checkout installer/NeuralAmpModeler.pkgproj
#  git checkout resources/img/AboutBox.png
#fi

echo "done!"
echo ""
