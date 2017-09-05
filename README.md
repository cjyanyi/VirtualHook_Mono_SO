VirtualHook
-----------

## Introduction 

VirtualHook is a tool for hooking application without root permission. It is based on two projects:

- [VirtualApp](https://github.com/asLody/VirtualApp). It's a plugin framework which allows running applications in its virtual space.
- [YAHFA](https://github.com/rk700/YAHFA). It's a hook framework for ART which allows hooking Java method of the application.

Currently VirtualHook supports:

- Android 5.0(API 21)
- Android 5.1(API 22)
- Android 6.0(API 23)
- __EXPERIMENTAL__ Android 7.0(API 24)
- __EXPERIMENTAL__ Android 7.1(API 25)

## ChangeLog

Basically, VirtualHook makes the following changes to the upstream:

- Inject YAHFA when the app is started: https://github.com/rk700/VirtualHook/commit/5c530eeb241ba4ff5a3d34831b5c9af21ae4b0d8
- ~~Use MSHook instead of GodinHook: https://github.com/rk700/VirtualHook/commit/6ab8078d24e25d724b7d20b990f8589d02e8429c~~
- Remove Admob and Flurry
- Install hooks manually instead of loading them from sdcard automatically: https://github.com/rk700/VirtualHook/commit/a95ba5184c22d3e91953ff603f47e8955c7c4c49
- Do preopt when installing plugins: https://github.com/rk700/VirtualHook/commit/c444f5e7c8c6628a498e16a56969c7e160ff6cb0

## Build

Import and build the project in Android Studio(__with Instant Run disabled__). There are four modules:

- `app`. This is the VirtualApp application module.
- `lib`. This is the VirtualApp library module.
- `YAHFA`. This is the YAHFA hook module.
- `demoHookPlugin`. This is a demo hook plugin which compiles to an APK.

## Usage

- Write and build a hook plugin APK. You can take a look at the [demoHookPlugin](/VirtualApp/demoHookPlugin) module for reference. __Don't forget to put following meta-data in AndroidManifest.xml:__

```xml
    <application
        android:label="@string/app_name">
        <meta-data
            android:name="yahfa.hook.plugin"
            android:value="true"
        />
    </application>
```

- Push the plugin APK to sdcard
- Run VirtualHook and click the `Add` button
- Swipe to the 'APPS IN SDCARD' page. Then select and add hook plugins which are displayed with an icon ![](/VirtualApp/app/src/main/res/drawable-xxhdpi/ic_extension_black_24dp.png)
- Add and run non-plugin apps 

## Hooking Native Methods

VirtualApp comes with native method hooking ability in the first place, which is done with the following function:

```cpp
namespace Cydia{
    void MSHookFunction(void *symbol, void *replace, void **result);
}
```

To utilize that, you can use `dlsym()` to find the symbol and then hook your targets. Here's a [demo](https://github.com/rk700/ChangePhoneInfo/blob/master/app/src/main/jni/hookprop.c) which hooks `__system_property_get`.

## Example Hook Plugins

- [CertUnpinning](https://github.com/rk700/CertUnpinning): HTTPS certificate unpinning.
- [ChangePhoneInfo](https://github.com/rk700/ChangePhoneInfo): Change device information.
- [VirtualHosts](https://github.com/rk700/VirtualHosts): Custom hostname resolution.

## License

Both VirtualApp and YAHFA are distributed under GNU GPL V3.
