# Development Guide

## Zweck

**Fragestellung:**

> Nach welchen Grundsätzen wird VirtUSB entwickelt?

Dieses Dokument beschreibt die projektweiten Entwicklungsrichtlinien für das
VirtUSB-Projekt.

Es legt die grundlegende Arbeitsweise, den Dokumentationsprozess sowie
projektweite Konventionen fest.

Es beschreibt keine Architektur einzelner Komponenten und ersetzt keine
Architektur- oder Anforderungsdokumente.

---

## Geltungsbereich

Dieser Leitfaden gilt für sämtliche Bestandteile des VirtUSB-Projekts.

Hierzu gehören insbesondere:

- Dokumentation
- Kernelmodul
- Userspace-Komponenten
- Bibliotheken
- Testprogramme
- Hilfswerkzeuge
- Buildsystem

---

## Allgemeine Entwicklungsgrundsätze

VirtUSB wird schrittweise entwickelt.

Architektur, Dokumentation und Implementierung werden nur soweit erstellt,
wie sie für den jeweils nächsten Entwicklungsschritt erforderlich sind.

Vorhandene Dokumente werden fortlaufend erweitert und verfeinert.
Architekturentscheidungen werden erst getroffen, wenn hierfür ausreichend
Informationen vorliegen.

Ideen, Alternativen und offene Fragestellungen werden zunächst im Sketchbook
gesammelt. Erst nach fachlicher Bewertung werden sie Bestandteil offizieller
Architekturdokumente.

Der grundsätzliche Entwicklungsprozess lautet:

```text
Idee
    ↓
Sketchbook
    ↓
Architekturdokument
    ↓
Implementierung
    ↓
ADR (falls erforderlich)
```

---

## Dokumentation

Die Dokumentation wird grundsätzlich in Markdown erstellt.

Mermaid-Diagramme dürfen verwendet werden, sofern sie zum besseren Verständnis
beitragen.

Die englische Dokumentation stellt die offizielle Projektdokumentation dar.

Während der Entwicklung können Dokumente zunächst auf Deutsch erstellt und
fachlich abgestimmt werden. Nach Abschluss der Review werden sie in die
englische Projektdokumentation übernommen.

Neue Dokumente entstehen grundsätzlich schrittweise.

Der empfohlene Arbeitsablauf lautet:

1. Dokumentstruktur erstellen.
2. Kapitel als Stichpunkte ausarbeiten.
3. Struktur fachlich prüfen.
4. Kapitel in Fließtext überführen.
5. Gesamtdokument überprüfen.

Referenzen auf andere Dokumente erfolgen über deren Titel.
Kopien oder lokale Duplikate externer Dokumente sollen vermieden werden.

Architekturentscheidungen werden mittels ADR dokumentiert, sofern deren
langfristige Nachvollziehbarkeit sinnvoll oder erforderlich ist.

---

## Buildsystem

VirtUSB verwendet CMake als projektweiten Einstiegspunkt für den Buildprozess.

Das Linux-Kernelmodul bleibt mit dem Linux-Kernel-Buildsystem kompatibel.

Die Projektstruktur soll eine spätere Integration mittels DKMS ermöglichen.

Das Projekt soll mindestens mit folgenden Compilern erstellt werden können:

- GCC
- Clang

Als primäre Entwicklungssysteme werden Debian und Arch Linux verwendet.

---

## Quellcode

Für Userspace-Komponenten wird Doxygen verwendet.

Für Kernelcode wird der Linux-übliche `kernel-doc`-Stil verwendet.

Dokumentation soll grundsätzlich nur an einer Stelle gepflegt werden.

Öffentliche Schnittstellen werden im Header dokumentiert.

Implementierungen wiederholen diese Dokumentation nicht.

Lokale Hilfsfunktionen werden nur dokumentiert, wenn deren Verhalten nicht
offensichtlich ist.

Der Quellcode soll klar strukturiert, gut lesbar und möglichst einfach
nachvollziehbar sein.

Implementierungen sollen bevorzugt einfache und robuste Lösungen verwenden.

---

## Testing

Tests sind ein wesentlicher Bestandteil der Entwicklung und sollen frühzeitig
erstellt sowie kontinuierlich erweitert werden.

Für Userspace-Komponenten wird GoogleTest als Unit-Test-Framework verwendet.

CTest dient als projektweiter Einstiegspunkt zur Ausführung von Tests und zur
Integration in den Buildprozess.

Kernelcode wird nach Möglichkeit mit den hierfür vorgesehenen
Linux-Kernel-Testwerkzeugen getestet.

Hierzu gehört insbesondere KUnit für Unit-Tests innerhalb des Kernels.

Userspace- und Kernelcode werden mit den jeweils dafür vorgesehenen
Testframeworks getestet.

Die Auswahl des Testframeworks richtet sich nach der jeweiligen
Ausführungsumgebung und orientiert sich an den etablierten Werkzeugen der
jeweiligen Plattform.

---

## Qualitätssicherung

VirtUSB soll kontinuierlich mit mehreren Compilern überprüft werden.

Zur statischen Codeanalyse werden insbesondere eingesetzt:

- cppcheck
- clang-tidy

Es wird ausschließlich projektinterner Quellcode analysiert.

Kernelquellcode und externe Bibliotheken sind hiervon ausgenommen.

Tests sollen frühzeitig erstellt und kontinuierlich erweitert werden.

---

## Versionsverwaltung

Änderungen werden in logisch zusammenhängenden Commits eingecheckt.

Größere Entwicklungsschritte werden durch Git-Tags gekennzeichnet.

Das Versionsschema lautet:

```
v<major>.<minor>.<patch>
```

---

## Erweiterbarkeit

Dieser Leitfaden beschreibt ausschließlich projektweite Grundsätze.

Weitere projektspezifische Richtlinien können bei Bedarf ergänzt werden.

Neue Regeln sollen nur aufgenommen werden, wenn sie projektweit gelten und
einen nachweisbaren Nutzen für die Entwicklung besitzen.
