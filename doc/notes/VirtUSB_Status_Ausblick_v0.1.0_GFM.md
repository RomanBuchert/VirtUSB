# VirtUSB -- Entwicklungsstand und Ausblick auf v0.1.0

## 1. Zweck dieses Dokuments

Dieses Dokument fasst den aktuellen Entwicklungsstand von **VirtUSB**
zusammen und beschreibt die nächsten geplanten Entwicklungsschritte bis
zu einer ersten Version **v0.1.0**.

Der dargestellte Stand umfasst die bisher implementierte Architektur,
das Objekt- und Lebenszyklusmodell, die virtuelle USB-Topologie, die
Control Plane sowie den aktuell erreichten End-to-End-Punkt der
USB-Enumeration unter Linux.

------------------------------------------------------------------------

## 2. Projektziel

VirtUSB stellt eine vollständig virtuelle USB-Infrastruktur für Linux
bereit. Ziel ist es, virtuelle USB-Geräte an virtuelle
USB-Hostcontroller anzuschließen, ohne eine physische USB-Verbindung
oder die elektrische Signal-, Bit- oder Paketebene zu emulieren.

Die Architektur besteht im Wesentlichen aus folgenden Ebenen:

``` mermaid
flowchart TB
    A["Linux-Anwendung / Gerätetreiber"]
    B["Linux USB Core"]
    C["VirtUsbHcd"]
    D["Virtuelle USB-Topologie"]
    E["VirtUsbDev"]
    F["Device-Backend / DCD"]
    G["USB Device Stack<br/>z. B. TinyUSB"]

    A <--> B
    B <--> C
    C <--> D
    D <--> E
    E <--> F
    F <--> G
```

VirtUSB selbst soll dabei keine USB-Device-Descriptoren interpretieren
oder erzeugen. Diese Aufgabe bleibt beim jeweiligen USB-Device-Stack
beziehungsweise Device-Backend.

------------------------------------------------------------------------

## 3. Bisher erreichter Stand

### 3.1 Virtueller USB-Hostcontroller

VirtUSB stellt einen Linux-USB-Hostcontroller bereit, der vom Linux USB
Core als regulärer HCD erkannt wird.

Beim Laden des Kernelmoduls wird beispielsweise ein virtueller USB-Bus
mit einem Root Hub und derzeit 31 Ports registriert.

Der Linux USB Core erkennt den HCD und initialisiert den Root Hub
regulär.

Damit ist die grundlegende Integration

``` text
VirtUSB Kernelmodul
        ↓
Linux HCD API
        ↓
Linux USB Core
        ↓
Root Hub
```

funktionsfähig.

------------------------------------------------------------------------

### 3.2 Trennung von Hardware-, Topologie- und USB-Protokollzustand

Die Zustandsmodelle wurden im Verlauf der Architekturarbeit klar
voneinander getrennt.

`VirtUsbPort` beschreibt lokale Eigenschaften eines virtuellen Ports.
USB-Protokollzustände werden nicht redundant im Port gespeichert.

Insbesondere werden unterschieden:

-   lokale Port-Hardwareeigenschaften,
-   Attachment beziehungsweise Topologie,
-   simulierte Versorgung über VBUS,
-   Device-seitiges Connection Signaling,
-   USB-sichtbarer Hub-Port-Zustand,
-   USB-Change-Bits,
-   aktueller USB-Protokollzustand.

Die Verbindung zweier Ports wird ausschließlich über eine reziproke
Peer-Beziehung modelliert.

``` mermaid
flowchart LR
    D["VirtUsbDev"]
    U["Upstream Port"]
    P["peer"]
    R["Downstream Port<br/>Root Hub / Hub"]
    H["VirtUsbHub"]

    D --> U
    U <-->|"Attachment"| R
    R --> H
```

Interne Device-Zustände wie SELF-Power, interne Stromversorgung oder
generische Hardware-Verfügbarkeit liegen bewusst außerhalb von VirtUSB.

