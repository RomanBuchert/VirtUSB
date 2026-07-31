# Gedankennotizen

Dieses Dokument dient als persönliches Labor-/Werkstattbuch für das VirtUSB-Projekt.

Grundsätze:
- Notizen werden ausschließlich ergänzt.
- Inhalte werden grundsätzlich nicht gelöscht.
- Zur besseren Übersicht dürfen Inhalte umsortiert werden.
- Veraltete oder verworfene Ideen bleiben erhalten und werden ggf. als solche gekennzeichnet.
- Das Dokument ist bewusst informell und dient ausschließlich als persönliche Arbeitsgrundlage.

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
Jeder Root-Hub (/dev/virtusb\<x\>) hat 31 Ports.

# Organisation

Bevor Code erstellt wird, soll die Architektur geplant werden.
Das Projekt soll möglichst die Tools von GitHub zur Planung, CI/CD, ... nutzen.

Bevorzugtes Dateiformat zur Dokumentation ist Markdown mit Mermaid zur UML-Darstellung. Labels in Mermaid müssen immer in doppelte Anführungszeichen gesetzt werden, um Parserprobleme zu vermeiden.

## Tags
Um den Fortschritt einzelner Punkte anzuzeigen, sollen folgende Unicodesymbole verwendet werden:

|Symbol | Meaning |
|-------|-------- |
| ⬜ | Not started |
| ⏳ | In progress |
| ✅ | Completed |
| 🚫 | Blocked |
| ❌ | Abandoned |

Auf das Symbol ⬜ kann verzichtet werden.

# Milestones

Jeder Milestone / größere Schritt muss in Git getaggt werden. Das Format ist v\<a\>.\<b\>.\<c\>.

Als Milestones werden vorerst festgelegt:

## v0.0.x: Architecture design

- v0.0.1  High-Level Architecture ✅
- v0.0.2  System Requirements
- v0.0.3  Software Requirements
- v0.0.4  Backend Requirements
- v0.0.5  Kernel/User Interface Specification
- v0.0.6  Userspace Communication Protocol
- v0.0.7  Kernel/User Ownership & Memory Model
- v0.0.8  Transfer Queue & Scheduling Model
- v0.0.9  Synchronization & Concurrency Design
- v0.0.10 Architecture Review & Baseline Freeze

## v0.1.x: Core USB functionality

  - Core Kernel framework
  - Virtual Host Controller
  - Root Hub
  - Port management
  - Userspace communication
  - Control transfers
  - Bulk transfers
  - Interrupt transfers

## v0.2.x: Full USB functionality

  - Isochronous transfers
  - Robust error handling
  - Performance optimizations
  - Long-term stability

## v0.3.x: Public API specified

  - libvirtusb architecture
  - Public API specification
  - API documentation
  - Example applications
  - API stabilization

## v0.4.x: Public API implemented

  - libvirtusb implementation
  - Unit tests
  - Integration tests
  - Example applications
  - Documentation updates

## v0.5.x: Production readiness

  - Developer documentation completed
  - User documentation
  - CI/CD pipeline
  - Packaging
  - DKMS packaging
  - Performance tuning
  - Compatibility testing
  - Release preparation

## v1.0.0: First stable release

## Dokumentation

Es wird das ADR-Dokumentationsschema verwendet.

Hinweise zur Codedokumentation:
- Es wird Doxygen genutzt.
- Steht etwas im Header-File, wird es nur im Header-File dokumentiert.
- Im Sourcefile wird bei der Implementierung / Definition dokumentiert, nich bei der Deklaration.
- Einfache lokale Helferfunktionen müssen nicht mittels Doxygen dokumentiert werden.
- Für den Kerneltreiber gelten - abweichend hierzu - dessen Coding- und Dokumentationsstil.
- Bei Referenzen immer nur auf den Titel verweisen. Keine Links, keine externen Dateikopien, da diese schnell veralten / verwaisen.

