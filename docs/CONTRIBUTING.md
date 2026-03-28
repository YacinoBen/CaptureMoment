# 🤝 Contributing to Capture Moment
Welcome and thank you for your interest in the open-source professional photo editing project, Capture Moment! Your contributions—whether code, documentation, bug reports, or feature ideas—are critical to achieving our goal of becoming a leading non-destructive photo workflow solution.

## 📦 Ways to Contribute

We encourage contributions across all facets of the project. Please choose the appropriate channel for your submission:

### 🐞 1. Reporting Issues and Bugs

If you find a malfunction or unexpected behavior, please file a comprehensive "Issue" on our GitHub repository. A high-quality bug report should include:

* **Version Details:** The application version or the specific commit hash.
* **Environment:** Your Operating System (Windows, macOS, Linux) and compiler used.
* **Steps to Reproduce:** A precise, step-by-step description of how to trigger the bug.
* **Expected vs. Observed:** A clear distinction between the desired outcome and the actual result. 

### ✨ 2. Proposing Features
New feature suggestions and architectural discussions must begin with an "Issue." This allows the core team and community to validate the design and approach, ensuring alignment with the project's long-term vision before significant development effort is expended.


### 💻 3. Submitting Code (Pull Requests)
For all code contributions (bug fixes, new features, or performance enhancements), please follow the established environment setup and submission process outlined below.


#### 🛠️ Setting Up Your Development Environment
Capture Moment relies on a complex, high-performance C++ toolchain (C++20/23, Halide, OIIO, Qt6, etc) requiring meticulous setup.

* ***Global Prerequisites:*** Ensure all global prerequisites (Git, CMake 3.21+, C++23 Compiler) are installed as detailed in the main guide.
* ***Build Guide:*** Refer to the [Build Guide](./BUILDING_MAIN.md) for platform-specific instructions (Windows, Linux, macOS) and the correct usage of Vcpkg (strongly recommended for robust dependency management).

####⚙️ Development CMake Configuration
To ensure all necessary checks and components are available during development, you must explicitly enable the UI and the test suite:
```powershell
# Example configuration for development (enables Desktop UI and Tests)
cmake --preset debug-vcpkg-msvc -Ddesktop_ui=ON -Dtests=ON 
# Compilation
cmake --build build/debug-vcpkg-msvc

```

### 📝 Code Standards and Pull Request Lifecycle

Adherence to these standards is mandatory for all code submissions to be accepted.

**1. Naming Conventions & Style**

* **Language Standard:** All source code must conform to the C++20/C++23 standard.

* **Formatting:** We enforce a consistent style guide (e.g., derived from LLVM or Google Style). It is required to use clang-format prior to submission.

* **Namespaces:** Core application logic must be encapsulated within the **CaptureMoment:: namespace**.

* **Testing:** All new features, performance-critical code changes, and complex bug fixes must be accompanied by unit tests.

---
## ⚖️ License Agreement

By submitting code, documentation, or any other material to the Capture Moment repository, you agree to license your contributions under the GNU General Public License v3 (GPLv3). This ensures that the project remains free and open for the entire community.
---

# 🛠️ How to Contribute in Development

## Core


## Core UI (Qt)

### Adding a New Operation (e.g., Vignette)

1.  **Define Operation Type:** Add `Vignette` to the `OperationType` enum in the Core.
2.  **Create Operation Implementation:** Define `OperationVignette` class in the Core that implements the new `execute(IWorkingImageHardware&, const OperationDescriptor&)` method and `appendToFusedPipeline` for pipeline fusion.
3.  **Register Operation:** Add `OperationVignette` to the `OperationRegistry::registerAll` function in the Core.
4.  **UI Integration:** Create a corresponding operation model in `ui_core` (`VignetteModel`) and expose it to QML via `OperationModelManager`.

### Adding a New Serialization Strategy

1.  **Define Strategy Interface:** (If not already done for the new strategy type).
2.  **Implement Strategy:** Create a new class implementing the required interface (e.g., `IXmpPathStrategy`) in the Core.
3.  **Configure in App Startup:** Inject the new strategy implementation into the `FileSerializerManager` and subsequently into the `SerializerController` during application setup.
4.  **No UI Code Change Required:** The `SerializerController` abstracts the core details, so the UI layer (QML) does not need modification.

## Desktop UI (Qt)
