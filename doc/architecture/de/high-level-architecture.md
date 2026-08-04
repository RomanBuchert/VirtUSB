# High-Level Architecture

## Zweck

- Fragestellung
  - Woraus besteht VirtUSB und wie hängen die Kernkomponenten zusammen?
- Konkretisiert die Systemübersicht.
- Beschreibt die grundlegende Architektur von VirtUSB.
- Beschreibt keine Implementierungsdetails.
- Bildet die Grundlage für nachgelagerte Architekturdokumente, welche einzelne Teilbereiche der High-Level Architecture weiter konkretisieren.

## Grundlegender Systemaufbau

- VirtUSB bildet eine vollständig virtuelle USB-Infrastruktur.
- Eine VirtUSB-Topologie besteht ausschließlich aus virtuellen Komponenten.
- Mehrere `VirtUsbHcd` können gleichzeitig existieren.
- Jeder `VirtUsbHcd` besitzt genau einen `VirtUsbRHub`.
- Jede VirtUSB-Topologie ist genau einem `VirtUsbHcd` zugeordnet.
- Die Existenz einer Kernkomponente ist unabhängig von ihrer Einbindung in eine VirtUSB-Topologie.
- Die Topologiezugehörigkeit einer Kernkomponente ist unabhängig von ihrem aktuellen Betriebszustand.

Diagramm:

- Übersicht der Kernkomponenten
- Topologie mit Root-Hub, Hubs und Devices

## Kernkomponenten

### VirtUsbHcd

- virtueller USB-Hostcontroller
- Wurzel einer VirtUSB-Topologie
- besitzt genau einen `VirtUsbRHub`
- stellt die Hostschnittstelle der Topologie dar

### VirtUsbRHub

- Root-Hub eines `VirtUsbHcd`
- Einstiegspunkt der virtuellen USB-Topologie
- stellt Downstream-Ports bereit
- Downstream-Ports sind Bestandteil des Root-Hubs
- funktional möglichst identisch zu `VirtUsbHub`

### VirtUsbHub

- funktional spezialisiertes `VirtUsbDev`
- stellt zusätzliche Downstream-Ports bereit
- Downstream-Ports sind Bestandteil des Hubs
- kann an höchstens einen Downstream-Port eines Root-Hubs oder Hubs angeschlossen werden
- kann entsprechend den Grenzen der USB-Spezifikation kaskadiert werden

### VirtUsbDev

- virtuelles USB-Gerät
- besitzt eigene Zustände
- besitzt definierte Schnittstellen
- kann an höchstens einem virtuellen USB-Port angeschlossen sein
- existiert unabhängig davon, ob es Bestandteil einer VirtUSB-Topologie ist

## Beziehungen der Kernkomponenten

### Beziehung `VirtUsbHcd` ↔ `VirtUsbRHub`

- Jeder `VirtUsbHcd` besitzt genau einen `VirtUsbRHub`.
- Jeder `VirtUsbRHub` gehört genau zu einem `VirtUsbHcd`.

### Beziehung Hub ↔ Downstream-Ports

- Jeder `VirtUsbRHub` besitzt mehrere Downstream-Ports.
- Jeder `VirtUsbHub` besitzt mehrere Downstream-Ports.
- Jeder Downstream-Port gehört genau zu einem Hub.

### Beziehung Downstream-Port ↔ `VirtUsbDev`

- An einem Downstream-Port kann höchstens ein `VirtUsbDev` angeschlossen sein.
- Ein `VirtUsbDev` kann an höchstens einem Downstream-Port angeschlossen sein.

### Beziehung `VirtUsbHub` ↔ Topologie

- `VirtUsbHub` ist funktional ein spezialisiertes `VirtUsbDev`.
- Ein `VirtUsbHub` kann an höchstens einem Downstream-Port angeschlossen sein.
- Ein `VirtUsbRHub` ist direkt einem `VirtUsbHcd` zugeordnet und besitzt keinen Parent-Port.
- Das Abstecken eines `VirtUsbHub` verändert nicht die Parent-Child-Struktur des darunterliegenden Teilbaums.
- Der Teilbaum bleibt intern erhalten, gehört jedoch zu keiner VirtUSB-Topologie, solange kein Pfad zu einem `VirtUsbHcd` besteht.
- Der erhaltene Teilbaum kann später wieder Bestandteil einer VirtUSB-Topologie werden.

### Beziehung `VirtUsbDev` ↔ Topologie

