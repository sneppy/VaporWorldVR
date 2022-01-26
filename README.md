# VaporWorldVR

A simple VR application that targets Meta Quest 2 VR headset.

Contributors
------------

- Andrea Mecchia @ [sneppy](https://github.com/sneppy)

Building the application
------------------------

Before cloning the repository, download and unzip the [Oculus Mobile SDK](https://developer.oculus.com/downloads/package/oculus-mobile-sdk/):

```console
~$ cd Projects
~/Projects$ mkdir ovr_sdk_mobile && cd ovr_sdk_mobile
~/Projects/ovr_sdk_mobile$ unzip ~/Downloads/ovr_sdk_mobile_1.50.0
```

Create a directory in the SDK where to place your VR projects, and clone this repository:

```console
~/Projects/ovr_sdk_mobile$ mkdir VrProject && cd VrProjects
~/Projects/ovr_sdk_mobile/VrProjects$ git clone git@github.com:sneppy/VaporWorldVR.git
```

Alternatively, you can clone the repository somewhere else and create a symbolic link:

```console
~/Projects/ovr_sdk_mobile/VrProjects$ ln -s <path_to_repo>
```

Cd into the Android project, and run `gradlew`:

```console
~/Projects/ovr_sdk_mobile/VrProjects/Projects/Android$ ../../../../gradlew
```

If you haven't configured the Android SDK, or you misconfigured it, you will see some errors. See [Configure Android SDK](ConfigureAndroidSDK.md) for more informations.

Finally, run the `build` task to build and test the application, and the `install(Debug|Release)` task to install the application on the connected device:

```console
~/Projects/ovr_sdk_mobile/VrProjects/Projects/Android$ ../../../../gradlew installRelease
```

> If you are having problems connecting the headset, follow the steps on the [Device Setup](https://developer.oculus.com/documentation/native/android/mobile-device-setup/) page.

You can also run `gradlew tasks` to get a list of all available tasks.
