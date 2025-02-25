.. _acrn_on_qemu:

Enable ACRN Over QEMU/KVM
#########################

This document shows how to bring up ACRN as a nested hypervisor on top of
QEMU/KVM with basic functionality such as running a Service VM and User VM.
Running ACRN as a nested hypervisor gives you an easy way to evaluate ACRN in an
emulated environment instead of setting up a separate hardware platform
configuration.

This setup was tested with the following configuration:

- ACRN hypervisor: ``v2.6`` tag
- ACRN kernel: ``v2.6`` tag
- QEMU emulator version: 4.2.1
- Service VM/User VM OS: Ubuntu 20.04
- Platforms tested: Kaby Lake, Skylake

Prerequisites
*************

1. Make sure the platform supports Intel VMX as well as VT-d
   technologies. On Ubuntu 20.04, this
   can be checked by installing the ``cpu-checker`` tool. If the
   output displays **KVM acceleration can be used**,
   the platform supports it.

   .. code-block:: console

      kvm-ok
      INFO: /dev/kvm exists
      KVM acceleration can be used

2. The host kernel version must be **at least 5.3.0** or above.
   Ubuntu 20.04 uses a 5.8.0 kernel (or later),
   so no changes are needed if you are using it.

3. Make sure KVM and the following utilities are installed.

   .. code-block:: none

      sudo apt update && sudo apt upgrade -y
      sudo apt install qemu-kvm virtinst libvirt-daemon-system -y


Prepare Service VM (L1 Guest)
*****************************

1. Use the ``virt-install`` command to create the Service VM.

   .. code-block:: none

      virt-install \
      --connect qemu:///system \
      --name ServiceVM \
      --machine q35 \
      --ram 4096 \
      --disk path=/var/lib/libvirt/images/servicevm.img,size=32 \
      --vcpus 4 \
      --virt-type kvm \
      --os-type linux \
      --os-variant ubuntu18.04 \
      --graphics none \
      --clock offset=utc,tsc_present=yes,kvmclock_present=no \
      --qemu-commandline="-machine kernel-irqchip=split -cpu Denverton,+invtsc,+lm,+nx,+smep,+smap,+mtrr,+clflushopt,+vmx,+x2apic,+popcnt,-xsave,+sse,+rdrand,+vmx-apicv-xapic,+vmx-apicv-x2apic,+vmx-flexpriority,+tsc-deadline,+pdpe1gb -device intel-iommu,intremap=on,caching-mode=on,aw-bits=48" \
      --location 'http://archive.ubuntu.com/ubuntu/dists/bionic/main/installer-amd64/' \
      --extra-args "console=tty0 console=ttyS0,115200n8"

#. Walk through the installation steps as prompted. Here are a few things to note:

   a. Make sure to install an OpenSSH server so that once the installation is
      complete, you can SSH into the system.

      .. figure:: images/acrn_qemu_1.png
         :align: center

   b. We use GRUB to boot ACRN, so make sure you install it when prompted.

      .. figure:: images/acrn_qemu_2.png
         :align: center

   c. After the installation is complete, the Service VM (guest) will restart.

#. Log in to the Service VM guest. Find the IP address of the guest and use it
   to connect via SSH. The IP address can be retrieved using the ``virsh``
   command as shown below.

   .. code-block:: console

      virsh domifaddr ServiceVM
       Name       MAC address          Protocol     Address
      -------------------------------------------------------------------------------
       vnet0      52:54:00:72:4e:71    ipv4         192.168.122.31/24

#. Once logged into the Service VM, enable the serial console. Once ACRN is enabled,
   the ``virsh`` command will no longer show the IP.

   .. code-block:: none

      sudo systemctl enable serial-getty@ttyS0.service
      sudo systemctl start serial-getty@ttyS0.service

#. Enable the GRUB menu to choose between Ubuntu and the ACRN hypervisor.
   Modify :file:`/etc/default/grub` and edit these entries:

   .. code-block:: none

      GRUB_TIMEOUT_STYLE=menu
      GRUB_TIMEOUT=5
      GRUB_CMDLINE_LINUX_DEFAULT=""
      GRUB_GFXMODE=text

#. The Service VM guest can also be launched again later using
   ``virsh start ServiceVM --console``. Make sure to use the domain name you
   used while creating the VM in case it is different than ``ServiceVM``.

This concludes the initial configuration of the Service VM. The next steps will
install ACRN in it.

.. _install_acrn_hypervisor:

Install ACRN Hypervisor
***********************

