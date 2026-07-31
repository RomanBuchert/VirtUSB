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

## Misc
```text
Daraus ergibt sich ein allgemeiner Architekturgrundsatz

Ich würde ihn fast als ADR formulieren:

VirtUSB exposes host-controller semantics, not backend implementation details.

Oder etwas ausführlicher:

Backend implementations shall emulate the externally observable behaviour of a physical USB host controller. Backend implementation details shall not be exposed through the VirtUSB interfaces.

Ich finde diesen Satz ausgesprochen wichtig.

Er beantwortet später viele Detailfragen automatisch:

Wer erzeugt Timeouts?
Welche Fehlercodes gibt es?
Dürfen Backends eigene Fehlercodes definieren?
Darf ein Backend Pakete verwerfen?
Darf ein Backend USB-Verhalten "optimieren"?

Die Antwort lautet immer:

Nein, sofern sich ein echter Host-Controller nicht genauso verhalten würde.

Ich würde diesen Grundsatz sogar über Kapitel 9 hinaus anwenden. Er beschreibt sehr gut die Philosophie von VirtUSB und könnte als eigener ADR festgehalten werden. Darauf könnten sich spätere Kapitel wie die Kernel/User-Schnittstelle oder das Kommunikationsprotokoll direkt beziehen.
```

## Next Step: Architecture Review
```text
Bis jetzt haben wir im Wesentlichen die Anforderungsphase (Requirements Engineering) abgeschlossen. Die nächsten Dokumente sind keine Requirements mehr, sondern Spezifikationen bzw. Designs, die beschreiben, wie die Anforderungen umgesetzt werden.

Ich würde die Phasen daher so sehen:

Phase	Status
High-Level Architecture	✅
System Requirements	✅
Software Requirements	✅
Backend Requirements	✅
Architecture Review	← würde ich jetzt einfügen
Interface Specifications	danach
Protocol Specifications	danach
Detailed Design	danach
Implementierung	deutlich später
Warum ich jetzt ein Review machen würde

Ich glaube, jetzt ist der letzte sinnvolle Zeitpunkt.

Wenn wir erst Kernel/User Interface, Kommunikationsprotokoll und Ownership Model schreiben und dann feststellen, dass irgendwo in den Requirements eine Unklarheit steckt, zieht sich diese Unklarheit durch alle nachfolgenden Dokumente.

Jetzt betreffen Änderungen maximal vier Dokumente.

Später vielleicht zehn oder fünfzehn.

Ich würde das Review in vier Durchgänge aufteilen
1. Architektur-Review

Der wichtigste Punkt.

Fragen wie:

Gibt es Widersprüche?
Sind Verantwortlichkeiten eindeutig?
Fehlt irgendwo etwas?
Wird irgendwo implizit etwas vorausgesetzt?
Sind Begriffe konsistent?
Ist die Abgrenzung zwischen Requirements und Spezifikation sauber?

Das ist eigentlich schon ein Mini-Design-Review.

2. Konsistenz-Review

Hier würde ich explizit dokumentübergreifend prüfen:

Definitionen
Glossar
Begriffe
"Backend"
"Virtual USB Device"
"Instance"
"Controller"
"Root Hub"
"Transfer"
"Enumeration"

Gibt es überall dieselbe Bedeutung?

3. Dokumenten-/Format-Review

Hier eher handwerklich:

Kapitelstruktur
Nummerierung
Überschriften
Horizontal Rules (---)
Tabellen
Bullet-Style
Codeblöcke
Mermaid-Style
Appendix-Aufbau
Referenzen
Statuszeilen
Sprachstil

Das sorgt später für einen professionellen Gesamteindruck.

4. Verständlichkeits-Review

Das finde ich fast spannender als "aufhübschen".

Ich würde mich fragen:

"Wo würde ein neuer Entwickler nach zwei Minuten das Lesen verlieren?"

Dann gezielt überlegen:

lohnt sich hier ein Diagramm?
lohnt sich eine Tabelle?
lohnt sich eine kleine Ablaufgrafik?

Nicht mehr.

Nicht weniger.

Ich würde Diagramme ausschließlich dort einsetzen, wo sie Informationen transportieren.

Ich würde sogar noch einen fünften Punkt ergänzen
5. ADR-Kandidaten identifizieren

Während der letzten Wochen haben wir viele Entscheidungen getroffen.

Einige gehören wahrscheinlich nicht in Requirements.

Zum Beispiel:

Backend = Software, Device = emuliertes Objekt
USB-Topologie statt Root-Port-Modell
Host-Controller-Semantik statt Backend-Semantik
Dokumentationsworkflow
Architektur vor Implementierung

Ich würde beim Review markieren:

ADR?

ohne sie sofort zu schreiben.

Dann haben wir später eine Liste.

Danach beginnt eine neue Phase

Ich würde den Übergang bewusst machen.

Bis jetzt:

Was muss VirtUSB können?

Ab dann:

Wie realisieren wir das?

Das ist ein echter Meilenstein im Projekt.

Deshalb gefällt mir der Gedanke, vor diesem Übergang einmal "aufzuräumen", sehr gut. Danach startet die Spezifikationsphase auf einer möglichst konsistenten Grundlage.
```