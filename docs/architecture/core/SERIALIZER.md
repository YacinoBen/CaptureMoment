# Serializer Module

This module handles the serialization and deserialization of image processing operations (`OperationDescriptor`) to and from persistent storage, currently using XMP metadata.

## Architecture

The serializer module follows a layered, dependency-injection-based design to ensure flexibility and testability.

### Core Interfaces

* **`IXmpProvider`**: Abstracts the low-level XMP packet I/O operations (read/write). Allows switching underlying XMP libraries (e.g., Exiv2, Adobe XMP Toolkit).
* **`IXmpPathStrategy`**: Defines how to determine the file path for the XMP metadata associated with a given source image path. Supports different storage strategies (sidecar, AppData, configurable).
* **`IFileSerializerWriter`**: Interface for writing a list of `OperationDescriptor`s to a file (e.g., XMP packet).
* **`IFileSerializerReader`**: Interface for reading a list of `OperationDescriptor`s from a file (e.g., XMP packet).

### Core Implementations

* **`Exiv2Provider`**: Concrete implementation of `IXmpProvider` using the Exiv2 library.
* **`SidecarXmpPathStrategy`**: Stores XMP files alongside the image (e.g., `image.jpg` -> `image.jpg.xmp`).
* **`AppDataXmpPathStrategy`**: Stores XMP files in a centralized application data directory (e.g., `~/.local/share/CaptureMoment/xmp_cache/`).
* **`ConfigurableXmpPathStrategy`**: Stores XMP files in a user-defined directory.
* **`FileSerializerWriter`**: Concrete implementation of `IFileSerializerWriter`. Uses `IXmpProvider` and `IXmpPathStrategy` to write operations to an XMP file.
* **`FileSerializerReader`**: Concrete implementation of `IFileSerializerReader`. Uses `IXmpProvider` and `IXmpPathStrategy` to read operations from an XMP file.

### High-Level Manager

* **`FileSerializerManager`**: Orchestrates the `FileSerializerWriter` and `FileSerializerReader`. Provides a unified interface (`saveToFile`, `loadFromFile`) for the `PhotoEngine` to interact with.

### Serialization Utilities

* **`OperationSerialization`**: A namespace containing utility functions (`serializeParameter`, `deserializeParameter`) for converting the `std::any` values within `OperationDescriptor::params` to/from string representations suitable for XMP storage, preserving type information.

### Initialization

* **`Exiv2Initializer`**: A utility class to ensure the Exiv2 library is initialized before any XMP operations are performed.

## Workflow

1.  **Saving Operations:**
    1.  `PhotoEngine` calls `FileSerializerManager::saveToFile(image_path, operations)`.
    2.  `FileSerializerManager` delegates to `FileSerializerWriter::saveToFile`.
    3.  `FileSerializerWriter` uses `IXmpPathStrategy::getXmpPathForImage` to determine the XMP file location.
    4.  `FileSerializerWriter` converts `OperationDescriptor`s to an XMP packet string (using `OperationSerialization`).
    5.  `FileSerializerWriter` uses `IXmpProvider::writeXmp` to write the packet to the determined file.

2.  **Loading Operations:**
    1.  `PhotoEngine` calls `FileSerializerManager::loadFromFile(image_path)`.
    2.  `FileSerializerManager` delegates to `FileSerializerReader::loadFromFile`.
    3.  `FileSerializerReader` uses `IXmpPathStrategy::getXmpPathForImage` to determine the XMP file location.
    4.  `FileSerializerReader` uses `IXmpProvider::readXmp` to read the XMP packet string.
    5.  `FileSerializerReader` parses the XMP packet back into `OperationDescriptor`s (using `OperationSerialization`).
    6.  The list of `OperationDescriptor`s is returned to `PhotoEngine`.

## Integration

The `FileSerializerManager` is created with the necessary dependencies (`FileSerializerWriter`, `FileSerializerReader`, which in turn depend on `IXmpProvider` and `IXmpPathStrategy`) and injected into the `PhotoEngine` constructor. `PhotoEngine` then uses its `FileSerializerManager` instance to save and load operations.

## Dependencies

*   **Exiv2**: For XMP packet parsing and writing.
*   **spdlog**: For logging.
*   **magic_enum**: For converting `OperationType` enum values to/from strings within the XMP packet.
*   