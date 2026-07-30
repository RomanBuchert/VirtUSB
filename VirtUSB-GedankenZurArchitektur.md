Folgende Aufgabenstellung:
# Präambel
Der Projektname ist VirtUSB.

Das Mutterrepository liegt unter https://github.com/RomanBuchert/VirtUSB

Die gemeinsame Sprache ist englisch. Da aber auch Nichtmuttersprachler am Projekt arbeiten, sollen Aussagen erstmal nicht perönlich wertend genommen werden,
sondern berücksichtig werden, dass eine etwas harsch klingende Aussage auch einfach durch mangelnde Sprachkenntisse zustande kam.
Nichtsdestotrotz sind persönliche Angriffe oder Beleidigungen nicht gedultet!

Hinweis, dass VirtUSB in keinerlei Verbindung zum historischen Windows Projekt "USB-VHCI" und dessen Komponente "virtusb" steht.

# Lizenz
Einerseits soll das Projekt so frei sein, dass Änderungen / Erweiterungen des bestehenden Codes zurückgegeben werden müssen, andererseites soll es aber auch ermöglichen, das Ganze in propritären Systemen einzusetzen.
So sollen z.B. weitere Backends durchaus closes-source sein dürfen. Zur Diskussion steht, ob API-Erweiterungen des Kerneltreibers und deren daraus resultierenden Backends open-source sein müssen.
Letztendlich soll damit verhindert werden, dass eine "Schatten-API" für closes-source Backends entsteht.

# Architektur

Es soll ein virtueller USB-Hostcontroller für Linux entwickelt werden. Damit soll eine USB-Device-Entwicklung ohne reale Hardware möglich werden.
Das Kernelmodul befindet sich außerhalb des Mainline-Kernel-Trees.
Der Hostcontroller soll alle Transferarten (Bulk, Interrupt, Isochronous, ...) unterstützen.
Dass Isochronous nicht Echtzeitfähig (125us-Frames) ist, ist bekannt und wird in Kauf genommen.
Die enummerierten Devices sollen sich wie echte Devices verhalnte und mittels lsusb sichtbar sein.
Es gibt kein bevorzugtes Device-Backend.
Die Steuerung des virtuellen Controllers erfolgt über /dev/virtusb<x>
Der Kerneltreiber unterstützt mehrere /dev/virtusb-Devices. Die Anzahl wird als Parameter beim Modulstart übergeben.
Jeder Root-Hub (/dev/virtusb<x>) hat 31 Ports.

# Organisation

Bevor Code erstellt wird, soll die Architektur geplant werden.
Das Projekt soll möglichst die Tools von GitLab Community-Edition zur Planung, CI/CD, ... nutzen.

Bevorzugtes Dateiformat zur Dokumentation ist Markdown mit Mermaid zur UML-Darstellung.

Jeder Milestone / größere Schritt muss in Git getaggt werden. Das Format ist v\<a\>.\<b\>.\<c\>.

Als Milesontes werden vorerst festgelegt:
- v0.0.\<n\>: Architekturplanung.
- v0.1.\<n\>: Implementierung bis zu Bulk- und Interrupttransfer.
- v0.2.\<n\>: Implementierung bis Isochronoustransfer.
- v0.3.\<n\>: Planung der User-API / libvirtusb
- v0.4.\<n\>: Implementierung der User-API

Es wird das ADR-Dokumentationsschema verwendet.

Hinweise zur Codedokumentation:
- Es wird Doxygen genutzt.
- Steht etwas im Header-File, wird es nur im Header-File dokumentiert.
- Im Sourcefile wird bei der Implementierung / Definition dokumentiert, nich bei der Deklaration.
- Einfache lokale Helferfunktionen müssen nicht mittels Doxygen dokumentiert werden.
- Für den Kerneltreiber gelten - abweichend hierzu - dessen Coding- und Dokumentationsstil.

Weitere Hinweise:
- KI ist als Werkzeug - vergleichbar mit einem Compiler - explizit erlaubt.
- Als Entwicklungssystem wird Debian und/oder Arch verwendet.
- Das Projekt nutzt CMake als Buildsystem.
- Zur statischen Codeanalyse wird cppcheck und clang-tidy genutzt. Es wird nur der Code des Projektes geprüft. Kein Kernel- oder fremder Bibliothekscode.
- Das Projekt soll sowohl mit GCC als auch mit LLVM/clang compilierbar sein.
- Das Kernelmodul muss DKMS-kompatibel sein und sich via DKMS installieren und entfernen lassen.
- Es muss Tools geben, um den Kernel zuverlässig via DKMS zu laden und entladen.
