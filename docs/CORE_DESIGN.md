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

---

## 3. Task-Based Processing Architecture: Abstraction and Orchestration
To manage the processing workflow effectively, especially in a potentially concurrent or sequential context, we introduce a layer of abstraction using interfaces and a central orchestrator.

### IProcessingTask & PhotoTask (Command Pattern / Task Abstraction)
The **IProcessingTask** interface defines a unit of work encapsulating the processing of an image region with a specific sequence of operations.

* **Responsibility:** Encapsulate the data (input tile, operations, factory), the execution logic (**execute**), and provide access to the result (**result**) and progress (**progress**).
* **Decoupling:** The interface abstracts the how of processing, allowing for different implementations (e.g., CPU-based, GPU-based in the future) or different types of tasks (e.g., loading, saving).
* **Implementation (PhotoTask):** A concrete implementation that applies a sequence of operations defined by **OperationDescriptors** to an **ImageRegion** using the static
**PipelineEngine::applyOperations method.** It receives an **std::shared_ptr<ImageRegion>** as input, processes it in-place, and stores the result internally.

### IProcessingBackend & PhotoEngine (Orchestrator / Facade)

The **IProcessingBackend** interface defines the contract for creating and submitting **IProcessingTasks**.

* **Responsibility**: Orchestrate the overall processing flow. This includes managing the state of the loaded image (**SourceManager**), maintaining the list of active operations, creating **IProcessingTasks** via **createTask**, and handling their execution via **submit**. It also manages committing the final result back to the source via **commitResult**.
* **Implementation (PhotoEngine):** The main concrete orchestrator. It uses the **SourceManager** to load images and provide initial data (**tiles**). It creates **PhotoTasks** via **createTask** by first calling **SourceManager::getTile** (which returns an **std::unique_ptr<ImageRegion>**), converting it to **std::shared_ptr<ImageRegion>**, and then passing it to **std::make_shared<PhotoTask>**. It executes tasks (synchronously in v1 via **submit**) and provides a method **commitResult** to write the processed tile (retrieved via **task->result()**) back to the **SourceManager** using **SourceManager::setTile**.

**Benefit:** This centralizes the high-level logic (when to process, what operations are active, how to handle results) away from the low-level pipeline execution.

---
## 4. Low-Level Pipeline Execution: Statelessness and Utility

The core logic for applying a sequence of operations to a data unit is encapsulated in a stateless utility located within the `operation` directory.

### OperationPipeline (Stateless Utility)
The `OperationPipeline` class (formerly `PipelineEngine`) is refactored into a stateless class containing only a static method (`apply` or `applyOperations`).

* **Responsibility:** Execute a sequence of operations on a given `ImageRegion`. It iterates through the `OperationDescriptor`s, uses the `OperationFactory` to create instances of the required operations, and executes them sequentially on the provided tile.

* **Decoupling:** It is independent of `SourceManager`, `PhotoEngine`, or any task management. It only knows about `ImageRegion`, `OperationDescriptor`, and `OperationFactory`.

* **Location:** This class resides in the `operation` folder, centralizing the low-level processing logic within the operation domain.

**Benefit:** High reusability and testability. It's a pure function-like component that performs the core processing step. It also allows for `[[nodiscard]]` attribute on its return value (`bool`) to ensure callers check for success/failure.

---
## 5. Operation Management: The Factory Patter
This pattern remains crucial for creating operation instances within the processing pipeline.

### Components

* **IOperation:** The base interface for all operations (e.g., BrightnessOperation, ContrastOperation). It defines a single **execute(ImageRegion& tile, const OperationDescriptor& descriptor)**    method.

**Benefit:** Polymorphism. The **PipelineEngine** can treat every operation the same way, regardless of whether it performs a simple brightness adjustment or a complex convolution.

* **OperationFactory:** This component is responsible for knowing how to construct concrete implementations of **IOperation** based on an **OperationType** defined in the **OperationDescriptor**.

**Benefit:** Adding a new operation (e.g., **SaturationOperation**) only requires defining the new class and registering it in the factory setup, without modifying the **PipelineEngine**'s core logic. This promotes high maintainability and scalability.