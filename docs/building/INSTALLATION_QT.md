# ⚙️ Building Qt Guide
CaptureMoment requires Qt 6.10.2 with the qtdeclarative and qtshadertools modules. You can install it using the official graphical installer or via the command-line tool aqtinstall.

## List of modules for CaptureMoment
* Core
* Quick
* Gui
* GuiPrivate
* ShaderTools

## Method 1: Qt Maintenance Tool (Official Installer)

This is the easiest method if you are setting up a development environment on your own machine.

* [➡️ Go to the Official Qt Download page](BUILDING_HALIDE.md).
* Download the Qt Online Installer for your operating system.

## Method 2: aqtinstall (Command Line)
This method is recommended for CI/CD pipelines or if you prefer a fast, scriptable installation without needing a Qt account.

### Prerequisites
You need Python and pip installed on your system.

```bash
pip3 install aqtinstall
```

#### Installation Commands
##### On Linux (Ubuntu/Debian):

```bash
aqt install-qt linux desktop 6.10.2 --outputdir /opt/qt -m qtdeclarative -m qtshadertools
```

##### On macOS:

```bash
aqt install-qt mac desktop 6.10.2 --outputdir /opt/qt -m qtdeclarative -m qtshadertools
```

##### On Windows (PowerShell):

```bash
aqt install-qt windows desktop 6.10.2 --outputdir C:\Qt -m qtdeclarative -m qtshadertools
```

### Environment Configuration

if `CMAKE_PREFIX_PATH` don't know the Qt path. You need to set the follow methods: 

#### On Linux
Add this to your `~/.bashrc` (adjust the path if you used the Maintenance Tool):


```bash
export CMAKE_PREFIX_PATH="/opt/qt/6.10.2/gcc_64:${CMAKE_PREFIX_PATH}"
```

#### On macOS
Add this to your `~/.zshrc` (adjust the path if you used the Maintenance Tool, e.g., ~/Qt/6.10.2/macos):


```bash
export CMAKE_PREFIX_PATH="/opt/qt/6.10.2/macos:${CMAKE_PREFIX_PATH}"
```

#### On Windows
Add this to your System Environment Variables via the Windows Search bar ("Edit the system environment variables" -> Environment Variables -> User variables):

* **Variable name:** `CMAKE_PREFIX_PATH`
* **Variable value:** `C:\Qt\6.10.2\msvc2022_64` (Adjust `msvc2022_64` based on your Visual Studio version, e.g., `mingw_64` if you use MinGW).
* 
Restart your computer after setting these variables!