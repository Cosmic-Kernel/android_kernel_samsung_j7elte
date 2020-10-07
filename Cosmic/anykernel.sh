# Ramdisk Mod Script
# Cosmic Kernel by themagicalmammal

# shell variables
block=/dev/block/bootdevice/by-name/boot;
if [ ! -e $block ]; then
  block=/dev/block/bootdevice/by-name/BOOT;
fi
blockrecovery=/dev/block/bootdevice/by-name/recovery;
if [ ! -e $blockrecovery ]; then
  blockrecovery=/dev/block/bootdevice/by-name/RECOVERY;
fi

## end setup


## AnyKernel methods (DO NOT CHANGE)
# set up extracted files and directories
ramdisk=/tmp/anykernel/ramdisk;
bin=/tmp/anykernel/tools;
split_img=/tmp/anykernel/split_img;
patch=/tmp/anykernel/patch;

## Variables for essential
imgtype=$(grep image.type /tmp/anykernel/essential.prop)
if [ $imgtype = "image.type=1" ]; then
  imgtypevalue='1'
else
  imgtypevalue='0'
fi
imgdirpath=$(grep img.dirpath /tmp/anykernel/essential.prop)
if [ $imgdirpath = "img.dirpath=0" ]; then
  imgdirpathvalue='0'
else
  imgdirpathvalue='1'
fi
selinuxfix=$(grep fix.selinux /tmp/anykernel/essential.prop)
if [ $selinuxfix = "fix.selinux=1" ]; then
  selinuxfixvalue='1'
else
  selinuxfixvalue='0'
fi
flashimage=$(grep flash.img /tmp/anykernel/essential.prop)
if [ $flashimage = "flash.img=1" ]; then
  flashimagevalue='1'
else
  flashimagevalue='0'
fi

chmod -R 755 $bin;
mkdir -p $ramdisk $split_img;

OUTFD=/proc/self/fd/$1;
ui_print() { echo -e "ui_print $1\nui_print" > $OUTFD; }

