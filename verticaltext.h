//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef VerticalText_h
#define VerticalText_h

#include <QString>
#include <QWidget>

class VerticalText : public QWidget
{
   public:
      VerticalText(QWidget * parent, const QString&, Qt::WFlags f = 0);
      ~VerticalText();

      void setText(const QString& text);
      QSize sizeHint() const;
      QSizePolicy sizePolicy () const;
      QSize minimumSizeHint() const;
   protected:
      void paintEvent ( QPaintEvent * event );

   private:
      QString m_labelText;
};

#endif