- Ein `VirtUsbDev` existiert unabhängig von einer VirtUSB-Topologie.
- Durch das Anstecken wird eine Parent-Child-Beziehung zu einem Hub-Port hergestellt.
- Ein `VirtUsbDev` wird Bestandteil einer VirtUSB-Topologie, wenn über seine Parent-Child-Beziehungen ein Pfad zu genau einem `VirtUsbHcd` besteht.
- Ein `VirtUsbDev` gehört zu höchstens einer VirtUSB-Topologie.
- Das Abstecken eines `VirtUsbDev` löst dessen Parent-Child-Beziehung.
- Das Abstecken eines `VirtUsbDev` verändert dessen Existenz nicht.

### Kaskadierung

- VirtUSB führt keine zusätzliche künstliche Begrenzung der Hub-Kaskadierung ein.
- Es gelten die Grenzen der USB-Spezifikation und der jeweiligen Implementierung.
- Zustandsänderungen eines Hubs können sich rekursiv auf den gesamten darunterliegenden Teilbaum auswirken.
- Rekursive Zustandsänderungen verändern nicht die Struktur des Teilbaums.

Diagramm:

- Komponentenbeziehungen
- Kardinalitäten

## Architekturebenen

- Architekturebenen beschreiben unterschiedliche fachliche Betrachtungsweisen derselben Kernkomponenten.
- Sie beschreiben Verantwortlichkeiten und stellen keine Implementierungs- oder Softwareschichten dar.

Architektursicht:

- Jede Kernkomponente kann hinsichtlich der für sie relevanten Architekturebenen beschrieben werden.

Fachliche Sicht:

- Eine Kernkomponente besitzt Zustände, Eigenschaften und Funktionen auf den für sie relevanten Architekturebenen.
- Welche Architekturebenen für eine Kernkomponente relevant sind, ergibt sich aus ihrer Funktion innerhalb von VirtUSB.
- Die Architekturebenen ermöglichen die getrennte Betrachtung von Existenz, Topologiezugehörigkeit, USB-Device-Controller und USB-Protokoll.
- Zustandsänderungen auf einer Architekturebene können Auswirkungen auf andere Architekturebenen besitzen, führen jedoch nicht zwangsläufig zu automatischen Zustandsänderungen.

### Geräteebene

- Existenz
- gerätespezifische Zustände
- Ein-/Ausschalten
- Reset
- interner Gerätezustand

### USB-Topologieebene

- Aufbau der virtuellen USB-Topologie
- Parent-Child-Beziehungen
- Anstecken
- Abstecken
- Portzuordnung

### USB-Device-Controller-Ebene

- USB-Device-Controller
- USB Connect
- USB Disconnect
- Endpoint-Aktivierung

### USB-Protokollebene

- Enumeration
- USB-Requests
- USB-Transfers
- Suspend / Resume

Diagramm:

- Beziehung der Architekturebenen
- Trennung von Existenz, Topologie, USB-Device-Controller und USB-Protokoll

## Systemoperationen

- Einschalten
- Ausschalten
- Anstecken
- Abstecken
- USB Connect
- USB Disconnect
- Enumeration

- fachliche Abhängigkeiten
- keine automatische Verkettung
- das Anstecken stellt eine Parent-Child-Beziehung her
- das Abstecken löst eine Parent-Child-Beziehung
- Zustandsänderungen können sich rekursiv entlang bestehender Parent-Child-Beziehungen auswirken
- rekursive Zustandsänderungen verändern nicht die Struktur des betroffenen Teilbaums

Diagramm:

- zeitliche bzw. logische Beziehungen
- rekursive Zustandsfortpflanzung

## Schnittstellen- und Kommunikationsmodell

- jede Kernkomponente besitzt eine definierte Schnittstelle
- Steuerung, Beobachtung und Konfiguration erfolgen ausschließlich über diese Schnittstelle
- Zustandsänderungen können über die definierte Schnittstelle gemeldet werden
- Die technische Umsetzung der Schnittstelle wird nicht festgelegt.
- Kernkomponenten auf derselben hierarchischen Ebene interagieren nicht direkt miteinander.
- Direkte Interaktion zwischen bereits verbundenen Kernkomponenten erfolgt ausschließlich entlang bestehender Parent-Child-Beziehungen.
- Das Anstecken stellt eine Parent-Child-Beziehung her.
- Das Abstecken löst eine Parent-Child-Beziehung.
- Die Kommunikation wird in zwei voneinander getrennte Kommunikationswege unterteilt:
  - Handling
  - USB-Kommunikation