# dump boot and extract ramdisk
dump_boot() {
  ui_print "--- Verifying Setup mode"
  if [ $imgtypevalue = 1 ]; then
    ui_print "----- Recovery.img mode selected"
    if [ $imgdirpathvalue = 0 ]; then
      ui_print "------- Taking .img file of this device"
      dd if=$blockrecovery of=/tmp/anykernel/boot.img;
    else
      ui_print "------- Taking .img file from zip"
      mv -f /tmp/anykernel/recovery.img /tmp/anykernel/boot.img
	fi
  else
    ui_print "----- Boot.img mode selected"
    if [ $imgdirpathvalue = 0 ]; then
      ui_print "------- Taking .img file of this device"
      dd if=$block of=/tmp/anykernel/boot.img;
    else
      ui_print "------- Taking .img file from zip"
    fi
  fi
  ui_print "--- Verification successful"
  ui_print " "
  ui_print "--- Unpacking the .img file"
  $bin/unpackbootimg -i /tmp/anykernel/boot.img -o $split_img;
  if [ $? != 0 ]; then
    ui_print " "; ui_print "----- Unpacking image failed. Aborting..."; exit 1;
  fi;
  mv -f $ramdisk /tmp/anykernel/rdtmp;
  mkdir -p $ramdisk;
  cd $ramdisk;
  gunzip -c $split_img/boot.img-ramdisk.gz | cpio -i;
  if [ $? != 0 -o -z "$(ls $ramdisk)" ]; then
    ui_print " "; ui_print "----- Unpacking ramdisk failed. Aborting..."; exit 1;
  fi;
  cp -af /tmp/anykernel/rdtmp/* $ramdisk;
  ui_print "--- Unpacking successful"
}

# repack ramdisk then build and write image
write_boot() {
  cd $split_img;
  cmdline=`cat *-cmdline`;
  board=`cat *-board`;
  base=`cat *-base`;
  pagesize=`cat *-pagesize`;
  kerneloff=`cat *-kerneloff`;
  ramdiskoff=`cat *-ramdiskoff`;
  tagsoff=`cat *-tagsoff`;
  if [ -f *-second ]; then
    second=`ls *-second`;
    second="--second $split_img/$second";
    secondoff=`cat *-secondoff`;
    secondoff="--second_offset $secondoff";
  fi;
  ui_print " "
  ui_print "--- Making necessary changes"
  if [ -f /tmp/anykernel/zImage ]; then
    ui_print "----- Changing kernel"
    kernel=/tmp/anykernel/zImage;
    ui_print "----- Changing successful"
  else
    kernel=`ls *-zImage`;
    kernel=$split_img/$kernel;
  fi;
  if [ -f /tmp/anykernel/dtb ]; then
    ui_print "----- Changing dt_image"
    dtb="--dt /tmp/anykernel/dtb";
    ui_print "----- Changing successful"
  elif [ -f *-dtb ]; then
    dtb=`ls *-dtb`;
    dtb="--dt $split_img/$dtb";
  fi;
  if [ -f /tmp/anykernel/dt_img ]; then
    ui_print "----- Changing dt_image"
    rm -f "$split_img/boot.img-dtb"
	cp -f /tmp/anykernel/dt_img "$split_img/boot.img-dtb"
    ui_print "----- Changing successful"
  fi;
  ui_print "----- Replaced ramdisk files are given"
  ui_print "                 below                "
  ### Files in / (root)
  if [ -f /tmp/anykernel/default.prop ]; then
    ui_print "------- Replacing default.prop"
    rm -f /tmp/anykernel/ramdisk/default.prop
    cp /tmp/anykernel/default.prop /tmp/anykernel/ramdisk/default.prop
    chmod 0644 /tmp/anykernel/ramdisk/default.prop
  fi
  if [ -f /tmp/anykernel/file_contexts ]; then
    ui_print "------- Replacing file_contexts"
    rm -f /tmp/anykernel/ramdisk/file_contexts
    cp /tmp/anykernel/file_contexts /tmp/anykernel/ramdisk/file_contexts
    chmod 0644 /tmp/anykernel/ramdisk/file_contexts
  fi
  if [ -f /tmp/anykernel/fstab.qcom ]; then
    ui_print "------- Replacing fstab.qcom"
    rm -f /tmp/anykernel/ramdisk/fstab.qcom
    cp /tmp/anykernel/fstab.qcom /tmp/anykernel/ramdisk/fstab.qcom
    chmod 0640 /tmp/anykernel/ramdisk/fstab.qcom
  fi
  if [ -f /tmp/anykernel/init ]; then
    ui_print "------- Replacing init"
    rm -f /tmp/anykernel/ramdisk/init
    cp /tmp/anykernel/init /tmp/anykernel/ramdisk/init
    chmod 0750 /tmp/anykernel/ramdisk/init
  fi
  if [ -f /tmp/anykernel/init.carrier.rc ]; then
    ui_print "------- Replacing init.carrier.rc"
    rm -f /tmp/anykernel/ramdisk/init.carrier.rc
    cp /tmp/anykernel/init.carrier.rc /tmp/anykernel/ramdisk/init.carrier.rc
    chmod 0750 /tmp/anykernel/ramdisk/init.carrier.rc
  fi
  if [ -f /tmp/anykernel/init.class_main.sh ]; then
    ui_print "------- Replacing init.class_main.sh"
    rm -f /tmp/anykernel/ramdisk/init.class_main.sh
    cp /tmp/anykernel/init.class_main.sh /tmp/anykernel/ramdisk/init.class_main.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.class_main.sh
  fi
  if [ -f /tmp/anykernel/init.container.rc ]; then
    ui_print "------- Replacing init.container.rc"
    rm -f /tmp/anykernel/ramdisk/init.container.rc
    cp /tmp/anykernel/init.container.rc /tmp/anykernel/ramdisk/init.container.rc
    chmod 0750 /tmp/anykernel/ramdisk/init.container.rc
  fi
  if [ -f /tmp/anykernel/init.environ.rc ]; then
    ui_print "------- Replacing init.environ.rc"
    rm -f /tmp/anykernel/ramdisk/init.environ.rc
    cp /tmp/anykernel/init.environ.rc /tmp/anykernel/ramdisk/init.environ.rc
    chmod 0750 /tmp/anykernel/ramdisk/init.environ.rc
  fi
  if [ -f /tmp/anykernel/init.mdm.sh ]; then
    ui_print "------- Replacing init.mdm.sh"
    rm -f /tmp/anykernel/ramdisk/init.mdm.sh
    cp /tmp/anykernel/init.mdm.sh /tmp/anykernel/ramdisk/init.mdm.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.mdm.sh
  fi
  if [ -f /tmp/anykernel/init.qcom.bms.sh ]; then
    ui_print "------- Replacing init.qcom.bms.sh"
    rm -f /tmp/anykernel/ramdisk/init.qcom.bms.sh
    cp /tmp/anykernel/init.qcom.bms.sh /tmp/anykernel/ramdisk/init.qcom.bms.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.bms.sh
  fi
  if [ -f /tmp/anykernel/init.qcom.class_core.sh ]; then
    ui_print "------- Replacing init.qcom.class_core.sh"
    rm -f /tmp/anykernel/ramdisk/init.qcom.class_core.sh
    cp /tmp/anykernel/init.qcom.class_core.sh /tmp/anykernel/ramdisk/init.qcom.class_core.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.class_core.sh
  fi
  if [ -f /tmp/anykernel/init.qcom.early_boot.sh ]; then
    ui_print "------- Replacing init.qcom.early_boot.sh"
    rm -f /tmp/anykernel/ramdisk/init.qcom.early_boot.sh
    cp /tmp/anykernel/init.qcom.early_boot.sh /tmp/anykernel/ramdisk/init.qcom.early_boot.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.early_boot.sh
  fi
  if [ -f /tmp/anykernel/init.qcom.factory.sh ]; then
    ui_print "------- Replacing init.qcom.factory.sh"
    rm -f /tmp/anykernel/ramdisk/init.qcom.factory.sh
    cp /tmp/anykernel/init.qcom.factory.sh /tmp/anykernel/ramdisk/init.qcom.factory.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.factory.sh
  fi
  if [ -f /tmp/anykernel/init.qcom.rc ]; then
    ui_print "------- Replacing init.qcom.rc"
    rm -f /tmp/anykernel/ramdisk/init.qcom.rc
    cp /tmp/anykernel/init.qcom.rc /tmp/anykernel/ramdisk/init.qcom.rc
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.rc
  fi
  if [ -f /tmp/anykernel/init.qcom.sh ]; then
    ui_print "------- Replacing init.qcom.sh"
    rm -f /tmp/anykernel/ramdisk/init.qcom.sh
    cp /tmp/anykernel/init.qcom.sh /tmp/anykernel/ramdisk/init.qcom.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.sh
  fi
  if [ -f /tmp/anykernel/init.qcom.syspart_fixup.sh ]; then
    ui_print "------- Replacing init.qcom.syspart_fixup.sh"
    rm -f /tmp/anykernel/ramdisk/init.qcom.syspart_fixup.sh
    cp /tmp/anykernel/init.qcom.syspart_fixup.sh /tmp/anykernel/ramdisk/init.qcom.syspart_fixup.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.syspart_fixup.sh
  fi
  if [ -f /tmp/anykernel/init.qcom.usb.rc ]; then
    ui_print "------- Replacing init.qcom.usb.rc"
    rm -f /tmp/anykernel/ramdisk/init.qcom.usb.rc
    cp /tmp/anykernel/init.qcom.usb.rc /tmp/anykernel/ramdisk/init.qcom.usb.rc
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.usb.rc
  fi
  if [ -f /tmp/anykernel/init.qcom.usb.sh ]; then
    ui_print "------- Replacing init.qcom.usb.sh"
    rm -f /tmp/anykernel/ramdisk/init.qcom.usb.sh
    cp /tmp/anykernel/init.qcom.usb.sh /tmp/anykernel/ramdisk/init.qcom.usb.sh
    chmod 0750 /tmp/anykernel/ramdisk/init.qcom.usb.sh
  fi
  if [ -f /tmp/anykernel/init.rc ]; then
    ui_print "------- Replacing init.rc"
    rm -f /tmp/anykernel/ramdisk/init.rc
    cp /tmp/anykernel/init.rc /tmp/anykernel/ramdisk/init.rc
    chmod 0750 /tmp/anykernel/ramdisk/init.rc
  fi
  if [ -f /tmp/anykernel/init.target.rc ]; then
    ui_print "------- Replacing init.target.rc"
    rm -f /tmp/anykernel/ramdisk/init.target.rc
    cp /tmp/anykernel/init.target.rc /tmp/anykernel/ramdisk/init.target.rc
    chmod 0750 /tmp/anykernel/ramdisk/init.target.rc
  fi
  if [ -f /tmp/anykernel/init.trace.rc ]; then
    ui_print "------- Replacing init.trace.rc"
    rm -f /tmp/anykernel/ramdisk/init.trace.rc
    cp /tmp/anykernel/init.trace.rc /tmp/anykernel/ramdisk/init.trace.rc
    chmod 0750 /tmp/anykernel/ramdisk/init.trace.rc
  fi

  ### Files in /sbin
  if [ -f /tmp/anykernel/sbin/kernel-init.sh ]; then
    ui_print "------- Replacing kernel-init.sh"
    rm -f /tmp/anykernel/ramdisk/sbin/kernel-init.sh
    cp /tmp/anykernel/sbin/kernel-init.sh /tmp/anykernel/ramdisk/sbin/kernel-init.sh
    chmod 0750 /tmp/anykernel/ramdisk/sbin/kernel-init.sh
  fi
  if [ -f /tmp/anykernel/sbin/qcom-setup.sh ]; then
    ui_print "------- Replacing qcom-setup.sh"
    rm -f /tmp/anykernel/ramdisk/sbin/qcom-setup.sh
    cp /tmp/anykernel/sbin/qcom-setup.sh /tmp/anykernel/ramdisk/sbin/qcom-setup.sh
    chmod 0750 /tmp/anykernel/ramdisk/sbin/qcom-setup.sh
  fi
  ui_print "----- Successful with ramdisk files"
  ui_print " "
  ui_print "--- Repacking ramdisk"
  cd $ramdisk;
  find . | cpio -H newc -o | gzip > /tmp/anykernel/ramdisk-new.cpio.gz;
  if [ $? != 0 ]; then
    ui_print " "; ui_print "--- Repacking ramdisk failed. Aborting..."; exit 1;
  fi;
  ui_print "--- Ramdisk successfully repacked"
  ui_print " "
  ui_print "--- Repacking .img file"
  $bin/mkbootimg --kernel $kernel --ramdisk /tmp/anykernel/ramdisk-new.cpio.gz $second --cmdline "$cmdline" --board "$board" --base $base --pagesize $pagesize --kernel_offset $kerneloff --ramdisk_offset $ramdiskoff $secondoff --tags_offset $tagsoff $dtb --output /tmp/anykernel/boot-new.img;
  if [ $? != 0 ]; then
    ui_print " "; ui_print "--- Repacking image failed. Aborting..."; exit 1;
  elif [ `wc -c < /tmp/anykernel/boot-new.img` -gt `wc -c < /tmp/anykernel/boot.img` ]; then
    ui_print " "; ui_print "--- New image larger than boot partition. Aborting..."; exit 1;
  fi;
  ui_print "--- .img file successfully repacked"
  if [ -f "/data/custom_boot_image_patch.sh" ]; then
    ash /data/custom_boot_image_patch.sh /tmp/anykernel/boot-new.img;
    if [ $? != 0 ]; then
      ui_print " "; ui_print "--- User script execution failed. Aborting..."; exit 1;
    fi;
  fi;
  if [ $selinuxfixvalue = 1 ]; then
    ui_print " "
    ui_print "--- Fixing Selinux warning"
    echo -n "SEANDROIDENFORCE" >> "/tmp/anykernel/boot-new.img"
    ui_print "--- Done"
  fi
  ui_print " "
  ui_print "--- Checking .img flashing property"
  if [ $flashimagevalue = 1 ]; then
    ui_print "----- Flashing .img file"
    if [ $imgtypevalue = 1 ]; then
      dd if=/tmp/anykernel/boot-new.img of=$blockrecovery;
	  ui_print "----- Flashing .img file to $blockrecovery"
    else
      dd if=/tmp/anykernel/boot-new.img of=$block;
	  ui_print "----- Flashing .img file to $block"
    fi
  else
    ui_print "----- NOT Flashing .img file"
  fi
  ui_print "--- Done Checking"
}

# backup_file <file>
backup_file() { cp $1 $1~; }

# replace_string <file> <if search string> <original string> <replacement string>
replace_string() {
  if [ -z "$(grep "$2" $1)" ]; then
      sed -i "s;${3};${4};" $1;
  fi;
}

# replace_section <file> <begin search string> <end search string> <replacement string>
replace_section() {
  line=`grep -n "$2" $1 | cut -d: -f1`;
  sed -i "/${2}/,/${3}/d" $1;
  sed -i "${line}s;^;${4}\n;" $1;
}

# remove_section <file> <begin search string> <end search string>
remove_section() {
  sed -i "/${2}/,/${3}/d" $1;
}

# insert_line <file> <if search string> <before|after> <line match string> <inserted line>
insert_line() {
  if [ -z "$(grep "$2" $1)" ]; then
    case $3 in
      before) offset=0;;
      after) offset=1;;
    esac;
    line=$((`grep -n "$4" $1 | cut -d: -f1` + offset));
    sed -i "${line}s;^;${5}\n;" $1;
  fi;
}

# replace_line <file> <line replace string> <replacement line>
replace_line() {
  if [ ! -z "$(grep "$2" $1)" ]; then
    line=`grep -n "$2" $1 | cut -d: -f1`;
    sed -i "${line}s;.*;${3};" $1;
  fi;
}

# remove_line <file> <line match string>
remove_line() {
  if [ ! -z "$(grep "$2" $1)" ]; then
    line=`grep -n "$2" $1 | cut -d: -f1`;
    sed -i "${line}d" $1;
  fi;
}

# prepend_file <file> <if search string> <patch file>
prepend_file() {
  if [ -z "$(grep "$2" $1)" ]; then
    echo "$(cat $patch/$3 $1)" > $1;
  fi;
}

# insert_file <file> <if search string> <before|after> <line match string> <patch file>
insert_file() {
  if [ -z "$(grep "$2" $1)" ]; then
    case $3 in
      before) offset=0;;
      after) offset=1;;
    esac;
    line=$((`grep -n "$4" $1 | cut -d: -f1` + offset));
    sed -i "${line}s;^;\n;" $1;
    sed -i "$((line - 1))r $patch/$5" $1;
  fi;
}

# append_file <file> <if search string> <patch file>
append_file() {
  if [ -z "$(grep "$2" $1)" ]; then
    echo -ne "\n" >> $1;
    cat $patch/$3 >> $1;
    echo -ne "\n" >> $1;
  fi;
}

# replace_file <file> <permissions> <patch file>
replace_file() {
  cp -pf $patch/$3 $1;
  chmod $2 $1;
}

# patch_fstab <fstab file> <mount match name> <fs match type> <block|mount|fstype|options|flags> <original string> <replacement string>
patch_fstab() {
  entry=$(grep "$2" $1 | grep "$3");
  if [ -z "$(echo "$entry" | grep "$6")" ]; then
    case $4 in
      block) part=$(echo "$entry" | awk '{ print $1 }');;
      mount) part=$(echo "$entry" | awk '{ print $2 }');;
      fstype) part=$(echo "$entry" | awk '{ print $3 }');;
      options) part=$(echo "$entry" | awk '{ print $4 }');;
      flags) part=$(echo "$entry" | awk '{ print $5 }');;
    esac;
    newentry=$(echo "$entry" | sed "s;${part};${6};");
    sed -i "s;${entry};${newentry};" $1;
  fi;
}

## end methods


## AnyKernel permissions
# set permissions for included files



## AnyKernel install
dump_boot;

# begin ramdisk changes


# end ramdisk changes

write_boot;

## end install