- Präfix für Dokumente

	doc/requirements/
		system-requirements.md
		software-requirements.md
		backend-requirements.md

	Damit ist sofort klar, auf welcher Ebene sich die Anforderungen befinden.

- Requirement-Traceability
	Später könnte jede Anforderung eine kleine Tabelle erhalten:

    | Requirement | Verified by | ADR | Status |
    |-------------|-------------|-----|--------|
    | VUSB-FR-001 | - | - | Draft |
    | VUSB-FR-002 | - | - | Draft |
    | VUSB-FR-003 | - | - | Draft |

Weitere Hinweise:
- KI ist als Werkzeug - vergleichbar mit einem Compiler - explizit erlaubt.
- Als Entwicklungssystem wird Debian und/oder Arch verwendet.
- Das Projekt nutzt CMake als Buildsystem.
- Zur statischen Codeanalyse wird cppcheck und clang-tidy genutzt. Es wird nur der Code des Projektes geprüft. Kein Kernel- oder fremder Bibliothekscode.
- Das Projekt soll sowohl mit GCC als auch mit LLVM/clang compilierbar sein.
- Das Kernelmodul muss DKMS-kompatibel sein und sich via DKMS installieren und entfernen lassen.
- Es muss Tools geben, um den Kernel zuverlässig via DKMS zu laden und entladen.


# Erstellen von neuen Dateien

Anweisung an die KI:

Ich würde deshalb künftig bei allen neuen Dokumenten so beginnen:
```markdown
# Kapitel

- Punkt
- Punkt
- Punkt
- Punkt
```
Erst wenn wir mit der Liste zufrieden sind, schreiben wir den eigentlichen Fließtext.

Ich halte das sogar für effizienter. Wir diskutieren zunächst die Vollständigkeit ("Haben wir an alles gedacht?") und erst danach die Formulierung ("Wie schreiben wir das?"). Gerade bei Architektur- und Anforderungsdokumenten führt das meist zu besseren Ergebnissen, weil wir uns nicht zu früh auf konkrete Texte festlegen.

Prompt für die KI:

```text
When creating or extending documentation, follow these rules.

## Document format

- Use Markdown for all documentation.
- Mermaid diagrams are explicitly allowed where they improve clarity.
- Prefer simple, readable Markdown over complex formatting.

## Writing style

Use a style similar to a well-written software architecture specification.

The documentation should be:

- clear and precise
- technically accurate
- concise
- objective
- easy to review
- easy to maintain

Avoid:

- marketing language
- unnecessary repetition
- long paragraphs
- overly complex sentences
- implementation details unless explicitly required

Prefer:

- one idea per paragraph
- short paragraphs
- bullet lists for enumerations
- consistent terminology throughout the document
- explicit responsibility boundaries
- precise wording

The documentation should read like an engineering specification rather than a tutorial or marketing document.

## Working process

Documentation is developed incrementally.

### Step 1 – Create the document structure

Create the complete document structure first.

Each chapter shall initially contain only a list of bullet points representing the topics that will later be described.

These bullet points act as implementation stubs for the chapter.

### Step 2 – Review the stub

Before writing any prose, review the bullet list together with the user.

The goal is to ensure that the list is complete and that no important topic is missing.

Do not generate any paragraph text during this step.

### Step 3 – Write the chapter

After the bullet list has been accepted, replace the bullet list with the actual chapter text.

The resulting text should naturally cover every accepted bullet point.

### Step 4 – Continue

Once the chapter has been accepted, continue with the next chapter using exactly the same workflow.

Repeat this process until the complete document has been finished.
```

## Template

