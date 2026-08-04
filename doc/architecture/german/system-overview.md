# Systemübersicht

VirtUSB ist ein Projekt zur Bereitstellung virtueller USB-Hostcontroller
(`VirtUsbHcd`), an die virtuelle USB-Geräte (`VirtUsbDev`) über einen
virtuellen USB-Root-Hub (`VirtUsbRHub`) angeschlossen werden können.

**VirtUSB virtualisiert USB-Devices, um deren Software unabhängig von der
Zielhardware entwickeln, testen und debuggen zu können.**

VirtUSB emuliert keine physische USB-Hardware. Insbesondere werden weder die
elektrische Signalübertragung noch die Bit- oder Paketebene eines USB-Busses
emuliert. Die Kommunikation erfolgt auf einer höheren USB-Abstraktionsebene.

Jede **Kernkomponente** stellt eine **definierte Schnittstelle** zur Steuerung
ihrer Funktionen und Zustände bereit.

Die Kernkomponenten bilden gemeinsam eine vollständig virtuelle
USB-Infrastruktur. An einen `VirtUsbHcd` können ausschließlich virtuelle
USB-Geräte angeschlossen werden. Eine Vermischung virtueller und physischer
USB-Komponenten innerhalb derselben VirtUSB-Topologie ist nicht vorgesehen.

Die Bedeutung der verwendeten Begriffe ist im Glossar definiert.