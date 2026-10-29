# Render Graph Rewrite: Before / After

## Conditions

- Resolution: 3354×2104 (window), Vulkan via MoltenVK, M1 Max
- Scene: Sponza,
- Enabled: bloom, auto exposure (ACES); colour grading off
- Frame time: average of \_\_\_ frames from the GPU timer

## Results

| Metric           | Before   | After | Change |
| ---------------- | -------- | ----- | ------ |
| Frame time (avg) | 3.50 ms  |       |        |
| Barrier calls    | 123      |       |        |
| Image barriers   | 107      |       |        |
| Buffer barriers  | 16       |       |        |
| Texture memory   | 597 MiB  |       |        |
| Heap memory      | 1.26 GiB |       |        |
| Blit encoders    | 63       |       |        |

## Notes

- ~20 viewport-sized transient textures (16.12 MiB each) flagged "Temporary Resource". Aliasing target.
- Two unused heaps (256 + 128 MiB) are VMA blocks reserved but empty.
