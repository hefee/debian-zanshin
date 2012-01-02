/* This file is part of Zanshin Todo.

   Copyright 2008-2011 Mario Bensi <nef@ipsquad.net>

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

#ifndef ZANSHIN_TODOMETADATAMODEL_H
#define ZANSHIN_TODOMETADATAMODEL_H

#include "kidentityproxymodel.h"
#include <QtCore/QPersistentModelIndex>

#include <KDE/KCalCore/Todo>

#include "globaldefs.h"

namespace Akonadi
{
    class Item;
}

class TodoMetadataModel : public KIdentityProxyModel
{
    Q_OBJECT

public:
    TodoMetadataModel(QObject *parent = 0);
    virtual ~TodoMetadataModel();

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    virtual void setSourceModel(QAbstractItemModel *model);

private slots:
    void onSourceInsertRows(const QModelIndex &sourceIndex, int begin, int end);
    void onSourceRemoveRows(const QModelIndex &sourceIndex, int begin, int end);
    void onSourceDataChanged(const QModelIndex &begin, const QModelIndex &end);
    void onModelReset();

private:
    KCalCore::Todo::Ptr todoFromIndex(const QModelIndex &index) const;
    KCalCore::Todo::Ptr todoFromItem(const Akonadi::Item &item) const;

    void cleanupDataForSourceIndex(const QModelIndex &index);
    Zanshin::ItemType itemTypeFromItem(const Akonadi::Item &item) const;
    QString uidFromItem(const Akonadi::Item &item) const;
    QString relatedUidFromItem(const Akonadi::Item &item) const;
    QStringList ancestorsUidFromItem(const Akonadi::Item &item) const;
    QStringList ancestorsCategoriesFromItem(const Akonadi::Item &item) const;
    QStringList categoriesFromItem(const Akonadi::Item &item) const;
    QStringList childUidsFromItem(const Akonadi::Item &item) const;
    QModelIndexList childIndexesFromIndex(const QModelIndex &index) const;

    QHash<QString, QString> m_parentMap;
    QHash<QString, QStringList> m_childrenMap;
    QHash<QString, QPersistentModelIndex> m_indexMap;
};

#endif

