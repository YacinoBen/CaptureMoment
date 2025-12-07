# Core Architecture Design Principles
The CaptureMoment core library is designed with modularity, high performance, and future extensibility in mind, which is essential for a modern, tile-based image processing engine. This document outlines the key architectural decisions, focusing on how we separate data from behavior and use established design patterns.

---
## 1. Data-Centric Design: Plain Old Data (POD) / Value Types

A fundamental principle of this architecture is the segregation of data storage from processing logic. Core data structures are kept simple and easy to handle:


### Key Data Structures:

* **ImageRegion:** This structure represents a self-contained tile of pixel data (e.g., RGBA F32), along with its spatial coordinates.

**Why Value** Type? By making ImageRegion a simple container, it becomes highly efficient for memory management. It can be easily copied, moved, and passed between functions, minimizing complexity and supporting future integration with low-level processing frameworks (like Halide or CUDA) that thrive on raw, contiguous data buffers.

* **OperationDescriptor:** This structure holds all necessary parameters to define a single processing step (e.g., brightness value, contrast factor, enable state).

**Why Value Type?** It acts as a configuration snapshot. The PipelineEngine uses this descriptor to instruct the OperationFactory what to create and what parameters to apply, keeping the engine itself free from hard-coded operation logic.

---
## 2. Manager and Engine Separation

The core functionality is split into specialized components: Managers handle resources and external concerns, while the Engine handles orchestration.

### ISourceManager & SourceManager (Strategy Pattern)
The ISourceManager defines the contract for handling the image source namely, the crucial READ (getTile) and WRITE (setTile) operations.

* **Responsibility:** Encapsulate file I/O, caching, and physical pixel access.

* **Decoupling:** By using the ISourceManager interface, the core logic (the PipelineEngine) is entirely agnostic to the underlying I/O technology.

* **Implementation (SourceManager):** The concrete implementation uses industry-standard tools like OpenImageIO (OIIO) to handle diverse file formats and efficient caching via OIIO::ImageBuf and OIIO::ImageCache. This shields the rest of the application from OIIO's specific complexities.

### PipelineEngine (Orchestrator)

The PipelineEngine is the central control unit that governs the entire processing workflow.

* **Responsibility:** Implement the Tile Processing Loop:

**1. READ:** Requests a tile from the SourceManager via getTile().

**2. PROCESS:** Iterates through the list of OperationDescriptors, using the OperationFactory to execute each step.

**3. WRITE:** Persists the modified tile data back to the SourceManager via setTile().

**Benefit:** The engine focuses purely on the flow control and coordination, ensuring operations are applied in the correct sequence without needing to know how each operation works or how the data is loaded/saved.

---
## 3. Operation Management: The Factory Patter
To ensure the pipeline can be easily extended with new image filters (operations), we employ the Factory Design Pattern.

### Components

* **IOperation:** The base interface for all operations (e.g., BrightnessOperation, ContrastOperation). It defines a single execute(ImageRegion& tile, const OperationDescriptor& descriptor) method.

**Benefit:** Polymorphism. The PipelineEngine can treat every operation the same way, regardless of whether it performs a simple brightness adjustment or a complex convolution.

* **OperationFactory:** This component is responsible for knowing how to construct concrete implementations of IOperation based on an OperationType defined in the OperationDescriptor.

**Benefit:** Adding a new operation (e.g., SaturationOperation) only requires defining the new class and registering it in the factory setup (setupFactory()), without modifying the PipelineEngine's core logic. This promotes high maintainability and scalability.