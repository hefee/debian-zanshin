/* This file is part of Zanshin Todo.

   Copyright 2008-2010 Kevin Ottens <ervin@kde.org>
   Copyright 2008, 2009 Mario Bensi <nef@ipsquad.net>

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

#ifndef ZANSHIN_TODONODE_H
#define ZANSHIN_TODONODE_H

#include <QtCore/QChar>
#include <QtCore/QPersistentModelIndex>
#include <QtCore/QVariant>

class TodoNode
{
public:
    explicit TodoNode(const QModelIndex &rowSourceIndex, TodoNode *parent = 0);
    explicit TodoNode(TodoNode *parent = 0);
    ~TodoNode();

    TodoNode *parent() const;
    void setParent(TodoNode *parent);

    QList<TodoNode*> children() const;

    QModelIndex rowSourceIndex() const;

    QVariant data(int column, int role) const;
    void setData(const QVariant &value, int column, int role);
    void setRowData(const QVariant &value, int role);

    Qt::ItemFlags flags(int column) const;
    void setFlags(Qt::ItemFlags flags);

private:
    void init();

    TodoNode *m_parent;
    QList<TodoNode*> m_children;

    QPersistentModelIndex m_rowSourceIndex;
    QHash< QPair<int, int>, QVariant> m_data;
    Qt::ItemFlags m_flags;
};

#endif

