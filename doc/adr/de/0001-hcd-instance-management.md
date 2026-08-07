# ADR-0001: HCD Instance Management

- **Status:** Accepted
- **Date:** 2026-08-06

---

# Kontext

VirtUSB unterstützt mehrere voneinander unabhängige virtuelle USB-Hostcontroller
(`VirtUsbHcd`) innerhalb eines Kernelmoduls.

Bereits in der High-Level Architecture wurde festgelegt, dass mehrere
`VirtUsbHcd` gleichzeitig existieren können und jeweils den Ausgangspunkt einer
eigenen VirtUSB-Topologie bilden.

Für die Implementierung muss festgelegt werden,

- wie viele HCD-Instanzen erzeugt werden,
- wie deren Lebenszyklus verwaltet wird,
- wie die Instanzen gespeichert werden,
- sowie wie Fehler während der Initialisierung behandelt werden.

Diese Entscheidung beeinflusst den gesamten Lebenszyklus des Kernelmoduls und
ist nachträglich nur mit größerem Refactoring änderbar.

---

# Entscheidung

Die Anzahl der zu erzeugenden `VirtUsbHcd`-Instanzen wird beim Laden des
Kernelmoduls über einen Modulparameter festgelegt.

Der Modulparameter besitzt:

- Standardwert: `1`
- zulässiger Wertebereich: `1..31`

Die Obergrenze von 31 stellt eine projektspezifische Schutzgrenze dar und
entspricht keiner Einschränkung der USB-Spezifikation.

Der Modulparameter ist nach dem Laden des Kernelmoduls nicht veränderbar.

Der modulweite Zustand, dessen Größe von der Anzahl der HCD-Instanzen abhängt,
wird dynamisch erzeugt.

Für jede HCD-Instanz wird ein eigenes Linux-Trägerobjekt erzeugt.

Jede HCD-Instanz besitzt einen eigenen privaten Zustand.

Die Initialisierung erfolgt nach dem Alles-oder-nichts-Prinzip.

Kann eine HCD-Instanz nicht erfolgreich erzeugt werden, werden alle zuvor
erzeugten Instanzen in umgekehrter Reihenfolge wieder entfernt und die
Initialisierung des Kernelmoduls schlägt vollständig fehl.

Beim Entladen des Kernelmoduls werden sämtliche HCD-Instanzen kontrolliert
entfernt.

---

# Betrachtete Alternativen

## Einzelne globale HCD-Instanz

Es existiert genau eine globale `VirtUsbHcd`-Instanz.

### Bewertung

Verworfen.

Diese Lösung widerspricht der festgelegten Architektur, nach der mehrere
unabhängige VirtUSB-Topologien gleichzeitig existieren können.

---

## Statisches Array mit maximal 31 Instanzen

Ein statisches Array verwaltet alle möglichen HCD-Instanzen.

### Bewertung

Technisch möglich.

Die Lösung ist einfach zu implementieren, reserviert jedoch unabhängig von der
tatsächlich verwendeten Instanzzahl Speicher und bildet die tatsächlich
erzeugten Instanzen weniger präzise ab.

---

## Dynamische Verwaltung entsprechend der angeforderten Instanzzahl

Der modulweite Zustand wird entsprechend der tatsächlich angeforderten Anzahl
von HCD-Instanzen dynamisch erzeugt.

### Bewertung

Gewählt.

Die Lösung reserviert nur den tatsächlich benötigten Speicher, bildet die
angeforderte Konfiguration unmittelbar ab und vereinfacht die Fehlerbehandlung
sowie den Lebenszyklus der HCD-Instanzen.

---

## Dynamisches Erzeugen und Entfernen während der Laufzeit

HCD-Instanzen können nach dem Laden des Kernelmoduls beliebig erzeugt oder
entfernt werden.

### Bewertung

Verworfen.

Für VirtUSB besteht derzeit kein fachlicher Bedarf für eine dynamische
Konfigurationsänderung während der Laufzeit.

Eine solche Lösung würde den Lebenszyklus, die Synchronisation sowie die
Schnittstellen unnötig verkomplizieren.

---

# Konsequenzen

## Vorteile

- Unterstützung mehrerer unabhängiger VirtUSB-Topologien.
- Geringer Speicherverbrauch durch dynamische Verwaltung.
- Klarer Lebenszyklus jeder HCD-Instanz.
- Vollständig deterministische Initialisierung.
- Eindeutiges Fehlerverhalten.
- Gute Erweiterbarkeit für zukünftige Implementierungsschritte.

## Nachteile

- Komplexere Initialisierung gegenüber einer Einzelinstanz.
- Rollback-Logik bei Fehlern erforderlich.

---

# Umsetzung

Diese Entscheidung wird insbesondere in folgenden Komponenten umgesetzt:

- `virtusb_module.c`
- `virtusb_hcd.c`

Sie bildet die Grundlage für die Verwaltung sämtlicher `VirtUsbHcd`-Instanzen.
