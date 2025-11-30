# Capture Moment

**An open-source, non-destructive photo editing application (GPLv3), cross-platform (desktop + mobile), featuring RAW processing, cataloging, and professional color science.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++](https://img.shields.io/badge/C++-20/23-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Qt](https://img.shields.io/badge/Qt-6-blue.svg)](https://doc.qt.io/qt-6/)

---

## 🎯 Goal

Capture Moment aims to become a **performant and modular open-source alternative** to software like **Adobe Lightroom** or **Capture One**. Designed for **professional and advanced amateur photographers**, it offers a **modern**, **non-destructive**, and **color-accurate** photo workflow.

---

## ✨ Features

- **Advanced RAW Processing**: Supports major RAW formats via **OpenImageIO**.
- **Non-Destructive Pipeline**: Adjustments saved in **XMP sidecar files**.
- **Smart Cataloging**: Indexing via **SQLite**, advanced search, collections, keywords.
- **Professional Color Management**: Via **OpenColorIO** (planned integration).
- **Camera Profiles**: Automatic color correction based on camera model (**DCP support planned**).
- **Modern UI**: Smooth and reactive interface with **QML**.
- **Cross-Platform**: Compatible **Windows, macOS, Linux**. Ready for **mobile (iOS/Android)**.
- **Performance**: Fast calculations via **Halide**, intelligent caching via **OIIO**.
- **Integrated Benchmarking**: Tools to measure and ensure performance.

---

## 🛠️ Technologies

- **Language**: C++20 / C++23
- **Build System**: CMake 3.21+
- **I/O & Cache**: [OpenImageIO](https://openimageio.readthedocs.io/)
- **Processing**: [Halide](https://halide-lang.org/)
- **UI**: [Qt 6 Quick/QML](https://doc.qt.io/qt-6/qtquick-index.html)
- **Color Management**: [OpenColorIO](https://opencolorio.readthedocs.io/)
- **Cataloging**: SQLite
- **Serialization**: XMP (sidecar), JSON

---

## 📋 Technical Specifications

For a detailed view of the architecture, technical choices, roadmap, and planned features, see the [Technical Specifications Document](./docs/TechnicalSpecs.md) (link to be created or integrated).

---

## 🚀 Installation & Build

For detailed build instructions for each platform and using various package managers, see the [Build Guide](./docs/Build.md).

---

## 🤝 Contributing

Contributions are welcome! Please read the [Contribution Guide](./docs/CONTRIBUTING.md) (link to be created) for details.

---

## 📄 License

This project is distributed under the [GNU General Public License v3 (GPLv3)](./LICENSE).

**Redistribution Conditions:**
- Any redistribution (modified or not) must be under the same GPLv3 license.
- The name **CaptureMoment** and credits to the original author (Kenza ABDELGUERFI) must be preserved.
- It is forbidden to redistribute this software (or a modified version) under a name highly similar without explicit permission from the original author.
- Redistributions must include a link to the official repository and a clear mention of the source code origin.

See the [LICENSE](./LICENSE) file for more details.

---

> **Capture Moment** - *A photographic moment, captured freely.*