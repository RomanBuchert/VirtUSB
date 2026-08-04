# VirtUSB Glossar

# Architekturebenen

Architekturebenen beschreiben unterschiedliche fachliche Betrachtungsebenen
eines virtuellen USB-Gerätes.

Jede Architekturebene besitzt einen klar abgegrenzten Verantwortungsbereich und
ist unabhängig von ihrer konkreten Implementierung.

## Geräteebene

Die **Geräteebene** beschreibt sämtliche gerätespezifischen Eigenschaften und
Zustände eines virtuellen USB-Gerätes unabhängig von der USB-Topologie, dem
USB-Device-Controller und dem USB-Protokoll.

Hierzu gehören beispielsweise:

- Existenz eines virtuellen Gerätes
- Ein- und Ausschalten
- Reset
- Firmware- oder Bootzustand
- gerätespezifischer interner Zustand

Die Geräteebene besitzt kein Wissen über die Position des Gerätes innerhalb der
USB-Topologie, den Zustand des USB-Device-Controllers oder den aktuellen
USB-Protokollzustand.

## USB-Topologieebene

Die **USB-Topologieebene** beschreibt die virtuelle USB-Infrastruktur sowie die
Beziehungen zwischen virtuellen USB-Hostcontrollern, Root-Hubs, USB-Hubs,
Ports und virtuellen USB-Geräten.

Hierzu gehören beispielsweise:

- virtuelle USB-Hostcontroller
- virtuelle Root-Hubs
- virtuelle USB-Hubs
- virtuelle USB-Ports
- Zuordnung eines virtuellen USB-Gerätes zu einem Port
- virtuelles Anstecken und Abstecken
- Portstatus und Portänderungen

Die USB-Topologieebene besitzt kein Wissen über den gerätespezifischen Zustand,
den Zustand des USB-Device-Controllers oder die USB-Protokollkommunikation.

## USB-Device-Controller-Ebene

Die **USB-Device-Controller-Ebene** beschreibt den Zustand und die Steuerung des
virtuellen USB-Device-Controllers eines virtuellen USB-Gerätes.

Hierzu gehören beispielsweise:

- Initialisierung des USB-Device-Controllers
- Deinitialisierung des USB-Device-Controllers
- USB Connect
- USB Disconnect
- Aktivierung und Deaktivierung von Endpoints

Die USB-Device-Controller-Ebene besitzt kein Wissen über den
gerätespezifischen Zustand oder die USB-Topologie. Sie bildet die Schnittstelle
zwischen der Geräteebene und der USB-Protokollebene.

## USB-Protokollebene

Die **USB-Protokollebene** beschreibt sämtliche durch die USB-Spezifikation
definierten Kommunikationsabläufe zwischen Host und USB-Geräten.

Hierzu gehören beispielsweise:

- Enumeration
- Standard Requests
- Class Requests
- Vendor Requests
- Endpoint-Verhalten
- Control-, Bulk-, Interrupt- und Isochronous-Transfers
- USB Suspend
- USB Resume

Die USB-Protokollebene besitzt kein Wissen über den gerätespezifischen Zustand
oder die USB-Topologie.

# Systemoperationen

Systemoperationen beschreiben grundlegende Operationen, welche den Zustand eines
virtuellen USB-Gerätes oder dessen Einbindung in die virtuelle USB-Infrastruktur
verändern.

## Einschalten

Das Einschalten aktiviert ein virtuelles USB-Gerät auf der Geräteebene.

Ein eingeschaltetes Gerät ist betriebsbereit. Es muss jedoch weder an einen
USB-Port angeschlossen sein noch einen initialisierten USB-Device-Controller
besitzen.

## Ausschalten

Das Ausschalten deaktiviert ein virtuelles USB-Gerät auf der Geräteebene.

Ein ausgeschaltetes Gerät kann weder einen initialisierten USB-Device-Controller
besitzen noch am USB-Protokollbetrieb teilnehmen.

## Anstecken

Das Anstecken ordnet ein virtuelles USB-Gerät einem virtuellen USB-Port zu.

Das Anstecken verändert ausschließlich die USB-Topologie und stellt noch keine
USB-Verbindung zum Host her.

## Abstecken

Das Abstecken entfernt die Zuordnung eines virtuellen USB-Gerätes zu einem
virtuellen USB-Port.

Nach dem Abstecken besitzt das Gerät keine Verbindung mehr zur virtuellen
USB-Topologie.

## USB Connect

USB Connect signalisiert dem virtuellen USB-Hostcontroller die Anwesenheit
eines virtuellen USB-Gerätes.

Der virtuelle USB-Device-Controller muss hierzu bereits initialisiert sein.

