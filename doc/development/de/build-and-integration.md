# Build and Integration

## Zweck

**Fragestellung:**

> Wie werden die Komponenten von VirtUSB gebaut, geprüft und in das Zielsystem integriert?

Dieses Dokument beschreibt die projektweiten Vorgaben für den Buildprozess,
die Integration und die Entwicklungswerkzeuge.

Es beschreibt keine Architektur einzelner Komponenten und keine
Implementierungsdetails.

## Buildsystem

VirtUSB verwendet CMake als projektweiten Einstiegspunkt für den Buildprozess.

CMake dient insbesondere zur:

- Konfiguration des Projekts,
- Auswahl von Buildvarianten,
- Integration von Tests,
- Integration statischer Codeanalysen,
- Erzeugung zusätzlicher Projektartefakte,
- Integration in CI-Systeme.

Für Kernelmodule ersetzt CMake nicht das Linux-Kernel-Buildsystem, sondern
koordiniert dessen Verwendung.

## Kernelintegration

Das VirtUSB-Kernelmodul wird außerhalb des Linux-Kernel-Quellbaums entwickelt.

Die Implementierung soll mit den Buildmechanismen des Linux-Kernels kompatibel
bleiben.

Die Projektstruktur soll so gewählt werden, dass eine spätere Integration über
DKMS ohne grundlegende Umstrukturierung möglich ist.

## Compiler

VirtUSB soll, soweit technisch sinnvoll und von der verwendeten
Kernelkonfiguration unterstützt, mindestens mit folgenden Compilern gebaut
werden können:

- GCC
- Clang

## Qualitätssicherung

Der Buildprozess soll die Integration projektweiter
Qualitätssicherungsmaßnahmen ermöglichen.

Hierzu gehören insbesondere:

- Unit-Tests
- statische Codeanalyse
- Formatprüfung
- Continuous Integration

Die Auswahl der eingesetzten Werkzeuge wird nicht durch dieses Dokument
festgelegt.

## Projektstruktur

Die Buildstruktur soll sowohl Kernel- als auch Userspace-Komponenten
unterstützen.

Sie soll insbesondere die gemeinsame Entwicklung von:

- Kernelmodulen,
- Bibliotheken,
- Testprogrammen,
- Beispielen und
- Hilfswerkzeugen

ermöglichen.

## Architekturregeln

- CMake ist der projektweite Einstiegspunkt für den Buildprozess.
- Kernelmodule bleiben mit dem Linux-Kernel-Buildsystem kompatibel.
- Die Projektstruktur unterstützt eine spätere DKMS-Integration.
- Build- und Integrationslogik bleiben von der Laufzeitarchitektur getrennt.
- Die Wahl konkreter Entwicklungswerkzeuge wird nicht durch dieses Dokument
  festgelegt.