VirtUSB modelliert nur die für USB relevanten Zustände.

------------------------------------------------------------------------

### 3.3 Device-seitiges Connection Signaling

Ein `VirtUsbDev` besitzt einen Upstream-Port mit Device-seitigem
Connection Signaling.

Dieses Signaling entspricht funktional dem Verhalten eines Device
Controllers, der seine USB-Präsenz gegenüber dem Host aktiviert oder
deaktiviert.

Der effektive USB-Verbindungszustand ergibt sich aus mehreren
Bedingungen:

``` mermaid
flowchart TD
    A["Device ist attached"]
    B["Downstream-Port hat VBUS"]
    C["Device Connection Signaling aktiv"]
    D["USB-visible PORT_CONNECTION"]

    A --> D
    B --> D
    C --> D
```

Nur wenn die erforderlichen Bedingungen erfüllt sind, erscheint das
Device für den Host als verbunden.

Dadurch bleibt beispielsweise ein Self-Powered Device intern außerhalb
des Zuständigkeitsbereichs von VirtUSB. Für VirtUSB ist lediglich
relevant, ob VBUS vorhanden ist und ob das Device seine USB-Verbindung
signalisiert.

------------------------------------------------------------------------

### 3.4 Hub- und Portmodell

Das gemeinsame Hubmodell verwaltet die USB-relevanten Zustände der
Downstream-Ports.

Dazu gehören unter anderem:

-   Port Power,
-   Connection,
-   Enable,
-   Suspend,
-   Reset,
-   Over-Current,
-   Change-Bits,
-   aktuelle USB-Geschwindigkeit.

Power Switching und Over-Current-Verhalten werden als Eigenschaften
beziehungsweise Capabilities des Hubs modelliert.

Der Control Layer und der USB-Hub-Layer greifen auf denselben
kanonischen Hardwarezustand zu. Es existiert beispielsweise kein
separater Port-Power-Zustand für `virtusbctl`.

------------------------------------------------------------------------

### 3.5 Hub Status Change Notification

Änderungen an USB-sichtbaren Portzuständen werden über einen gemeinsamen
Notification-Mechanismus weitergegeben.

Für den Root Hub wird dieser Mechanismus auf die Linux-HCD-Infrastruktur
abgebildet:

``` mermaid
sequenceDiagram
    participant Dev as VirtUsbDev
    participant Hub as VirtUsbHub Model
    participant HCD as VirtUsbHcd
    participant USB as Linux USB Core

    Dev->>Hub: Connection-Zustand ändert sich
    Hub->>Hub: C_PORT_CONNECTION setzen
    Hub->>HCD: Status Change Notification
    HCD->>USB: usb_hcd_poll_rh_status()
    USB->>HCD: Hub-Status abfragen
```

Die Architektur ist so angelegt, dass derselbe abstrakte
Notification-Mechanismus später auch für normale externe virtuelle Hubs
verwendet werden kann. Dort würde die Benachrichtigung über den
Interrupt-IN-Endpoint des Hubs transportiert.

------------------------------------------------------------------------

## 4. Objektmodell und Lebenszyklus

### 4.1 VirtUsbObjMgr

VirtUSB besitzt inzwischen einen gemeinsamen Object Manager für
verwaltete Core Components.

Der Object Manager stellt insbesondere bereit:

-   globale Objektidentität,
-   Registry,
-   Lookup,
-   Referenzzählung,
-   kontrollierten Objektlebenszyklus.

Die Referenzzählung basiert auf `kref`, die Registry auf einer `xarray`.

Objekt-IDs sind 32 Bit breit. ID `0` ist ungültig.

Während einer Modullaufzeit werden IDs monoton vergeben und nicht
wiederverwendet.

Beispiel:

``` text
create → ID 1
create → ID 2
create → ID 3
destroy ID 2
create → ID 4
```