USB Connect setzt voraus, dass das Gerät an einen virtuellen USB-Port
angesteckt ist.

USB Connect startet selbst keine Enumeration, ermöglicht diese jedoch.

## USB Disconnect

USB Disconnect beendet die vom virtuellen USB-Device-Controller signalisierte
USB-Verbindung zum virtuellen USB-Hostcontroller.

Das virtuelle USB-Gerät bleibt dabei weiterhin Bestandteil der virtuellen
USB-Topologie und kann weiterhin an einem virtuellen USB-Port angesteckt sein.

## Enumeration

Die Enumeration beschreibt den durch den USB-Host initiierten Ablauf zur
Erkennung, Identifikation und Konfiguration eines neu verbundenen
USB-Gerätes.

Die Enumeration wird vom USB-Host nach Erkennen eines USB Connect initiiert und
umfasst unter anderem das Auslesen der USB-Deskriptoren, die Vergabe einer
USB-Adresse sowie die Konfiguration des USB-Gerätes.

# Kernkomponenten

Kernkomponenten sind die virtuellen Gegenstücke der grundlegenden
Hardwarekomponenten einer realen USB-Infrastruktur.

Sie bilden die Basis der virtuellen USB-Topologie und übernehmen innerhalb von
VirtUSB dieselben grundsätzlichen Aufgaben wie ihre realen Vorbilder. Die
konkrete Implementierung kann dabei von der realen Hardware abweichen.

## VirtUsbHcd

Ein **VirtUsbHcd** (Virtual USB Host Controller) ist das virtuelle Gegenstück zu
einem physischen USB-Hostcontroller.

Er stellt den virtuellen USB-Root-Hub (`VirtUsbRHub`) bereit und bildet die
Schnittstelle zwischen dem Hostbetriebssystem und der virtuellen USB-Topologie.

## VirtUsbRHub

Ein **VirtUsbRHub** (Virtual USB Root Hub) ist das virtuelle Gegenstück zum
Root-Hub eines physischen USB-Hostcontrollers.

Er ist direkt einem `VirtUsbHcd` zugeordnet und stellt Downstream-Ports bereit,
an die virtuelle USB-Geräte (`VirtUsbDev`) oder weitere virtuelle USB-Hubs
(`VirtUsbHub`) angeschlossen werden können.

## VirtUsbHub

Ein **VirtUsbHub** (Virtual USB Hub) ist das virtuelle Gegenstück zu einem
physischen USB-Hub.

Er erweitert die virtuelle USB-Topologie um zusätzliche Downstream-Ports, an
die virtuelle USB-Geräte (`VirtUsbDev`) oder weitere virtuelle USB-Hubs
(`VirtUsbHub`) angeschlossen werden können.

## VirtUsbDev

Ein **VirtUsbDev** (Virtual USB Device) ist das virtuelle Gegenstück zu einem
physischen USB-Gerät.

Es stellt gegenüber dem USB-Host das Verhalten eines entsprechenden
USB-Gerätes bereit und kann an einen virtuellen USB-Port angeschlossen werden.

---
---

# Projektdokumente

Dieses Kapitel definiert den Zweck der projektrelevanten Dokumente.

Jedes Projektdokument beantwortet genau eine Fragestellung.
Nachgelagerte Dokumente konkretisieren den Inhalt der darüberliegenden
Dokumente, ohne deren Abstraktionsebene zu verlassen.

## Systemübersicht

**Fragestellung:**

> Was ist VirtUSB?

Die Systemübersicht beschreibt den Zweck, den Umfang und die grundlegenden
Architekturprinzipien von VirtUSB.

Sie dient als Einstieg in das Projekt und soll bewusst kurz gehalten werden.
Detailbeschreibungen gehören ausschließlich in nachgelagerte
Architekturdokumente.

## Glossar

**Fragestellung:**

> Was bedeuten die im Projekt verwendeten Begriffe?

Das Glossar definiert die projektweit verwendete Terminologie sowie den Zweck
der Projektdokumente.

Es beschreibt keine Architektur oder Implementierungsdetails.

## High-Level Architecture

**Fragestellung:**

> Woraus besteht VirtUSB und wie hängen die Kernkomponenten zusammen?

Die High-Level Architecture konkretisiert die Systemübersicht.

Sie beschreibt den grundsätzlichen Aufbau von VirtUSB, die Beziehungen der
Kernkomponenten sowie deren grundlegende Verantwortlichkeiten.

Sie bildet die Grundlage für nachgelagerte Architekturdokumente.

Nachgelagerte Dokumente konkretisieren einzelne Teilbereiche der
High-Level Architecture, dürfen deren grundlegende Architekturentscheidungen
jedoch nicht widersprechen.
