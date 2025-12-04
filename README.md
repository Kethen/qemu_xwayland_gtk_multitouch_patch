### qemu_xwayland_gtk_multitouch_patch

Alternative for https://lists.nongnu.org/archive/html/qemu-devel/2024-07/msg05065.html

XWayland populates XIDeviceEvent.detail with an incrementing number when adapting multitouch events from Wayland, which gets rejected by https://github.com/qemu/qemu/blob/1053bb627cf564e8b81ad0ef0dcc0fad9ff76de5/ui/gtk.c#L1185 -> https://github.com/qemu/qemu/blob/1053bb627cf564e8b81ad0ef0dcc0fad9ff76de5/ui/console.c#L613 as GTK uses that number for sequence/finger id

This allows GTK ui multitouch to work in XWayland by remapping sequence numbers, personally this is for getting multitouch working under Steam gamescope session XWayland

#### Usage

```
WAYLAND_DISPLAY="" LD_PRELOAD="$(pwd)/qemu_xwayland_gtk_multitouch_patch.so" qemu-system-x86_64 -display gtk -device virtio-multitouch-pci ...
```
