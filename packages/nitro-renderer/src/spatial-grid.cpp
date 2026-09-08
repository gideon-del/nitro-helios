#include "nitro-renderer/spatial-grid.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace nitro::renderer
{
    SpatialGrid::SpatialGrid(float cellSize) : m_cellSize(cellSize) {};
    const GridCellCoord SpatialGrid::worldToCell(glm::vec3 pos)
    {

        return {
            static_cast<int32_t>(std::floor(pos.x / m_cellSize)),
            static_cast<int32_t>(std::floor(pos.z / m_cellSize)),
        };
    };

    const std::vector<GridCellCoord> SpatialGrid::worldToCellRange(glm::vec3 a, glm::vec3 b)
    {

        auto min = glm::min(a, b);
        auto max = glm::max(a, b);
        auto minCoord = worldToCell(min);
        auto maxCoord = worldToCell(max);

        std::vector<GridCellCoord> cellRange;
        for (int x = minCoord.x; x <= maxCoord.x; ++x)
        {
            for (int z = minCoord.z; z <= maxCoord.z; ++z)
            {
                cellRange.push_back({x, z});
            }
        }

        return cellRange;
    }

    const std::vector<GridCellCoord> SpatialGrid::cellsInRadius(const GridCellCoord &center, int radius)
    {
        std::vector<GridCellCoord> cells;
        cells.reserve((2 * radius + 1) * (2 * radius + 1));
        for (int dx = -radius; dx <= radius; dx++)
        {
            for (int dz = -radius; dz <= radius; dz++)
            {
                cells.push_back({center.x + dx, center.z + dz});
            }
        }

        return cells;
    }

    void SpatialGrid::addEntity(EntityHandle handle, const std::vector<GridCellCoord> &cellCoords)
    {

        for (auto &coord : cellCoords)
        {
            m_cells[coord].entities.push_back(handle);
        }
    }

    void SpatialGrid::removeEntity(EntityHandle handle, const std::vector<GridCellCoord> &cellCoords)
    {
        for (auto &coord : cellCoords)
        {
            auto it = m_cells.find(coord);

            if (it == m_cells.end())
                continue;

            auto &cell = it->second;

            cell.entities.erase(
                std::remove(
                    cell.entities.begin(),
                    cell.entities.end(),
                    handle),
                cell.entities.end());

            if (cell.entities.empty())
                m_cells.erase(it);
        }
    };

    GridCell *SpatialGrid::getCell(const GridCellCoord &coord)
    {
        auto it = m_cells.find(coord);

        if (it == m_cells.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    void SpatialGrid::debugPrint() const
    {
        for (const auto &[coord, cell] : m_cells)
        {
            std::cout
                << "Cell (" << coord.x << ", " << coord.z << ")"
                << " -> " << cell.entities.size()
                << " instances\n";
        }
    }

    const std::vector<EntityHandle> SpatialGrid::getEntities(const std::vector<GridCellCoord> &cellCoords)
    {

        std::unordered_set<EntityHandle, EntityHandleHash> handles;
        for (auto &coord : cellCoords)
        {
            auto cell = getCell(coord);
            if (!cell)
                continue;

            for (auto &entityHandle : cell->entities)
            {
                if (!handles.count(entityHandle))
                {
                    handles.insert(entityHandle);
                }
            }
        }

        std::vector<EntityHandle> instanceHandles(handles.begin(), handles.end());
        return instanceHandles;
    }
    void SpatialGrid::clear()
    {
        m_cells.clear();
    }
} // namespace nitro::renderer
