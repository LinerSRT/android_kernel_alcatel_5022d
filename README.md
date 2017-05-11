# Alcatel 5022D
#### Kernel source 3.10.105 (updated) MT6580

[![N|Solid](http://srtmemory.cloudapp.net/vk_btn.png)](https://vk.com/serini_ty)
[![N|Solid](http://srtmemory.cloudapp.net/4pda_btn.png)](http://4pda.ru/forum/index.php?showuser=4548849)

###  Remastered kernel source code for Alcatel 5022D
### Downloading and compiling

```sh
$ cd && git clone https://github.com/SeriniTY320/android_kernel_alcatel_5022d.git
$ cd android_kernel_alcatel_5022d
$ export ARCH=arm && export KBUILD_BUILD_USER=serinity && export KBUILD_BUILD_HOST=smartromteam
$ make tcl5022d_defconfig
$ make -j* (*-threads)
```

# About phone
![5022D](https://avatars.mds.yandex.net/get-mpic/200316/img_id9222652967177461589/9)

_|_
------------ | -------------
CPU | MT6580
RAM |  1 Gb
ROM | 8 Gb
Screen | 5", 720 x 1280, IPS
GPU | Mali-400 MP2

# Drivers used
_|_
------------ | -------------
ALS/PS | stk3x1x_new
Accel |  lis3dh
LCM | otm1287a_dsi_vdo ili9881c_dsi_vdo
Touchscreen | Focaltech
Imgsensors | s5k4h5yc_mipi_raw s5k4h6yc_mipi_raw ov5670_mipi_raw s5k5e2ya_mipi_raw
Charger | BQ24158