```markup
<!--
Document Template

Workflow
========

1. Copy this template and rename the file appropriately.
2. Replace placeholder chapter and appendix titles.
3. Add stub bullet points describing the intended content of each chapter.
4. Review and refine the document structure.
5. Replace the stub bullet points with prose.
6. Remove all remaining placeholder text before the document is considered complete.

Notes
=====

- Use Markdown.
- Write all document content in English.
- Keep the document implementation-independent unless it is explicitly a design document.
- Use concise, technical language.
- Keep chapter numbering sequential.
- Remove unused placeholder chapters and appendices.
-->

# <Document Title>

Status: Draft

# 1. Purpose

- ...

---

# 2. Scope

- ...

---

# 3. Definitions and Abbreviations

- <Definition category>
- <Abbreviation category>

---

# 4. References

- Internal references
- External references

---

# <N>. <Chapter Title>

- <Topic 1>
- <Topic 2>
- <Topic 3>

---

# <N>. <Chapter Title>

- <Topic 1>
- <Topic 2>

---

# <N>. Verification

- <Verification topic>

---

# Appendix A – <Appendix Title>

- <Appendix content>

---

# Appendix B – <Appendix Title>

- <Appendix content>
```

# ChatGPT Anmerkungen

## Hub
```text
Ich dachte immer, ein Hub ist auch nur ein Device. Ein virtueller Hub müsste ja dann wie eine VirtUSB-Instanz sein, oder?

Die Frage ist berechtigt. Und tatsächlich hast du einen wichtigen Punkt getroffen.

Die Antwort lautet:

Ja, ein Hub ist aus Sicht des USB-Protokolls zunächst einmal ein ganz normales USB-Gerät.

Der Unterschied ist lediglich, was dieses Gerät tut.

Ein normales USB-Gerät
Host
 │
 └── HID Keyboard

Das Keyboard besitzt:

Device Descriptor
Configuration Descriptor
Interface(s)
Endpoint(s)

Fertig.

Ein USB-Hub
Host
 │
 └── USB Hub

Auch der Hub besitzt:

Device Descriptor
Configuration Descriptor
Interface(s)
Endpoint(s)

Also zunächst genau dasselbe.

Zusätzlich besitzt er aber:

Downstream-Ports
Port Status
Port Power
Port Reset
Connection Detection
usw.

Diese Funktionen werden über Hub Class Requests gesteuert.

Woher kommen dann die Geräte hinter dem Hub?

Jetzt wird es interessant.

Host
 │
 └── Hub
      │
      ├── Keyboard
      └── Mouse

Der Hub meldet dem Host:

"An Port 1 wurde etwas eingesteckt."

Der Host führt anschließend die komplette Enumeration dieses neuen Gerätes durch.

Der Hub enthält also nicht die Maus oder das Keyboard.

Er sagt lediglich:

"An Port X befindet sich jetzt ein Gerät."

Was bedeutet das für VirtUSB?

Hier muss man unterscheiden zwischen:

VirtUSB

VirtUSB emuliert einen Host Controller.

Linux USB Core
        │
VirtUSB HCD
        │
Root Hub

Der Root Hub gehört zum Host Controller.

Ein virtueller Hub

Ein virtueller Hub wäre dagegen einfach ein weiteres USB-Gerät.

Root Hub
    │
    └── Virtual Hub

Bis hierhin ist alles völlig normal.

Jetzt kommt der Knackpunkt

Wenn an diesem Hub später ein Keyboard erscheint:

Root Hub
    │
    └── Virtual Hub
            │
            └── Keyboard

Wer erzeugt das Keyboard?

Das ist keine USB-, sondern eine VirtUSB-Architekturfrage.

USB sagt nur:

Der Hub meldet dem Host:

"Port 3: Device Connected."

Wie VirtUSB diese Information intern erzeugt, ist völlig offen.

Und jetzt glaube ich, sehe ich deine ursprüngliche Idee

Wenn ich dich richtig verstehe, denkst du eher an so etwas:

VirtUSB Instance
        │
        └── Backend
                │
                └── Virtual Hub
                        │
                        ├── Backend
                        │      └── Keyboard
                        │
                        └── Backend
                               └── Mouse
Das wäre tatsächlich eine sehr elegante Architektur.

Der Hub wäre einfach wieder ein Backend.

Er würde lediglich zusätzlich Port-Events erzeugen.

Das gefällt mir ehrlich gesagt besser

Denn damit bleibt der Grundsatz erhalten:

Jedes USB-Gerät wird durch genau ein Backend repräsentiert.

Ein Hub ist eben nur ein spezielles USB-Gerät.

Die Topologie entsteht dadurch, dass ein Hub weitere Geräte "unter sich" ankündigt.

Das ist wesentlich näher am echten USB-Modell als meine erste Überlegung.

Ich glaube sogar, dass wir daraus einen allgemeinen Architekturgrundsatz ableiten können:

VirtUSB modelliert USB-Geräte unabhängig von ihrer Geräteklasse. Ein USB-Hub ist daher kein Sonderfall der Architektur, sondern ein Backend, das zusätzlich Hub-spezifisches Verhalten implementiert.

Das wäre eine sehr schöne Eigenschaft der Architektur, weil sie USB möglichst unverändert abbildet und keine künstlichen Sonderregeln für Hubs einführt.
```

