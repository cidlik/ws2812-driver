# ws2812 driver

## Описание

Драйвер позволяет управлять светодиодной лентой с адресными светодиодами WS2812.
В системе создается устройство `/dev/ws2812`, которому на вход можно передать:

* `reset` - сброс светодиодной ленты;
* `<LED:COLOR_CODE>` - зажечь светодиод `LED` цветом `COLOR_CODE`.

Для задания максимальной длины ленты используется атрибут `strip_len`:

```
/sys/class/ws2812_class/ws2812/strip_len
```

Драйвер тестировался на Raspberry Pi 4 Model B. Сборка выполнялась на целевом
устройстве.

## dts overlay

Для работы драйвера требуется скомпилировать DTBO (device-tree binary overlay)
(см. `ws2812.dtso`)

```
dtc -O dtb -o ws2812.dtbo <PATH_TO_DTSO>
```

и добавить его в `/boot/config.txt`:

```
# Enable DRM VC4 V3D driver
dtoverlay=vc4-kms-v3d
dtoverlay=ws2812
max_framebuffers=2
```

Скопировать dtbo в `/boot/overlays`.

## Быстрый тест

Загрузить драйвер и запустить:

```
while true; do
    for i in $(seq 1 8); do 
        echo $i:888888 > /dev/ws2812; sleep 0.04
    done
done
```

На ленте должен забегать белый цвет.