### Handling

- Handling umfasst Vorgänge außerhalb der USB-Kommunikation.
- Handling dient insbesondere zur Steuerung und Weitergabe von:
  - Ein- und Ausschalten
  - Anstecken und Abstecken
  - Device Power
  - Port Power
  - Verfügbarkeit des USB-Device-Controllers
  - USB Connect und USB Disconnect
  - relevanten Zustandsänderungen
- Handling zwischen bereits verbundenen Kernkomponenten erfolgt entlang ihrer Parent-Child-Beziehung.
- Zustandsänderungen können rekursiv entlang eines Topologieteilbaums weitergegeben werden.
- Die Weitergabe erfolgt in kausaler Reihenfolge entsprechend den zugrunde liegenden Abhängigkeiten.
- Das Verhalten orientiert sich am Verhalten entsprechender realer USB-Hardware.

### USB-Kommunikation

- USB-Kommunikation umfasst sämtliche durch die USB-Spezifikation definierten Kommunikationsabläufe.
- Hierzu gehören insbesondere:
  - Enumeration
  - USB-Requests
  - USB-Transfers
  - USB Reset
  - USB Suspend
  - USB Resume
- USB-Kommunikation erfolgt ausschließlich über die USB-Protokollebene.
- Das Verhalten orientiert sich am Verhalten entsprechender realer USB-Hardware.

### Verhältnis beider Kommunikationswege

- Handling und USB-Kommunikation sind voneinander getrennte Kommunikationswege.
- Eine Zustandsänderung kann Auswirkungen auf beide Kommunikationswege besitzen.
- USB Connect und USB Disconnect werden über den Handling-Weg zwischen einem `VirtUsbDev` und seinem Parent weitergegeben.
- Die daraus entstehende hostsichtbare Connection beziehungsweise Disconnection ist Voraussetzung oder Abbruchbedingung der USB-Kommunikation.
- Handling ersetzt keine USB-Kommunikation.
- USB-Kommunikation ersetzt kein Handling.

## USB-Abstraktion

- keine Emulation physischer USB-Hardware
- keine elektrische Signalebene
- keine Bit-Ebene
- keine Paketebene
- Kommunikation auf höherer USB-Abstraktionsebene

## Grundlegende Architekturregeln

- mehrere `VirtUsbHcd` möglich
- genau ein `VirtUsbRHub` je `VirtUsbHcd`
- `VirtUsbHub` ist funktional ein spezialisiertes `VirtUsbDev`
- `VirtUsbRHub` und `VirtUsbHub` verwenden nach Möglichkeit dasselbe funktionale Hub-Modell
- VirtUSB führt keine zusätzliche künstliche Begrenzung der Hub-Kaskadierung ein
- für die Hub-Kaskadierung gelten die Grenzen der USB-Spezifikation und der jeweiligen Implementierung
- ein Device kann nur an einem virtuellen USB-Port angeschlossen sein
- ein `VirtUsbHub` kann an höchstens einem Downstream-Port angeschlossen sein
- ein `VirtUsbRHub` besitzt keinen Parent-Port
- die Existenz einer Kernkomponente ist unabhängig von ihrer Einbindung in eine VirtUSB-Topologie
- die Parent-Child-Struktur ist von der Zugehörigkeit zu einer VirtUSB-Topologie zu unterscheiden
- die Topologiezugehörigkeit einer Kernkomponente ist unabhängig von ihrem Betriebszustand
- das Abstecken einer Kernkomponente verändert deren Existenz nicht
- das Abstecken eines Hubs verändert nicht die interne Parent-Child-Struktur seines darunterliegenden Teilbaums
- ein Teilbaum gehört nur dann zu einer VirtUSB-Topologie, wenn ein Pfad zu genau einem `VirtUsbHcd` besteht
- Zustandsänderungen können sich rekursiv entlang bestehender Parent-Child-Beziehungen auswirken
- Device Power und Port Power sind unterschiedliche Zustände
- Anstecken ist nicht gleich USB Connect
- USB Connect ist nicht gleich Enumeration
- Handling und USB-Kommunikation sind getrennte Kommunikationswege
- USB Connect und USB Disconnect werden über den Handling-Weg weitergegeben
- das Verhalten beider Kommunikationswege orientiert sich am Verhalten entsprechender realer USB-Hardware
- jede Kernkomponente wird ausschließlich über ihre definierte Schnittstelle gesteuert
