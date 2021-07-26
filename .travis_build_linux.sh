#!/bin/bash

echo "*****************************"
echo "Building SocNetV for linux..."
echo "*****************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"

echo "TRAVIS_TAG is: $TRAVIS_TAG"
echo "TRAVIS_COMMIT is: $TRAVIS_COMMIT"

echo "SOCNETV_VERSION is: $SOCNETV_VERSION"

LAST_COMMIT_SHORT=$(git rev-parse --short HEAD)
echo "LAST_COMMIT_SHORT is: $LAST_COMMIT_SHORT"

# linuxdeployqt always uses the output of 'git rev-parse --short HEAD' as the version.
# We can change this by exporting $VERSION environment variable 

if [ ! -z "$TRAVIS_TAG" ] ; then
    # If this is a tag, then version will be the tag, i.e. 2.6 or 2.6-beta
    export VERSION=${TRAVIS_TAG}
else 
    # If this is not a tag, the we want version to be like "2.6-beta-a0be9cd"
    export VERSION=${SOCNETV_VERSION}-${LAST_COMMIT_SHORT}
fi


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    source /opt/qt512/bin/qt512-env.sh
    qmake # default: all go to /usr
    make -j4

    echo "Building finished! "
    echo ""
    echo "Files in current directory: "
    find .
    echo ""
    echo "Attempting make install in appdir: "
    make INSTALL_ROOT=appdir install;
    echo ""
    echo "SocNetV files installed in appdir: "
    find appdir/
    echo "Copying .desktop file to ./appdir: "
    cp appdir/usr/share/applications/socnetv.desktop ./appdir
    echo "Copying socnetv.png in current dir: "
    cp appdir/usr/share/pixmaps/socnetv.png .  
    echo "copying custom openssl libs to ./appdir/usr/bin..."
    cp /opt/openssl-1.1.1/lib/libssl.so.1.1 ./appdir/usr/bin/
    cp /opt/openssl-1.1.1/lib/libcrypto.so.1.1 ./appdir/usr/bin/
    echo ""
    echo "SocNetV files installed in appdir -- final: "
    find appdir/
    echo ""
    echo "Check SocNetV executable libraries:"
    ldd appdir/usr/bin/socnetv
    echo ""
    echo "Checking contents of /opt/qtXX/plugins: "
    find /opt/qt512/plugins
    echo "Downloading linuxdeployqt tool: "
    wget --no-verbose  "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    echo "Make executable the linuxdeployqt tool: "
    chmod a+x linuxdeployqt*.AppImage
    unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
    echo ""
    echo "Check SocNetV executable libraries (AGAIN):"
    ldd appdir/usr/bin/socnetv

    # export VERSION=... # linuxdeployqt uses this for naming the file
    echo "Run the linuxdeployqt tool: "
    ./linuxdeployqt*.AppImage appdir/usr/share/applications/*.desktop -appimage -extra-plugins=iconengines,imageformats

    echo "Check what we have created..."
    find . -type f -name "*AppImage"


elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
	# nothing here, go away...
    echo "Strange. I am running in macOS although I am a Linux build script :)"
else
	exit 1
fi

echo "Done!"
# always return zero
exit 0
