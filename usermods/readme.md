# Usermods

This folder serves as a repository for usermods (custom `usermod.cpp` files)!

If you have created a usermod you believe is useful (for example to support a particular sensor, display, feature...), feel free to contribute by opening a pull request!

In order for other people to be able to have fun with your usermod, please keep these points in mind:

* Create a folder in this folder with a descriptive name (for example `usermod_ds18b20_temp_sensor_mqtt`)  
* Include your custom files 
* If your usermod requires changes to other WLED files, please write a `readme.md` outlining the steps one needs to take  
* Create a pull request!  
* If your feature is useful for the majority of WLED users, I will consider adding it to the base code!  

While I do my best to not break too much, keep in mind that as WLED is updated, usermods might break.  
I am not actively maintaining any usermod in this directory, that is your responsibility as the creator of the usermod.

For new usermods, I would recommend trying out the new v2 usermod API, which allows installing multiple usermods at once and new functions!
You can take a look at `EXAMPLE_v2` for some documentation and at `Temperature` for a completed v2 usermod!

Thank you for your help :)

## MP3 Sound Module — DY-SV17F / JQ6500 serial MP3 module (`dy_sv17f`)

A v2 usermod that plays sound effects from a serial MP3 module, driven by a
physical push button. Supports **DY-SV17F** (`0xAA` protocol) and **JQ6500**
(`0x7E` protocol), selected with a dropdown on the Usermods page. The usermod is
labeled **"MP3 Sound Module"**.

* Sequential (1 → N → 1) or random sound-effect cycling
* Configurable module, volume (0-30), track count, button GPIO and UART pins on
  the *Settings > Usermods* page (persisted in `cfg.json`)
* UART command protocol at 9600 baud, 8N1
* Compile-time enable: `-D WLED_USERMOD_DY_SV17F`

See `dy_sv17f/readme.md` for wiring, configuration and the command tables, or the
upstream repository <https://github.com/Blowntobytes/WLED-MP3-sound-module-Usermod>.