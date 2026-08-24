#pragma once
#include "unordered_map"
#include "glm/glm.hpp"
#include "spatial-grid-coord.h"
#include "handles.h"

namespace nitro::renderer
{

    struct GridCell
    {
        std::vector<MeshInstanceHandle> meshInstances;
    };

    class SpatialGrid
    {

        std::unordered_map<GridCellCoord, GridCell, GridCellCoordHash> m_cells;
        float m_cellSize = 5.0f;

    public:
        SpatialGrid(float cellSize = 5.0f);
        ~SpatialGrid() = default;

        const GridCellCoord worldToCell(glm::vec3 pos);
        const std::vector<GridCellCoord> worldToCellRange(glm::vec3 min, glm::vec3 max);

        const std::vector<MeshInstanceHandle> getMeshInstances(const std::vector<GridCellCoord> &cellCoords);

        const std::vector<GridCellCoord> cellsInRadius(const GridCellCoord &center, int radius);
        void addMeshInstance(MeshInstanceHandle handle, const std::vector<GridCellCoord> &cellCoords);
        void removeMeshInstance(MeshInstanceHandle handle, const std::vector<GridCellCoord> &cellCoords);

        void debugPrint() const;
        GridCell *getCell(const GridCellCoord &coord);
    };
} // namespace nitro::renderer
