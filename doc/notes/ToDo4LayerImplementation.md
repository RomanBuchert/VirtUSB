```text
Ja. Nach Sichtung des Repositories sowie der beigefügten Notizen würde ich sechs bestehende Dokumente zwingend anpassen. Zusätzlich halte ich einen neuen ADR für sinnvoll.

Die drei Verantwortungsbereiche sollten dabei nicht als streng technische Protokollschichten verstanden werden, sondern als getrennte Zustands- und Verantwortungsbereiche:

Virtual Device Hardware
Virtual Host Controller and Topology
USB Protocol Operation

Diese Trennung ist in den Diskussionsnotizen bereits klar beschrieben. Besonders wichtig ist dabei: Eine Backend-Instanz darf nur die virtuelle Hardware ihres eigenen Geräts beeinflussen; VirtUSB verwaltet Controller und Topologie; der Linux-USB-Stack verarbeitet das daraus resultierende USB-Verhalten.

Zwingend zu ändernde Dateien
1. doc/glossary.md

Stärkste Priorität, weil alle anderen Dokumente darauf verweisen.

Zu ergänzen beziehungsweise zu überarbeiten sind mindestens:

Virtual Device Hardware
Device Hardware State
Device Power State
USB Hardware Availability
Virtual Host Controller and Topology
USB Protocol Operation
USB Connection State
Topology
Parent Hub
Parent Port
Device Association
Device Attachment
Device Detachment

Bestehende Definitionen müssen geschärft werden:

Association
Attachment
Connection
Backend
Backend Instance
Virtual USB Device
Port
Downstream Port

Derzeit sind insbesondere Attachment, Connection und Device Visibility zu eng miteinander gekoppelt. Nach dem neuen Modell müssen diese Sachverhalte getrennt sein:

Hardware existiert und läuft.
Gerät ist einem Port topologisch zugeordnet.
Gerät signalisiert eine USB-Verbindung.
Gerät befindet sich in einem USB-Protokollzustand.
2. doc/architecture/high-level-architecture.md

Hier muss das Modell verbindlich eingeführt werden. Das ist das zentrale Architekturdokument.

Betroffene Bereiche:

Architectural Principles
Drei Verantwortungsbereiche erläutern.
Trennung unabhängiger Zustandsräume festlegen.
System Decomposition
Diagramm um Virtual Device Hardware, Host-Controller-/Topologieebene und USB Protocol Operation erweitern.
Controller and Root Hub Model
Topologie nicht mehr ausschließlich als 31 Root-Hub-Ports darstellen.
Parent-Hub-/Parent-Port-Modell berücksichtigen.
Port and Device Model
Association, Attachment, Connection und USB Visibility trennen.
Kabelzustand und USB-seitige Präsenz nicht gleichsetzen.
Backend Model
Backend als Softwarekomponente von der repräsentierten virtuellen Hardware unterscheiden.
Autorität des Backends auf das eigene Gerät begrenzen.
Communication Model
Hardwarezustandsänderungen, Topologieoperationen und USB-Transfers als unterschiedliche Kommunikationskategorien behandeln.
Runtime Model
getrennte Lebenszyklen beziehungsweise Zustandsbereiche darstellen.
Failure and Recovery Model
Backend-Ausfall von bewusstem Device Power-off oder USB-Hardware-disable unterscheiden.

Die vorhandenen Mermaid-Diagramme in den Kapiteln 4, 6, 7 und 11 müssen dabei vermutlich angepasst werden.

Zusätzlich muss das bisher teilweise flache Root-Hub-Modell auf eine allgemeine USB-Topologie erweitert werden. Die bereits festgehaltene Architekturidee lautet: VirtUSB modelliert einen USB-Baum; ein Hub ist ein normales USB-Gerät mit zusätzlichen Downstream-Ports.

3. doc/architecture/system-overview.md

Dieses Dokument muss die neue Architektur verständlich zusammenfassen, ohne deren Details vollständig zu wiederholen.

Betroffene Kapitel:

System Context
Principal Components
System Boundary
Typical Operational Flow
Deployment View
Interfaces

Insbesondere das typische Ablaufmodell muss künftig unterscheiden zwischen:

Device hardware powered on
→ USB hardware becomes available
→ Host controller reports connection
→ Linux enumerates device

und:

Device detached from parent port
→ topology association is removed
→ host-visible connection disappears

Das bestehende Sequenzdiagramm behandelt Attach derzeit praktisch als unmittelbaren USB-Connect. Diese Gleichsetzung muss entfallen.

4. doc/requirements/system-requirements.md

Hier sollte das Drei-Bereiche-Modell nicht im Detail beschrieben werden. Die dafür notwendigen, extern beobachtbaren Systemfähigkeiten müssen aber gefordert werden.

Zu ergänzen beziehungsweise zu prüfen:

Device-Hardwarezustand kann unabhängig von Portzuordnung geändert werden.
Ein zugeordnetes Gerät kann USB-seitig erscheinen und verschwinden, ohne neu erzeugt oder einem anderen Port zugewiesen zu werden.
Wiederholtes Disconnect/Reconnect muss möglich sein.
Virtuelles Ein-/Ausstecken muss sich hostseitig wie reale Hardware verhalten.
VirtUSB muss hierarchische USB-Topologien unterstützen.
Ein Gerät darf zu genau einem Parent-Port gehören.
Ein Hub darf zusätzliche Ports bereitstellen, ohne als architektonischer Sonderfall behandelt zu werden.

Betroffene Kapitel:

Scope
System Overview
Virtual USB Devices
Device Enumeration
Userspace Interaction
Interface Requirements
Error Handling

Die System Requirements sollen weiterhin nur das Was festlegen, nicht Zustandsautomaten oder Nachrichtenformate.

5. doc/requirements/software-requirements.md

Hier müssen die Verantwortlichkeiten eindeutig den Softwarekomponenten zugeordnet werden.

Kernelmodul

Verantwortlich für:

virtuelle Host Controller
Root Hub
allgemeine USB-Topologie
Parent-Port-Zuordnung
Port- und Connection-State gegenüber Linux
Routing von USB-Transfers
Umsetzung hostseitig beobachtbarer Zustandsänderungen
Userspace-Komponenten

Verantwortlich für:

Control Plane
Koordination von Backend und Controller
Topologieoperationen
Weitergabe oder Verarbeitung von Hardwarezustandsänderungen
Kommunikation zwischen Backend und Kernelmodul
Backend

Verantwortlich für:

eigene virtuelle Device-Hardware
gerätespezifisches USB-Verhalten
Meldung oder Anforderung relevanter eigener Zustandsänderungen
keine Steuerung fremder Geräte oder Topologieobjekte

Betroffene Kapitel:

Software Overview
Kernel Module Requirements
Userspace Requirements
Backend Integration Requirements
Internal Interface Requirements

Die bisherige Formulierung

representation, attachment, disconnection, and removal of virtual USB devices

ist zu unscharf und sollte getrennt werden in:

Device-Hardware-Lifecycle
Topologiezuordnung
USB Connection State
USB-Protokollbetrieb
6. doc/requirements/backend-requirements.md

Dieses Dokument ist am stärksten von der inhaltlichen Präzisierung betroffen.

Die Notizen nennen ausdrücklich die kritischen Punkte:

Backend lifecycle ≠ device hardware lifecycle
Device creation/removal ≠ power on/off
Attach/detach ≠ USB presence
Backend darf nur die eigene virtuelle Hardware beeinflussen
resultierende Host-Controller-Änderungen laufen über die dokumentierte Schnittstelle
USB-Transfers bleiben davon getrennt.

Betroffene Kapitel:

Kapitel 5 – Backend Overview
Backend als Softwarekomponente klar von Virtual Device Hardware trennen.
Aussage „one or more virtual USB devices“ auf genau eine Device-Instanz pro Backend-Instanz korrigieren, sofern noch vorhanden.
Kapitel 6 – General Backend Requirements
Autoritätsgrenze ergänzen.
Backend darf ausschließlich sein eigenes Gerät beeinflussen.
Kapitel 7 – Backend Lifecycle Requirements
Backend-Lifecycle von Device-Hardware-Lifecycle trennen.
Registrierung, Initialisierung und Shutdown sind nicht dasselbe wie Device Power-on/off.
Kapitel 8 – Virtual Device Requirements

Hier sollte wahrscheinlich eine explizite Unterteilung entstehen:

Device instance lifecycle
Hardware power/operational state
USB hardware availability
Topology association
USB connection state
USB protocol state
Kapitel 9 – USB Transfer Requirements
Klarstellen, dass USB-Transfers ausschließlich zur USB Protocol Operation gehören.
Verhalten bei Hardware-off, USB-disable oder Detach definieren, zumindest auf Requirement-Ebene.
Kapitel 10 – Backend Interface Requirements
Interface muss Kommunikation in beide Richtungen ermöglichen:
Requests vom Host zum Backend
Completions vom Backend zum Host
Zustandsänderungen der eigenen virtuellen Hardware vom Backend zur Control Plane

Dabei darf noch keine konkrete API definiert werden.

Neues Dokument
doc/adr/ADR-xxxx-virtual-device-control-model.md

Ein ADR ist technisch empfohlen.

Möglicher Titel:

Separate Virtual Device Hardware, Host Controller Topology, and USB Protocol Operation

Der ADR sollte festhalten:

warum die drei Bereiche getrennt werden,
welche Alternativen betrachtet wurden,
dass sie keine starren Implementierungsschichten darstellen,
dass ihre Zustände unabhängig sein können,
dass kausale Wirkungen zwischen ihnen bestehen,
dass ein Backend nur seine eigene virtuelle Hardware steuern darf,
dass Topologieverwaltung Aufgabe der VirtUSB-Control-Plane ist,
dass nach außen ausschließlich USB-konformes Verhalten sichtbar wird.

Ein zweiter ADR zur USB Topology Model-Entscheidung wäre ebenfalls sinnvoll, kann aber möglicherweise mit diesem ADR kombiniert werden. Die Notizen identifizieren ausdrücklich die Grundsätze „USB-Topologie statt Root-Port-Modell“ und „Host-Controller-Semantik statt Backend-Implementierungsdetails“ als ADR-Kandidaten.

Nicht zwingend zu ändern
README.md

Eine Anpassung ist erst sinnvoll, wenn das Architekturmodell in den offiziellen Dokumenten konsolidiert ist. Danach könnte die Architekturübersicht knapp erwähnen:

hierarchische USB-Topologien,
Backend-Instanz repräsentiert ein Gerät,
getrennte Hardware-, Controller-/Topologie- und USB-Protokollverantwortung.

Aktuell ist das nicht zwingend.

doc/document-template.md

Keine Änderung wegen des Layer-Modells erforderlich.

doc/code-documentation-examples.md

Nicht betroffen.

CODE_OF_CONDUCT.md, CONTRIBUTING.md, doc/legal/*

Nicht betroffen.

doc/notes/*

Die Notizen sollen gemäß Sketchbook-Regel nur ergänzt und nicht als normative Dokumente refaktoriert werden.

Ergebnis

Damit ergibt sich folgende Arbeitsliste:

Priorität	Datei	Änderung
1	doc/glossary.md	Begriffs- und Zustandsmodell
2	doc/architecture/high-level-architecture.md	verbindliche Architektur
3	doc/architecture/system-overview.md	verständliche Gesamtübersicht
4	doc/requirements/system-requirements.md	extern beobachtbare Fähigkeiten
5	doc/requirements/software-requirements.md	Komponentenverantwortlichkeiten
6	doc/requirements/backend-requirements.md	Backend-Autorität und getrennte Lebenszyklen
7	neuer ADR	Entscheidung und Begründung dokumentieren

Ich würde mit dem Glossar beginnen und danach die High-Level Architecture überarbeiten. Erst wenn diese beiden Dokumente akzeptiert sind, sollten die drei Requirements-Dokumente angepasst werden.
```