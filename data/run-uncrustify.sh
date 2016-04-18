#!/bin/bash

for DIR in ../src ../libnautilus-private ../nautilus-desktop ../test ../libnautilus-extension ../eel ../nautilus-sendto-extension
do
    # Aligning prototypes is not working yet, so avoid headers
    #uncrustify -c kr-gnome-indent.cfg --no-backup $(find $DIR -name "*.[ch]")
    uncrustify -c uncrustify.cfg --no-backup $(find $DIR -name "*.c")
done

