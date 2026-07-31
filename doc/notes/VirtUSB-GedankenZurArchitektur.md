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