```text
Und die "Hub Class Requests" werden ja von Linux abgearbeitet.

Genau! Und das ist der entscheidende Punkt. Ich glaube, wir haben gerade einen wichtigen Architekturbaustein gefunden.

Ein USB-Hub ist kein besonderer Teilnehmer. Er ist einfach ein USB-Gerät der Hub-Klasse.

Der Ablauf wäre dann:

Linux USB Core
        │
        │  GET_DESCRIPTOR
        │  SET_CONFIGURATION
        │  GET_STATUS
        │  CLEAR_FEATURE
        │  ...
        ▼
VirtUSB
        ▼
Hub Backend

Die Hub-spezifischen Class Requests (GET_PORT_STATUS, SET_PORT_FEATURE, CLEAR_PORT_FEATURE, ...) werden genauso wie alle anderen Control Transfers an das Backend weitergeleitet.

Das Backend antwortet einfach entsprechend der USB-Hub-Spezifikation.

Wer übernimmt die Enumeration?

Die Enumeration übernimmt weiterhin Linux.

Wenn das Hub-Backend meldet:

Port 2: Connection Change

dann macht der Linux-USB-Stack genau das Gleiche wie bei einem echten Hub:

GET_PORT_STATUS
SET_PORT_FEATURE(PORT_RESET)
erneutes GET_PORT_STATUS
Enumeration des neuen Geräts

VirtUSB muss dafür keine eigene Hub-Logik erfinden.

Konsequenz für VirtUSB

Das spricht sehr dafür, VirtUSB möglichst transparent zu halten.

Der Kernel stellt die URBs bereit.

VirtUSB transportiert sie.

Das Backend implementiert die Geräteklasse.

Ob das nun

HID,
CDC,
MSC,
DFU,
Audio,
Hub

ist, spielt architektonisch keine Rolle.

Ein kleiner Haken bleibt

Es gibt nur eine Stelle, die wir irgendwann sauber definieren müssen:

Wie teilt ein Hub-Backend VirtUSB mit, dass sich an einem Downstream-Port etwas geändert hat?

Denn bei echter Hardware kommt diese Information vom Host Controller (Interrupt des Hubs).

Bei VirtUSB muss sie vom Hub-Backend kommen.

Das ist aber keine USB-Frage, sondern eine Frage der Backend-Schnittstelle.

Ich würde das deshalb nicht jetzt lösen, sondern später in der Kernel/User Interface Specification bzw. der Userspace Communication Protocol behandeln.

Interessante Konsequenz

Ich glaube, wir können daraus sogar einen Architekturgrundsatz ableiten:

VirtUSB kennt keine besonderen Gerätetypen. Alle USB-Geräte – einschließlich USB-Hubs – werden über dieselbe Backend-Architektur modelliert. Unterschiede ergeben sich ausschließlich aus ihrem USB-Protokollverhalten und den unterstützten Geräteklassen.

Das halte ich für eine sehr starke Eigenschaft der Architektur. Sie vermeidet Sonderfälle und nutzt den Linux-USB-Stack genau so, wie er für reale Hardware vorgesehen ist.
```