Nach einem vollständigen Entladen und erneuten Laden des Kernelmoduls
darf die Nummerierung wieder bei `1` beginnen.

------------------------------------------------------------------------

### 4.2 VirtUsbDev als verwaltetes Core Component

`VirtUsbDev` ist das erste Core Component, das vollständig in dieses
Objektmodell integriert wurde.

Der grundsätzliche Erzeugungspfad lautet:

``` mermaid
flowchart TD
    A["VirtUsbDev allozieren"]
    B["Common Object / kref initialisieren"]
    C["Device initialisieren"]
    D["Im VirtUsbObjMgr registrieren"]
    E["Globale Object-ID vergeben"]
    F["Objekt für Lookup publizieren"]

    A --> B --> C --> D --> E --> F
```

Ein Lookup erzeugt eine zusätzliche Referenz. Nach dem Unregister sind
keine neuen Lookups mehr möglich, bereits bestehende Referenzen bleiben
jedoch gültig.

------------------------------------------------------------------------

### 4.3 Normales und erzwungenes Zerstören

Für den Lebenszyklus wurden drei Fälle unterschieden.

#### Normales Destroy

Das normale Zerstören ist konservativ.

Ein Objekt darf die Operation beispielsweise mit `-EBUSY` ablehnen, wenn
noch Zustände existieren, die vorher explizit bereinigt werden müssen.

#### Forced Destroy

Ein administrativ angefordertes Forced Destroy führt zunächst das für
den konkreten Objekttyp notwendige Cleanup durch.

Bei einem `VirtUsbDev` gehören dazu derzeit insbesondere Disconnect und
Detach.

Der Object Manager selbst implementiert diese Semantik nicht. Sie gehört
zum jeweiligen Core Component.

#### Module Shutdown

Beim Entladen des Kernelmoduls werden noch vorhandene Core Components
kontrolliert zwangsweise heruntergefahren.

``` mermaid
flowchart TD
    A["Module Unload beginnt"]
    B["Keine neuen Management-Operationen"]
    C["Verbleibende Core Components<br/>kontrolliert herunterfahren"]
    D["Objekte unregister"]
    E["HCD-Infrastruktur abbauen"]
    F["ObjMgr muss leer sein"]
    G["VirtUsbObjMgr beenden"]

    A --> B --> C --> D --> E --> F --> G
```

Der Object Manager zerstört beim eigenen Shutdown keine verbliebenen
Objekte stillschweigend. Eine nicht leere Registry weist auf einen
Fehler im vorherigen Shutdown-Ablauf hin.

Der Forced-Cleanup-Pfad wurde praktisch durch Module-Unload mit noch
vorhandenen Devices getestet.

------------------------------------------------------------------------

## 5. Control Plane und virtusbctl

Die VirtUSB-Control-Plane ist über `/dev/virtusbX` erreichbar.

Darauf bauen `libvirtusb` und das Kommandozeilenwerkzeug `virtusbctl`
auf.

Derzeit können virtuelle Devices unter anderem erzeugt und zerstört
werden:

``` text
virtusbctl device create full
virtusbctl device destroy OBJECT_ID
virtusbctl device destroy OBJECT_ID --force
```

Zusätzlich stehen Topologie- und Connection-Operationen zur Verfügung:

``` text
virtusbctl device attach OBJECT_ID PORT
virtusbctl device detach OBJECT_ID
virtusbctl device connect OBJECT_ID
virtusbctl device disconnect OBJECT_ID
```

Der simulierte VBUS-Zustand eines Root-Hub-Ports kann ebenfalls
manipuliert werden:

``` text
virtusbctl port power PORT on
virtusbctl port power PORT off
```

Die aktuelle Implementierung adressiert bei diesen Operationen zunächst
den Root Hub des ausgewählten HCD. Eine allgemeinere Hub-Adressierung
kann später auf dem vorhandenen Hub-/Objektmodell aufbauen.

------------------------------------------------------------------------