1. Launch the ``ServiceVM`` Service VM guest and log into it (SSH is recommended
   but the console is available too).

   .. important:: All the steps below are performed **inside** the Service VM
      guest that we built in the previous section.

#. Install the ACRN build tools and dependencies following the :ref:`gsg`.

#. Switch to the ACRN hypervisor ``v2.6`` tag.

   .. code-block:: none

      cd ~
      git clone https://github.com/projectacrn/acrn-hypervisor.git
      cd acrn-hypervisor
      git checkout v2.6

#. Build ACRN for QEMU:

   .. code-block:: none

      make BOARD=qemu SCENARIO=sdc

   For more details, refer to the :ref:`gsg`.

#. Install the ACRN Device Model and tools:

   .. code-block::

      sudo make install

#. Copy ``acrn.32.out`` to the Service VM guest ``/boot`` directory.

   .. code-block:: none

      sudo cp build/hypervisor/acrn.32.out /boot

#. Clone and configure the Service VM kernel repository following the
   instructions in the :ref:`gsg` and using the ``v2.6`` tag. The User VM (L2
   guest) uses the ``virtio-blk`` driver to mount the rootfs. This driver is
   included in the default kernel configuration as of the ``v2.6`` tag.

#. Update GRUB to boot the ACRN hypervisor and load the Service VM kernel.
   Append the following configuration to the :file:`/etc/grub.d/40_custom`.

   .. code-block:: none

      menuentry 'ACRN hypervisor' --class ubuntu --class gnu-linux --class gnu --class os $menuentry_id_option 'gnulinux-simple-e23c76ae-b06d-4a6e-ad42-46b8eedfd7d3' {
         recordfail
         load_video
         gfxmode $linux_gfx_mode
         insmod gzio
         insmod part_msdos
         insmod ext2

         echo 'Loading ACRN hypervisor with SDC scenario ...'
         multiboot --quirk-modules-after-kernel /boot/acrn.32.out
         module /boot/bzImage Linux_bzImage
      }

#. Update GRUB:

   .. code-block:: none

      sudo update-grub

#. Enable networking for the User VMs:

   .. code-block:: none

      sudo systemctl enable systemd-networkd
      sudo systemctl start systemd-networkd

#. Shut down the guest and relaunch it using
   ``virsh start ServiceVM --console``.
   Select the ``ACRN hypervisor`` entry from the GRUB menu.

   .. note::
      You may occasionally run into the following error: ``Assertion failed in
      file arch/x86/vtd.c,line 256 : fatal error``. This is a transient issue;
      try to restart the VM when that happens. If you need a more stable setup,
      you can work around the problem by switching your native host to a
      non-graphical environment (``sudo systemctl set-default
      multi-user.target``).

#. Use ``dmesg`` to verify that you are now running ACRN.

   .. code-block:: console

      dmesg | grep ACRN
      [    0.000000] Hypervisor detected: ACRN
      [    2.337176] ACRNTrace: Initialized acrn trace module with 4 cpu
      [    2.368358] ACRN HVLog: Initialized hvlog module with 4 cpu
      [    2.727905] systemd[1]: Set hostname to <ServiceVM>.

   .. note::
      When shutting down the Service VM, make sure to cleanly destroy it with
      these commands, to prevent crashes in subsequent boots.

      .. code-block:: none

         virsh destroy ServiceVM # where ServiceVM is the virsh domain name.

Bring Up User VM (L2 Guest)
***************************

1. Build the User VM disk image (``UserVM.img``) following
   :ref:`build-the-ubuntu-kvm-image` and copy it to the Service VM (L1 guest).
   Alternatively you can use an
   `Ubuntu Desktop ISO image <https://ubuntu.com/#download>`_.
   Rename the downloaded ISO image to ``UserVM.iso``.

#. Transfer the ``UserVM.img``  or ``UserVM.iso`` User VM disk image to the
   Service VM (L1 guest).

#. Launch the User VM using the ``launch_ubuntu.sh`` script.

   .. code-block:: none

      cp ~/acrn-hypervisor/misc/config_tools/data/samples_launch_scripts/launch_ubuntu.sh ~/
      cp ~/acrn-hypervisor/devicemodel/bios/OVMF.fd ~/

#. Update the script to use your disk image (``UserVM.img`` or ``UserVM.iso``).

   .. code-block:: none

      acrn-dm -A -m $mem_size -s 0:0,hostbridge \
      -s 3,virtio-blk,~/UserVM.img \
      -s 4,virtio-net,tap0 \
      -s 5,virtio-console,@stdio:stdio_port \
      --ovmf ~/OVMF.fd \
      $logger_setting \
      $vm_name