```text
Ein kleiner Haken bleibt

Und wie löst man den Haken?

Der vermeintliche Haken wird durch den normalen USB-Hub-Mechanismus gelöst. Dafür braucht VirtUSB kein spezielles Hub-Event außerhalb von USB.

Portänderung melden

Ein USB-Hub besitzt einen Interrupt-IN-Endpunkt. Der Linux-Hub-Treiber reicht dafür regelmäßig einen Interrupt-Transfer ein.

Das Hub-Backend behandelt ihn wie ein reales Hub-Gerät:

Linux hub driver
      │
      │ Interrupt-IN transfer
      ▼
VirtUSB
      ▼
Hub backend

Solange sich kein Portzustand geändert hat, bleibt der Transfer aus Sicht des Backends ohne Nutzdaten beziehungsweise wird noch nicht abgeschlossen. Das entspricht funktional dem NAK-Verhalten eines realen Geräts.

Ändert sich ein Port, beispielsweise weil ein virtuelles Gerät eingesteckt wurde, setzt das Hub-Backend das entsprechende Change-Bit und schließt den wartenden Interrupt-IN-Transfer mit einer Bitmap ab:

Bit 0  Hub status changed
Bit 1  Port 1 changed
Bit 2  Port 2 changed
Bit 3  Port 3 changed
...

Beispiel:

0000 0100b

bedeutet: Der Zustand von Port 2 hat sich geändert.

Linux reagiert darauf selbstständig und sendet anschließend die üblichen Hub-Class-Requests:

GET_STATUS(port 2)
SET_FEATURE(PORT_RESET)
CLEAR_FEATURE(C_PORT_CONNECTION)
...

Diese Control Transfers gelangen wiederum zum Hub-Backend.

Kein zusätzlicher Hub-spezifischer Meldekanal

Damit gilt:

Virtuelles Einstecken
        ↓
Hub backend updates port state
        ↓
Hub backend completes pending Interrupt-IN transfer
        ↓
Linux hub driver reads port status
        ↓
Linux starts enumeration

VirtUSB muss dafür weder die Hub-Klasse verstehen noch ein proprietäres Ereignis wie HUB_PORT_CHANGED definieren.

Was VirtUSB zusätzlich modellieren muss

Es braucht allerdings eine interne Control-Plane-Operation, mit der ein virtuelles Gerät an einen Downstream-Port des Hub-Geräts gebunden wird:

attach device B
    parent = device A
    port   = 3

Nicht nur:

attach device B
    root_hub_port = 3

Die Topologie muss also hierarchisch adressierbar sein:

VirtUSB Root Hub
└── Hub backend A
    ├── Port 1
    ├── Port 2
    └── Port 3
        └── Device backend B

Diese Attach-Operation verändert zunächst nur den internen Zustand:

Device B wird Port 3 von Hub A zugeordnet.
Hub A setzt PORT_CONNECTION und C_PORT_CONNECTION.
Hub A schließt den Interrupt-IN-Transfer ab.
Linux entdeckt das neue Gerät über den normalen USB-Ablauf.
Die nachfolgenden Transfers werden Device B zugeordnet.
Wesentliche Trennung

Es existieren damit zwei Ebenen:

Control Plane von VirtUSB

Verwaltet die virtuelle physische Topologie:

welches Gerät an welchem Port steckt,
welches Backend zu welchem Gerät gehört,
Attach und Detach.
USB Data Plane

Bildet ausschließlich reales USB-Verhalten ab:

Interrupt-IN-Bitmap des Hubs,
Hub-Class-Requests,
Port Reset,
Enumeration,
Control-, Bulk-, Interrupt- und Isochronous-Transfers.

Damit bleibt der sichtbare Ablauf vollständig USB-konform. Die einzige VirtUSB-spezifische Operation ist das virtuelle Einstecken eines Backends an einen bestimmten Port. Alles danach erledigt Linux genauso wie bei echter Hardware.
```

