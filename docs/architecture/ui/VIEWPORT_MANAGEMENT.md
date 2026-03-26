# Viewport Management System - Complete Documentation

## Overview

This system provides optimal viewport management for all screen types:
- Standard desktop monitors (DPR 1.0)
- Retina/HiDPI displays (DPR 2.0+)
- Mobile devices (DPR 2.0-4.0)
- Large displays and projectors

**Key Principle: NEVER UPSCALE** — The system only downsamples, never upscales source images.

## File Structure

```
display/
├── viewport_config.h      - consteval constants and helper functions
├── viewport_manager.h     - Core viewport calculation class
├── viewport_manager.cpp   - Implementation
├── display_manager.h      - High-level display coordination
└── display_manager.cpp    - Implementation
```

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        QML Layer                                 │
│  ┌─────────────────┐                                            │
│  │ Screen (Qt)     │ ──► DPR, Screen Size                      │
│  │ devicePixelRatio│                                            │
│  └─────────────────┘                                            │
│           │                                                      │
│           ▼                                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   DisplayManager                         │   │
│  │  - Coordinates display state                             │   │
│  │  - Handles zoom/pan                                      │   │
│  │  - Manages rendering item                                │   │
│  │  - Requests downsampled images                           │   │
│  └─────────────────────────────────────────────────────────┘   │
│           │                                                      │
│           ▼                                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  ViewportManager                         │   │
│  │  - Detects screen characteristics                        │   │
│  │  - Calculates plafond (2K or 3K)                         │   │
│  │  - Calculates max downsample                             │   │
│  │  - Memory-aware texture sizing                           │   │
│  └─────────────────────────────────────────────────────────┘   │
│           │                                                      │
│           ▼                                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   DisplayConfig                          │   │
│  │  - consteval compile-time constants                      │   │
│  │  - MIN_DISPLAY_DIMENSION: 256px                          │   │
│  │  - PLAFOND_SMALL_SCREEN: 2048px (2K)                     │   │
│  │  - PLAFOND_LARGE_SCREEN: 3072px (3K)                     │   │
│  │  - DEFAULT_QUALITY_MARGIN: 1.25                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│           │                                                      │
│           ▼                                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                 Rendering Item                           │   │
│  │  (RHIImageItem, SGSImageItem, PaintedImageItem)         │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## Core Formula

```
MAX_DOWNSAMPLE = min(viewport_physical × quality_margin, PLAFOND)

DOWNSAMPLE = min(source_size, MAX_DOWNSAMPLE)  ← NEVER UPSCALE

Where:
- viewport_physical = viewport_logical × DPR
- PLAFOND = 2048px (screen < 2000px) or 3072px (screen ≥ 2000px)
- quality_margin = 1.0-2.0 (default 1.25)
```

## Plafond Calculation

The **plafond** (ceiling) is determined by physical screen size, not device type:

| Screen Physical Size | Plafond | Reasoning |
|---------------------|---------|-----------|
| < 2000px (width or height) | 2048px (2K) | Laptops, tablets, phones |
| ≥ 2000px (width or height) | 3072px (3K) | 4K monitors, 5K iMac, etc. |

This is screen-based, not device-category-based.

## Screen-Based Configuration

| Parameter | Small Screen (< 2000px) | Large Screen (≥ 2000px) |
|-----------|------------------------|------------------------|
| Plafond | 2048px | 3072px |
| Example Devices | 1080p laptop, tablet, phone | 4K monitor, 5K iMac |
| Max Texture | Limited by plafond | Limited by plafond |

## Example Calculations

### Example 1: 1080p Laptop (DPR 1.25, Screen 1920×1080)

```
Source: 4928x3264 (raw photo)
Viewport: 800x600 (logical)
DPR: 1.25
Screen: 1920x1080 (physical)

Step-by-step:
1. Physical viewport = 800×600 × 1.25 = 1000×750
2. Plafond = 2048px (screen < 2000px)
3. Max downsample = min(1000 × 1.25, 2048) = min(1250, 2048) = 1250px
4. Fit zoom = min(800/4928, 600/3264) = min(0.162, 0.184) = 0.162
5. Display size = 4928×3264 × 0.162 = 800×530
6. Downsample = min(4928, 1250) = 1250 → 1250×828 (proportional)
7. Texture memory = 1250×828×4 = ~4 MB

Result:
- Downsample: 1250x828
- Display: 800x530
- Fit zoom: 0.162
- Memory: ~4 MB (vs ~64 MB for full resolution)
```

### Example 2: 4K Monitor (DPR 1.0, Screen 3840×2160)

```
Source: 8000x6000 (8K photo)
Viewport: 1200x800 (logical)
DPR: 1.0
Screen: 3840x2160 (physical)

Step-by-step:
1. Physical viewport = 1200×800 × 1.0 = 1200×800
2. Plafond = 3072px (screen ≥ 2000px)
3. Max downsample = min(1200 × 1.25, 3072) = min(1500, 3072) = 1500px
4. Fit zoom = min(1200/8000, 800/6000) = min(0.15, 0.133) = 0.133
5. Display size = 8000×6000 × 0.133 = 1066×800
6. Downsample = min(8000, 1500) = 1500 → 1500×1125 (proportional)
7. Texture memory = 1500×1125×4 = ~7 MB

Result:
- Downsample: 1500x1125
- Display: 1066x800
- Fit zoom: 0.133
- Memory: ~7 MB
```

### Example 3: Small Image (No Downsample Needed)

```
Source: 800x600 (small photo)
Viewport: 800x600 (logical)
DPR: 1.25
Screen: 1920x1080 (physical)

Step-by-step:
1. Max downsample = 1250px
2. Source (800) < Max (1250)
3. needs_downsample = false
4. Downsample size = source size (unchanged)
5. GOLDEN RULE: Never upscale!

Result:
- Downsample: 800x600 (no change)
- Display: 800x600
- Fit zoom: 1.0
- Memory: ~2 MB
```

## Memory Comparison

| Source Size | Without Downsample | With Smart Downsample | Savings |
|-------------|-------------------|----------------------|---------|
| 4928×3264 | ~64 MB | ~4 MB | 94% |
| 8000×6000 | ~183 MB | ~7 MB | 96% |
| 12000×8000 | ~366 MB | ~18 MB | 95% |

## Quality Presets

| Preset | Quality Margin | Use Case |
|--------|---------------|----------|
| Performance | 1.0× | Preview, fast navigation |
| Balanced | 1.25× (default) | Normal editing |
| Quality | 1.5× | Detailed work, zooming |
| Maximum | 2.0× | Maximum zoom headroom |

Higher quality margin = more memory, but better zoom capability before pixelation.


## Key Design Decisions

1. **Screen-Based, Not Device-Based**: Plafond is determined by physical screen dimensions, not device category (mobile/desktop/etc.)

2. **Fixed at Startup**: MAX_DOWNSAMPLE is calculated once at app startup based on screen characteristics

3. **Never Upscale**: Source images smaller than MAX_DOWNSAMPLE are displayed at full resolution

4. **DPR-Aware**: Physical viewport size accounts for device pixel ratio

5. **Quality Margin**: Provides zoom headroom (1.25× = 25% zoom before pixelation)
