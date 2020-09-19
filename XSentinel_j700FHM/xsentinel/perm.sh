#!/sbin/sh

chmod 0644 /system/vendor/lib/libsecure_storage.so
chmod 0644 /system/vendor/lib64/libsecure_storage.so
chmod 0644 /system/lib/libbauthserver.so
chmod 0644 /system/lib64/libbauthserver.so
chmod 0777 /system/etc/init.d
chmod 0700 /system/su.d

chown 0:0 /system/xbin/*
chmod 755 /system/xbin/*
chcon u:object_r:system_file:s0 /system/xbin/*

chown 0:0 /system/etc/init.d/*
chmod 777 /system/etc/init.d/*
chcon u:object_r:system_file:s0 /system/etc/init.d/*

chown 0:0 /system/su.d/*
chmod 700 /system/su.d/*
chcon u:object_r:system_file:s0 /system/su.d/*