```text
Welche Konsequenzen ergeben sich daraus?

Ich glaube, die Konsequenzen sind größer, als es zunächst aussieht. Sie sind aber überwiegend positiv, weil sie die Architektur vereinfachen.

1. VirtUSB muss Topologien unterstützen

Bisher dachten wir implizit:

Root Hub
   ├── Device A
   ├── Device B
   └── Device C

Mit Hubs wird daraus:

Root Hub
   ├── Device A
   ├── Hub B
   │      ├── Device C
   │      └── Device D
   └── Device E

Die Topologie ist also ein Baum, keine flache Liste.

2. Ports gehören nicht mehr ausschließlich zum Root Hub

Bisher konnte man denken:

Controller
    ↓
31 Root-Hub-Ports

Künftig gilt:

Der Root Hub besitzt 31 Ports.
Jeder USB-Hub besitzt weitere Ports.
Ein Device steckt immer an genau einem Port.

Der Port wird damit zu einer allgemeinen Eigenschaft eines Hubs, nicht des Controllers.

3. "Attach" muss allgemeiner werden

Bisher:

Attach(Device, RootHubPort)

Eigentlich müsste es heißen:

Attach(Device, ParentHub, Port)

oder allgemeiner:

Attach(Device, ParentPort)

Denn jedes Device steckt an genau einem Port.

4. Das Backendmodell wird sogar einfacher

Das gefällt mir besonders.

Alle Backends sind gleich.

HID Backend
CDC Backend
MSC Backend
Hub Backend

VirtUSB kennt keine Sonderfälle.

Ein Hub ist einfach ein weiteres USB-Gerät.

5. Kapitel 8 wird wichtiger

Unser Kapitel

Virtual Device Requirements

passt plötzlich hervorragend.

Denn ein Hub ist eben auch:

Device creation
Device attachment
Device state
Device reset
Descriptoren
Endpoints
Detachment
Removal

Es braucht praktisch keine Sonderregeln.

6. Die Control Plane bekommt Topologie

Bisher dachte ich:

Controller
    ↓
Device

Jetzt eher:

Controller
      │
      ▼
Root Hub
      │
      ▼
Topology Manager
      │
      ▼
USB Tree

Die USB-Transfers interessieren sich dafür kaum.

Nur das Einstecken/Ausstecken verändert den Baum.

7. Das Interface sollte topologiebasiert sein

Ich würde inzwischen vermeiden:

attach(device, port=5)

und eher etwas wie

attach(device, parent_device, port)

oder

attach(device, parent_port)

Dadurch funktioniert dieselbe API sowohl für den Root Hub als auch für beliebig viele weitere Hubs.

Der wichtigste Punkt

Ich glaube, wir haben gerade einen Grundsatz gefunden, den ich unbedingt in einem ADR festhalten würde:

VirtUSB modelliert die USB-Topologie statt lediglich Geräte an Root-Hub-Ports.

Das klingt zunächst nach einer kleinen Änderung, ist aber ein wesentlicher Unterschied.

Denn dadurch gilt automatisch:

Root Hub → USB-Gerät
USB Hub → USB-Gerät
HID → USB-Gerät
CDC → USB-Gerät
MSC → USB-Gerät

VirtUSB behandelt nicht Geräteklassen, sondern USB-Geräte in einer USB-Topologie.

Eine Konsequenz sehe ich allerdings kritisch

Wenn wir diesen Weg gehen, würde ich die bisherige Aussage

"Backend = Virtual USB Device"

leicht abschwächen.

Ich würde stattdessen formulieren:

Eine Backend-Instanz repräsentiert genau ein virtuelles USB-Gerät.

Das ist eine subtile, aber wichtige Unterscheidung. Das Backend ist die Software, das virtuelle USB-Gerät das emulierte Objekt. Ein Hub-Backend repräsentiert also genau ein Hub-Gerät, ein HID-Backend genau eine Tastatur oder Maus usw.

Das bleibt auch dann konsistent, wenn später weitere Gerätetypen oder komplexere Topologien hinzukommen.
```