## 6. Port Reset und Geschwindigkeit

Der Root-Hub-Port-Reset ist inzwischen so weit implementiert, dass Linux
die Enumeration eines angeschlossenen Devices beginnen kann.

Der Reset wird derzeit synchron abgeschlossen.

Der Ablauf ist:

``` mermaid
sequenceDiagram
    participant Linux as Linux USB Core
    participant HCD as VirtUsbHcd
    participant Hub as VirtUsbHub
    participant Dev as VirtUsbDev

    Linux->>HCD: SetPortFeature(PORT_RESET)
    HCD->>Hub: Port Reset
    Hub->>Hub: PORT_ENABLE = 0
    Hub->>Hub: PORT_RESET = 1
    Hub->>Hub: effektive Speed bestimmen
    Hub->>Hub: PORT_RESET = 0
    Hub->>Hub: PORT_ENABLE = 1
    Hub->>Hub: C_PORT_RESET = 1
    Hub->>HCD: Status Change Notification
    HCD->>Linux: Reset abgeschlossen
```

Die aktuelle Geschwindigkeit wird aus der Schnittmenge der
Speed-Capabilities beider verbundenen Ports bestimmt.

Bei mehreren gemeinsamen Möglichkeiten wird derzeit die höchste
unterstützte Geschwindigkeit gewählt:

``` text
HIGH → FULL → LOW
```

Die aktuelle Geschwindigkeit bleibt dabei USB-Protokollzustand und wird
nicht als redundanter Hardwarezustand im Port gespeichert.

------------------------------------------------------------------------

## 7. Aktuell erreichter End-to-End-Meilenstein

Der derzeit wichtigste praktische Meilenstein ist erreicht:

> Linux erkennt ein mit VirtUSB erzeugtes und verbundenes virtuelles
> Device, führt den Port Reset aus, erkennt dessen Geschwindigkeit und
> beginnt die USB-Enumeration.

Der vollständige bisherige Ablauf lautet:

``` mermaid
flowchart TD
    A["VirtUsbDev erzeugen"]
    B["Object-ID vergeben"]
    C["Device an Root-Hub-Port attachen"]
    D["VBUS vorhanden"]
    E["Connection Signaling aktivieren"]
    F["PORT_CONNECTION"]
    G["C_PORT_CONNECTION"]
    H["Root-Hub Notification"]
    I["Linux erkennt Device"]
    J["PORT_RESET"]
    K["Reset Completion"]
    L["Speed bestimmen"]
    M["PORT_ENABLE"]
    N["Linux beginnt Enumeration"]
    O["Transferpfad noch nicht implementiert"]
    P["Enumeration schlägt erwartungsgemäß fehl"]

    A --> B --> C --> D --> E --> F --> G --> H --> I --> J --> K --> L --> M --> N --> O --> P
```

Ein aktueller Test mit einem Full-Speed-Device führte unter Linux
beispielsweise bis zu Meldungen der Form:

``` text
usb 5-1: new full-speed USB device number ... using virtusb-hcd
usb usb5-port1: unable to enumerate USB device
```

Das Scheitern der Enumeration entspricht dem derzeitigen
Implementierungsstand. Linux versucht bereits, mit dem virtuellen Device
zu kommunizieren; der eigentliche Device-Transferpfad ist jedoch noch
nicht vorhanden.

------------------------------------------------------------------------

## 8. Aktuelle funktionale Grenze

Die bisher implementierte Infrastruktur deckt insbesondere folgende
Bereiche ab:

