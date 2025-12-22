# 🏗️ Architecture QML - CaptureMoment Desktop

## 🎯 The main structures

qt/desktop/qml/

    ├── CaptureMoment/
    │   ├── DesktopMain.qml                    # Main entry point
    │   │
    │   ├── views/                             # Complete application views
    │   │   ├── EditorView.qml                 # Main editor layout
    │   │   └── WelcomeView.qml                # Splash/welcome screen
    │   │
    │   ├── layout/                            # Layout components
    │   │   ├── EditorLayout.qml               # 4-panel layout (left/center/right/bottom)
    │   │   ├── LeftPanel.qml                  # Tools & history
    │   │   ├── CenterPanel.qml                # Viewport/scene
    │   │   ├── RightPanel.qml                 # Operations
    │   │   └── BottomPanel.qml                # Gallery/albums
    │   │
    │   ├── panels/                            # Operation panels (collapsible)
    │   │   ├── base/
    │   │   │   ├── CollapsiblePanel.qml       # Reusable collapsible container
    │   │   │   └── PanelHeader.qml            # Panel title with collapse button
    │   │   ├── TonePanel.qml                  # Brightness, Contrast, Exposure...
    │   │   ├── ColorPanel.qml                 # Saturation, Vibrance, Hue...
    │   │   ├── DetailPanel.qml                # Sharpness, Noise Reduction...
    │   │   └── EffectsPanel.qml               # Vignette, Grain...
    │   │
    │   ├── controls/                          # Reusable controls
    │   │   ├── SliderControl.qml              # Label + Slider + SpinBox
    │   │   └── DropdownControl.qml            # Label + ComboBox
    │   │
    │   ├── operations/                        # Individual operation widgets
    │   │   ├── BrightnessOperation.qml
    │   │   ├── ContrastOperation.qml
    │   │   ├── ExposureOperation.qml
    │   │   └── SaturationOperation.qml
    │   │
    │   └── styles/                            # Theme & styling
    │       └── Icons.qml                      # Icon constants
    │   └── app/                            # App
    │       └── AppMenuBar.qml                      # menuBar
    │   └── display/                            # Display
    │       └── DisplayArea.qml                       
---

##  Design Principles

### 1. Separation of Responsibilities

* **Views:** High-level orchestration
* **Layout:** Spatial structure
* **Panels:** Thematic operation groups
* **Controls:** Basic reusable components
* **Operations:** Business-specific widgets

### 3. Reusability

* **Generic base:** CollapsiblePanel, SliderControl
* **Specialization:** TonePanel extends CollapsiblePanel

##  Hierarchy of Components

    └── CaptureMoment/
    └── EditorView
        └── EditorLayout
              ├── LeftPanel
              │     ├── ToolsSection
              │     └── HistorySection
              │
              ├── CenterPanel
              │     └── QMLPaintedImageItem
              │
              ├── RightPanel (ScrollView)
              │     ├── TonePanel
              │     │     ├── BrightnessOperation
              │     │     ├── ContrastOperation
              │     │     └── ExposureOperation
              │     │
              │     ├── ColorPanel
              │     │     ├── SaturationOperation
              │     │     └── VibranceOperation
              │     │
              │     └── DetailPanel
              │           └── SharpnessOperation
              │
              └── BottomPanel
                    └── GalleryView


# 🛠️ How to Contribute
## Adding a New Operation
Example: Contrast (after Brightness)

Create C++ Class ContrastModel

* **Properties:** value, minimum, maximum
* **Method:** setValue(real value)
* **Signam:** valueChanged(real newValue)
* Similarstructure to BrightnessModel


* Register in QML via the class QmlContextSetup
* context->setContextProperty("contrastControl", m_contrast_model.get())
* Create ContrastOperation.qml in /operations
* Pattern: SliderControl bound to C++ model
* One SliderControl per operation
* Call contrastControl.setValue() on slider change
* Add ContrastOperation to TonePanel.qml

## Adding a New Panel (e.g., ColorPanel)

* Create ColorPanel.qml in /panels
* Extends CollapsiblePanel
* Set a title
* Add multiple operations in contentItem
* Add operations inside (SaturationOperation, VibranceOperation, etc.)
* Create each operation widget
* Register C++ models
* Add ColorPanel to OperationsView.qml

### Adding a New Control

* Create CustomControl.qml in /controls
* Design reusable component
* Use in multiple operations if possible
* Use in operations/panels
* Import via import CaptureMoment.desktop