```text
Welche Dokumente müssten dazu angefasst werden?

Ich glaube, wir sollten hier zwischen zwingend und optional unterscheiden.

Die gute Nachricht: Ich sehe kein Dokument, das grundlegend umgeschrieben werden müsste. Die Architektur ist erstaunlich robust.

Dokument	Änderung	Bewertung
High-Level Architecture	USB-Topologie statt flache Root-Hub-Geräteliste expliziter beschreiben	🟡 klein
System Requirements	keine	✅
Software Requirements	evtl. "Topologie" ergänzen	🟡 klein
Backend Requirements	praktisch keine	✅
Kernel/User Interface Specification	ja	🔴
Userspace Communication Protocol	ja	🔴
ADR	ja	🔴
1. High-Level Architecture (kleine Änderung)

Ich würde vermutlich nur ein Diagramm anpassen.

Bisher denkt der Leser wahrscheinlich:

Controller
   │
Root Hub
   ├── Device
   ├── Device
   └── Device

Ich würde künftig lieber zeigen:

Controller
   │
Root Hub
   ├── Device
   ├── Hub
   │     ├── Device
   │     └── Device
   └── Device

Dadurch wird sofort klar:

VirtUSB modelliert eine USB-Topologie.

2. Software Requirements (kleine Änderung)

Hier würde ich eventuell eine einzige Responsibility ergänzen.

Heute steht dort:

representation, attachment, disconnection, and removal of virtual USB devices

Vielleicht ergänzt um:

management of the virtual USB topology

Mehr würde ich nicht ändern.

3. Backend Requirements

Erstaunlicherweise fast nichts.

Ein Hub ist weiterhin:

ein Device
Descriptoren
Endpoints
Reset
State Machine

Kapitel 8 passt weiterhin.

Ab hier wird es interessant
4. Kernel/User Interface Specification

Hier ändert sich tatsächlich etwas.

Bisher hätte ich vermutlich etwas geschrieben wie:

attach(device, controller, root_port)

Jetzt eher:

attach(device, parent_device, port)

oder

attach(device, parent_hub, port)

Die API wird dadurch allgemeiner.

5. Userspace Communication Protocol

Dasselbe.

Nicht:

ROOT_PORT = 5

sondern eher

PARENT_DEVICE = Hub42
PORT = 3

oder ähnliches.

6. Neuer ADR

Den halte ich inzwischen für sinnvoll.

Etwa:

ADR-00xx
USB Topology Model

Inhalt:

VirtUSB modelliert USB-Topologien.
Root Hub und USB-Hubs werden gleich behandelt.
Ein Hub ist ein normales USB-Gerät.
Topologieänderungen erfolgen durch Attach/Detach.
Enumeration bleibt Aufgabe des Linux-USB-Stacks.
Das Interessanteste

Mir ist beim Nachdenken etwas aufgefallen.

Eigentlich brauchen wir gar keine Sonderbehandlung für Hubs.

Stattdessen braucht VirtUSB lediglich zwei Objekte:

Device
Port

Mehr nicht.

Denn:

Ein Device besitzt eventuell...

Descriptoren
Endpoints
...

Ein Hub besitzt zusätzlich...

Ports

Und ein Port besitzt genau...

0 oder 1 Device

Das ist exakt das physische USB-Modell.

Deshalb würde ich nicht schreiben

VirtUSB unterstützt USB-Hubs.

Sondern:

VirtUSB modelliert die USB-Topologie. USB-Hubs ergeben sich daraus als normale USB-Geräte mit Downstream-Ports.

Ich halte das für die elegantere Architektur. Sie vermeidet Sonderfälle und orientiert sich eng am realen USB-Modell.

Meine Empfehlung

Ich würde jetzt nichts an den bestehenden Dokumenten ändern.

Stattdessen würde ich beim Erstellen der Kernel/User Interface Specification und der Userspace Communication Protocol Specification diese Topologie als Grundlage verwenden und anschließend einen ADR schreiben, der diese Architekturentscheidung dokumentiert.

So vermeiden wir unnötige Änderungen an bereits fertiggestellten Dokumenten und halten die Architektur konsistent.
```