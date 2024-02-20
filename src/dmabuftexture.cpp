/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dmabuftexture.h"

#include "kwineglimagetexture.h"
#include "kwinglutils.h"

namespace KWin
{

    DmaBufTexture::DmaBufTexture(const std::shared_ptr<GLTexture>& texture, DmaBufAttributes &&attributes)
            : m_texture{texture.get()}
            , m_framebuffer(new KWin::GLFramebuffer(texture.get()))
            , m_attributes{std::move(attributes)}
    {
    }


    const DmaBufAttributes &DmaBufTexture::attributes() const
    {
        return m_attributes;
    }

    DmaBufTexture::~DmaBufTexture() = default;

    KWin::GLTexture *DmaBufTexture::texture() const
    {
        return m_texture.data();
    }

    KWin::GLFramebuffer *DmaBufTexture::framebuffer() const
    {
        return m_framebuffer.data();
    }

} // namespace KWin