| Bereich | Stand |
| --- | --- |
| Linux HCD Integration | vorhanden |
| Root Hub | vorhanden |
| 31 Root-Hub-Ports | vorhanden |
| Virtuelle Port-Topologie | vorhanden |
| Attachment / Detachment | vorhanden |
| VBUS / Port Power | vorhanden |
| Device Connection Signaling | vorhanden |
| USB-visible Connection | vorhanden |
| Hub Change Notification | vorhanden |
| Port Reset | vorhanden |
| Reset Completion | vorhanden |
| Speed-Bestimmung | vorhanden |
| Object Manager | vorhanden |
| Device Object Lifecycle | vorhanden |
| Normal / Forced Destroy | vorhanden |
| Module-Unload-Cleanup | vorhanden |
| Control Plane | vorhanden |
| `libvirtusb` | grundlegend vorhanden |
| `virtusbctl` | grundlegend vorhanden |
| Linux beginnt Device-Enumeration | erreicht |
| Device-Transfermodell | ausstehend |
| EP0 / Control Transfers | ausstehend |
| Device-Backend-Schnittstelle | ausstehend |
| Erfolgreiche Enumeration | ausstehend |
| Bulk Transfers | ausstehend |
| Interrupt Transfers | ausstehend |
| Isochronous Transfers | später |

Damit liegt die wesentliche nächste Entwicklungsphase nicht mehr bei der
grundlegenden Topologie, sondern bei der tatsächlichen
USB-Datenübertragung.

------------------------------------------------------------------------

# 9. Nächste Entwicklungsschritte

## 9.1 Transfermodell

Als nächstes muss ein generisches internes Transfermodell definiert
werden.

Dieses Modell muss unter anderem festlegen:

-   Repräsentation eines USB-Transfers,
-   Zuordnung zum `VirtUsbDev`,
-   Endpoint-Zuordnung,
-   Transfer-Richtung,
-   Datenpuffer,
-   Ownership,
-   Referenzzählung,
-   Completion,
-   Fehlerstatus,
-   Cancellation beziehungsweise Unlink,
-   Verhalten bei Disconnect und Destroy.

Dieses Modell bildet anschließend die gemeinsame Grundlage für Control-,
Bulk-, Interrupt- und gegebenenfalls später Isochronous-Transfers.

``` mermaid
flowchart LR
    U["Linux URB"]
    H["VirtUsbHcd"]
    T["VirtUSB Transfer"]
    D["VirtUsbDev"]
    B["Device Backend"]
    S["Device Stack"]

    U --> H
    H --> T
    T --> D
    D --> B
    B --> S

    S --> B
    B --> D
    D --> T
    T --> H
    H --> U
```

------------------------------------------------------------------------

## 9.2 EP0 und Control Transfers

Der erste konkrete Transferpfad soll Endpoint 0 unterstützen.

Das erste Ziel ist, den von Linux während der Enumeration erzeugten
Request

``` text
GET_DESCRIPTOR(Device)
```

bis zur Device-Seite zu transportieren.

VirtUSB soll den Descriptor dabei nicht selbst kennen oder
interpretieren.

Der Request wird lediglich zwischen Host- und Device-Seite
transportiert.

------------------------------------------------------------------------

## 9.3 Device-Backend-Schnittstelle

Ein `VirtUsbDev` benötigt eine definierte Schnittstelle zu einem
Device-Backend.

Diese Schnittstelle bildet die Grenze zwischen VirtUSB und einem
eigentlichen USB-Device-Stack.

Ein wichtiges Ziel ist ein DCD für TinyUSB:

``` mermaid
flowchart LR
    L["Linux USB Core"]
    H["VirtUsbHcd"]
    D["VirtUsbDev"]
    V["VirtUSB Device Backend / DCD"]
    T["TinyUSB"]

    L <--> H
    H <--> D
    D <--> V
    V <--> T
```

Dadurch kann TinyUSB ein virtuelles USB-Gerät bereitstellen, ohne
physische USB-Hardware zu benötigen.

------------------------------------------------------------------------

## 9.4 Erste erfolgreiche Enumeration

Der nächste große End-to-End-Meilenstein ist eine vollständig
erfolgreiche Enumeration.

Linux soll dann unter anderem:

