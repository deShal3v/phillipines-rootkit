qemu-system-x86_64 \
        -m 2G \
        -smp 2 \
        -kernel $1/arch/x86/boot/bzImage \
        -append "console=ttyS0 root=/dev/sda earlyprintk=serial net.ifnames=0  nokaslr" \
        -drive file=$2/bullseye.img,format=raw \
        -net user,host=10.0.2.10,hostfwd=tcp:$3:10021-:22,hostfwd=tcp::5021-:21,hostfwd=tcp::5020-:20,hostfwd=tcp::5001-:5001 \
        -net nic,model=e1000 \
        -nographic \
        -pidfile vm.pid \
	-s
        2>&1 | tee vm.log
