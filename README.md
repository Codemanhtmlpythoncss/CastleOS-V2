docker start ubuntu
docker exec -it ubuntu /bin/bash


qemu-system-x86_64 -hda ./


add-symbol-file ./build/kernelfull.o 0x100000
target remote | qemu-system-x86_64 -hda ./bin/os.bin -S -gdb stdio
break kernel.c:549
c