1.  den Device Descriptor lesen,
2.  eine USB-Adresse vergeben,
3.  Configuration Descriptoren lesen,
4.  gegebenenfalls String Descriptoren lesen,
5.  eine Configuration auswählen,
6.  das Device regulär im USB-Gerätebaum registrieren.

Das sichtbare Ziel ist:

> Ein von einem VirtUSB-Backend bereitgestelltes Device erscheint mit
> eigenen Descriptoren, VID und PID regulär in `lsusb`.

------------------------------------------------------------------------

## 9.5 Bulk- und Interrupt-Transfers

Nach erfolgreicher EP0-Enumeration sollen zunächst Bulk- und
Interrupt-Transfers ergänzt werden.

Damit werden bereits viele praktisch relevante USB-Geräteklassen
beziehungsweise Testgeräte möglich, beispielsweise:

-   CDC,
-   HID,
-   Vendor-specific Devices.

Diese Transferarten sind für die erste Version wesentlich relevanter als
Isochronous Transfers.

------------------------------------------------------------------------

## 9.6 Robustheit und Fehlerfälle

Nach dem funktionalen Transferpfad müssen insbesondere Lifecycle- und
Parallelitätsfälle getestet werden.

Dazu gehören unter anderem:

-   Disconnect während eines Transfers,
-   Detach während eines Transfers,
-   Forced Destroy,
-   Module Unload,
-   URB Unlink / Cancellation,
-   Reset während aktiver Kommunikation,
-   Reconnect,
-   mehrere Devices,
-   mehrere Ports,
-   ungültige Endpoint-Operationen,
-   Backend-Fehler.

------------------------------------------------------------------------

# 10. Zielumfang von v0.1.0

Version **v0.1.0** soll noch keine vollständige USB-2.0-Implementierung
darstellen.

Sie soll einen ersten praktisch nutzbaren End-to-End-Stand definieren.

Als Ziel für v0.1.0 wird folgender Pfad betrachtet:

``` mermaid
flowchart TD
    A["VirtUsbDev erzeugen"]
    B["Attachen"]
    C["Connect"]
    D["Linux Enumeration"]
    E["EP0 Control Transfers"]
    F["Device Backend / TinyUSB"]
    G["Configuration erfolgreich"]
    H["Bulk / Interrupt Transfers"]
    I["Datenübertragung"]
    J["Disconnect"]
    K["Detach"]
    L["Destroy"]

    A --> B --> C --> D --> E --> F --> G --> H --> I --> J --> K --> L
```

Ein geeignetes virtuelles Testdevice soll:

-   erzeugt werden können,
-   an einen virtuellen Port angeschlossen werden können,
-   vom Linux USB Core erkannt werden,
-   erfolgreich enumeriert werden,
-   regulär in `lsusb` erscheinen,
-   Control Transfers durchführen,
-   reale Nutzdaten über mindestens Bulk oder Interrupt übertragen,
-   sauber getrennt und zerstört werden können.

Für einen belastbaren v0.1.0-Stand ist vorgesehen, sowohl **Bulk als
auch Interrupt** zu unterstützen.

------------------------------------------------------------------------

## 11. Nicht notwendiger Umfang für v0.1.0

Folgende Funktionen sind für v0.1.0 nicht zwingend erforderlich:

-   Isochronous Transfers,
-   vollständige USB-2.0-Abdeckung,
-   komplexe externe virtuelle Hub-Topologien,
-   vollständige Fault-Injection-Infrastruktur,
-   Performanceoptimierung für hohe Transferlast,
-   endgültig eingefrorene UAPI/ABI,
-   vollständige Produktionsreife.

Diese Funktionen können in späteren Entwicklungsstufen ergänzt werden.

------------------------------------------------------------------------

# 12. Einordnung des aktuellen Fortschritts

Der aktuelle Stand befindet sich an der Grenze zwischen zwei
Entwicklungsphasen.

Die erste Phase umfasste hauptsächlich:

