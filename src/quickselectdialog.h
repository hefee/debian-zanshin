/* This file is part of Zanshin Todo.

   Copyright 2009 Kevin Ottens <ervin@kde.org>

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

#ifndef ZANSHIN_QUICKSELECTDIALOG_H
#define ZANSHIN_QUICKSELECTDIALOG_H

#include <KDE/KDialog>

#include "globaldefs.h"

class QAbstractItemModel;
class QLabel;
class QTreeView;
namespace Akonadi
{
    class Collection;
}
class KRecursiveFilterProxyModel;

class QuickSelectDialog : public KDialog
{
    Q_OBJECT

public:
    enum ActionType {
        MoveAction,
        CopyAction,
        JumpAction
    };

    QuickSelectDialog(QWidget *parent, QAbstractItemModel *model, Zanshin::ApplicationMode mode, ActionType action);

    QString selectedId() const;
    Zanshin::ItemType selectedType() const;
    Akonadi::Collection collection() const;
    QModelIndex selectedIndex() const;

private:
    QString categorySelectedId() const;
    QString projectSelectedId() const;

    QString pattern() const;
    void applyPattern(const QString &pattern);
    bool eventFilter(QObject *object, QEvent *ev);

    QLabel *m_label;
    QTreeView *m_tree;
    KRecursiveFilterProxyModel *m_filter;
    QAbstractItemModel *m_model;
    Zanshin::ApplicationMode m_mode;
};

#endif

