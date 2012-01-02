/* This file is part of Zanshin Todo.

   Copyright 2010 Mario Bensi <mbensi@ipsquad.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/

#include "actionlistcompleterview.h"

#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>

#include "actionlistcombobox.h"

ActionListCompleterView::ActionListCompleterView(ActionListComboBox *parent)
    : QListView(parent),
      m_combo(parent)
{
}

void ActionListCompleterView::mouseReleaseEvent(QMouseEvent *e)
{
    QModelIndex index = indexAt(e->pos());
    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid()) {
        return;
    }
    Qt::CheckState state = (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked
                            ? Qt::Unchecked : Qt::Checked);
    model()->setData(index, state, Qt::CheckStateRole);
    hide();
}

void ActionListCompleterView::resizeEvent(QResizeEvent *e)
{
    QListView::resizeEvent(e);

    static bool forceGeometry = false;

    if (!forceGeometry) {
        forceGeometry = true;
        QRect geo = m_combo->finalizePopupGeometry(geometry());
        setGeometry(geo);
        forceGeometry = false;
    }
}
