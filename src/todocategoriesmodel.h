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

#ifndef ZANSHIN_TODOCATEGORIESMODEL_H
#define ZANSHIN_TODOCATEGORIESMODEL_H

#include <QtGui/QAbstractProxyModel>

#include "todonode.h"
#include "todoproxymodelbase.h"

class TodoCategoriesModel : public TodoProxyModelBase
{
    Q_OBJECT

public:
    TodoCategoriesModel(QObject *parent = 0);
    virtual ~TodoCategoriesModel();

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private slots:
    void onSourceDataChanged(const QModelIndex &begin, const QModelIndex &end);
    void onSourceInsertRows(const QModelIndex &sourceIndex, int begin, int end);
    void onSourceRemoveRows(const QModelIndex &sourceIndex, int begin, int end);
    void createCategoryNode(const QString &categoryPath);
    void removeCategoryNode(const QString &categoryPath);
    void renameCategoryNode(const QString &oldCategoryPath, const QString &newCategoryPath);
    void moveCategoryNode(const QString &oldCategoryPath, const QString &newCategoryPath);

private:
    virtual void init();
    virtual TodoNode *createInbox() const;
    virtual void resetInternalData();

    TodoNode *m_categoryRootNode;
    QMap<QString, TodoNode*> m_categoryMap;
};

#endif

