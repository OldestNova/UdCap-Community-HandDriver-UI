# GTK4 Ports
在使用 vcpkg 时，由于 GTK4 版本为 4.16.3，该版本有一个导致 Windows 上计算宽度时崩溃的 bug，通过 overlay port 打上这个补丁就可以正常使用了。

```bash
vcpkg install gtk --overlay-ports=overlay/gtk
```