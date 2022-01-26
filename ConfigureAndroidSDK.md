Configure Android SDK
=====================

Linux (without AndroidStudio)
-----------------------------

Download the latest Android command line tools for Linux:

```console
~/Downloads$ wget -q https://dl.google.com/android/repository/commandlinetools-linux-7583922_latest.zip
```

Create a directory for the Android SDK, unzip the command line tools and rename it to `latest`:

```console
~$ mkdir -p Android/Sdk/cmdline-tools && cd Android/Sdk
~/Android/Sdk/cmdline-tools$ unzip ~/Downloads/commandlinetools-linux-7583922_latest.zip
~/Android/Sdk/cmdline-tools$ mv cmdline-tools latest
```

From the SDK root, install the required packages using `sdkmanager`:

```console
~/Android/Sdk$ cmdline-tools/latest/bin/sdkmanager --install "platforms;android-26" "platforms;android-29" "build-tools;32.0.0 "ndk;23.1.7779620"
```

The output of `sdkmanager --list_installed` should look like this:

```console
~/Android/Sdk$ cmdline-tools/latest/bin/sdkmanager --list_installed
Installed packages:=====================] 100% Fetch remote repository...
  Path                 | Version      | Description                     | Location
  -------              | -------      | -------                         | -------
  build-tools;29.0.2   | 29.0.2       | Android SDK Build-Tools 29.0.2  | build-tools/29.0.2
  build-tools;32.0.0   | 32.0.0       | Android SDK Build-Tools 32      | build-tools/32.0.0
  emulator             | 31.1.4       | Android Emulator                | emulator
  ndk;23.1.7779620     | 23.1.7779620 | NDK (Side by side) 23.1.7779620 | ndk/23.1.7779620
  patcher;v4           | 1            | SDK Patch Applier v4            | patcher/v4
  platform-tools       | 31.0.3       | Android SDK Platform-Tools      | platform-tools
  platforms;android-26 | 2            | Android SDK Platform 26         | platforms/android-26
  platforms;android-29 | 5            | Android SDK Platform 29         | platforms/android-29

```

Finally, make sure the following environment variables are defined and have the correct values:

- `ANDROID_HOME` should point to the Android SDK root directory (e.g. `~/Android/Sdk`);
- `ANDROID_NDK_HOME` should point to the NDK root directory (e.g. `$ANDROID_HOME/ndk/23.1.7779620`);
- update `PATH` to include `$ANDROID_HOME/cmdline-tools/latest/bin` and `$ANDROID_HOME/tools`.

> See [Environment Variables](https://developer.android.com/studio/command-line/variables) for more informations.

Also make sure that a Java installation exists and is correctly setup.

The following scripts should take care of everything (append it to `~/.profile`):

```bash
# Setup Java
if [ -d "/usr/lib/jvm/default-java" ]; then
    export JAVA_HOME="/usr/lib/jvm/default-java"
    PATH="$JAVA_HOME/bin:$PATH"
fi

# Setup Android SDK
if [ -d "$HOME/Android/Sdk" ]; then
    export ANDROID_HOME="$HOME/Android/Sdk"

    # Setup NDK
    if [ -d "$ANDROID_HOME/ndk/latest" ]; then
        export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/latest"
    elif [ -d "$ANDROID_HOME/ndk" ]; then
        # Use first ndk folder
        NDK_VERSION=$(ls "$ANDROID_HOME/ndk" | head)
        export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/$NDK_VERSION"
    fi

    # Udpate path to include tools
    PATH="$ANDROID_HOME/cmdline-tools/latest/bin:$PATH"
    PATH="$ANDROID_HOME/tools/bin:$PATH"
fi
```

Useful resources
----------------

- [Oculus Developer Center](https://developer.oculus.com/)
- [Android Developers](https://developer.android.com/)