``` text
Architektur
    ↓
Core Components
    ↓
Objektmodell
    ↓
Topologie
    ↓
Lifecycle
    ↓
Hubmodell
    ↓
Connection
    ↓
Reset
```

Diese Grundlagen sind inzwischen weitgehend vorhanden.

Die nächste Phase konzentriert sich auf:

``` text
Transfers
    ↓
EP0
    ↓
Backend
    ↓
Enumeration
    ↓
Bulk / Interrupt
    ↓
Robustheit
    ↓
v0.1.0
```

Eine exakte prozentuale Bewertung ist aufgrund der unterschiedlichen
Komplexität der einzelnen Bereiche nicht sinnvoll. Funktional ist ein
wesentlicher Teil der Infrastruktur vorhanden; das Transfermodell stellt
jedoch einen großen und zentralen noch offenen Entwicklungsblock dar.

------------------------------------------------------------------------

# 13. Nächste Meilensteine

Die weitere Entwicklung lässt sich in vier unmittelbar sichtbare
Meilensteine gliedern.

### Meilenstein 1 -- Erster Transfer erreicht die Device-Seite

Linux erzeugt während der Enumeration einen
`GET_DESCRIPTOR(Device)`-Request und dieser wird über das
VirtUSB-Transfermodell bis zum `VirtUsbDev` beziehungsweise
Device-Backend transportiert.

### Meilenstein 2 -- Erfolgreiche USB-Enumeration

Das Backend beantwortet die Control Requests vollständig.

Ein virtuelles Device erscheint mit eigenen Descriptoren, VID und PID in
`lsusb`.

### Meilenstein 3 -- Nutzdatenübertragung

Bulk- und Interrupt-Transfers funktionieren zwischen Linux und dem
Device-Backend.

Ein reales Testdevice, beispielsweise auf Basis von TinyUSB, kann Daten
mit dem Host austauschen.

### Meilenstein 4 -- v0.1.0

Die grundlegenden Transfer- und Lifecycle-Pfade sind getestet und
ausreichend robust.

``` mermaid
timeline
    title VirtUSB – Weg zu v0.1.0
    section Erreicht
        HCD und Root Hub : Linux-HCD-Integration
        Topologie : Attachment und Portmodell
        Lifecycle : ObjMgr und Forced Cleanup
        Connection : VBUS und Connection Signaling
        Enumeration Start : Reset und Speed-Ermittlung
    section Nächste Schritte
        Transfermodell : URB zu VirtUSB Transfer
        EP0 : GET_DESCRIPTOR bis Device-Backend
        Enumeration : Device erfolgreich in lsusb
        Nutzdaten : Bulk und Interrupt
        v0.1.0 : stabiler End-to-End-Betrieb
```

------------------------------------------------------------------------

## 14. Zusammenfassung

VirtUSB hat inzwischen den Punkt erreicht, an dem Linux ein vollständig
virtuell erzeugtes USB-Gerät an einem virtuellen Root-Hub-Port erkennt,
den Port zurücksetzt, die Gerätegeschwindigkeit bestimmt und mit der
Enumeration beginnt.

Damit sind die grundlegenden Architektur-, Topologie-, Lifecycle- und
Connection-Mechanismen vorhanden.

Der nächste zentrale Entwicklungsblock ist die eigentliche
USB-Transferinfrastruktur. Diese muss zunächst EP0 und Control Transfers
ermöglichen und anschließend als gemeinsame Grundlage für Bulk- und
Interrupt-Transfers dienen.

Der entscheidende nächste technische Nachweis lautet:

> Der erste `GET_DESCRIPTOR(Device)`-Request des Linux USB Core erreicht
> ein VirtUSB-Device-Backend.

Darauf folgt die erste vollständige Enumeration eines virtuellen
Devices. Sobald anschließend Bulk- und Interrupt-Transfers sowie die
wesentlichen Lifecycle- und Fehlerpfade funktionieren, ist der geplante
Funktionsumfang für **VirtUSB v0.1.0** erreicht.
