VirtualHook demoHookPlugin
--------------------------

Here is a demo plugin which contains the hook info. It hooks the following methods:

- AssetManager.open()
- File()
- URL.openConnection()

The arguments would be logged and then the original method be called.

## Usage

Build the plugin and you'll get an APK file. Push the APK to sdcard, then add the hook and run applications in VirtualHook.

Please take a look at [the demo plugin in YAHFA](https://github.com/rk700/YAHFA/tree/master/demoPlugin) on how to write a hook plugin.
