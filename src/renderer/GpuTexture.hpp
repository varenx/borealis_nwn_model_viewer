/*
 * This file is part of Borealis NWN Model Viewer.
 * Copyright (C) 2025-2026 Varenx
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <borealis/nwn/mdl/Mdl.hpp>
#include <cstdint>

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    class GpuTexture
    {
    public:
        GpuTexture() = default;
        ~GpuTexture();

        // Non-copyable
        GpuTexture(const GpuTexture&) = delete;
        GpuTexture& operator=(const GpuTexture&) = delete;

        // Movable
        GpuTexture(GpuTexture&& other) noexcept;
        GpuTexture& operator=(GpuTexture&& other) noexcept;

        void upload(const mdl::TextureData& texture);
        void bind(int unit = 0) const;
        void unbind() const;

        bool isValid() const
        {
            return m_TextureId != 0;
        }
        uint32_t id() const
        {
            return m_TextureId;
        }

    private:
        uint32_t m_TextureId{0};
        uint32_t m_Width{0};
        uint32_t m_Height{0};
    };

} // namespace nwn
