/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include "kwin_export.h"
#include "core/dmabufattributes.h"
#include <QScopedPointer>
#include <memory>

namespace KWin
{
    class GLFramebuffer;
    class GLTexture;

    class KWIN_EXPORT DmaBufTexture
    {
    public:
        explicit DmaBufTexture(const std::shared_ptr<GLTexture>& texture, DmaBufAttributes &&attributes);
        virtual ~DmaBufTexture();

//        virtual quint32 stride() const = 0;
//        virtual int fd() const = 0;
        const DmaBufAttributes& attributes() const;
        KWin::GLTexture *texture() const;
        KWin::GLFramebuffer *framebuffer() const;

    protected:
        QScopedPointer<KWin::GLTexture> m_texture;
        QScopedPointer<KWin::GLFramebuffer> m_framebuffer;
        DmaBufAttributes m_attributes;
    };

}

