/* This file is part of Zanshin Todo.

   Copyright 2008-2010 Kevin Ottens <ervin@kde.org>

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

#include "actionlisteditorpage.h"

#include <KDE/Akonadi/ItemCreateJob>

#include <KDE/KConfigGroup>
#include <kdescendantsproxymodel.h>
#include <kmodelindexproxymapper.h>

#include <QtCore/QTimer>
#include <QtGui/QHeaderView>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QVBoxLayout>

#include "actionlistdelegate.h"
#include "categorymanager.h"
#include "globaldefs.h"
#include "todotreeview.h"
#include "todohelpers.h"

static const char *_z_defaultColumnStateCache = "AAAA/wAAAAAAAAABAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAvAAAAAFAQEAAQAAAAAAAAAAAAAAAGT/////AAAAgQAAAAAAAAAFAAABNgAAAAEAAAAAAAAAlAAAAAEAAAAAAAAAjQAAAAEAAAAAAAAAcgAAAAEAAAAAAAAAJwAAAAEAAAAA";

class GroupLabellingProxyModel : public QSortFilterProxyModel
{
public:
    GroupLabellingProxyModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (role!=Qt::DisplayRole || index.column()!=0) {
            return QSortFilterProxyModel::data(index, role);
        } else {
            int type = index.data(Zanshin::ItemTypeRole).toInt();

            if (type!=Zanshin::ProjectTodo
             && type!=Zanshin::Category) {
                return QSortFilterProxyModel::data(index, role);

            } else {
                QString display = QSortFilterProxyModel::data(index, role).toString();

                QModelIndex currentIndex = mapToSource(index.parent());
                type = currentIndex.data(Zanshin::ItemTypeRole).toInt();

                while (type==Zanshin::ProjectTodo
                    || type==Zanshin::Category) {
                    display = currentIndex.data().toString() + ": " + display;

                    currentIndex = currentIndex.parent();
                    type = currentIndex.data(Zanshin::ItemTypeRole).toInt();
                }

                return display;
            }
        }
    }
};

class GroupSortingProxyModel : public QSortFilterProxyModel
{
public:
    GroupSortingProxyModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const
    {
        int leftType = left.data(Zanshin::ItemTypeRole).toInt();
        int rightType = right.data(Zanshin::ItemTypeRole).toInt();

        return leftType==Zanshin::Inbox
            || (leftType==Zanshin::CategoryRoot && rightType!=Zanshin::Inbox)
            || (leftType==Zanshin::Collection && rightType!=Zanshin::Inbox)
            || (leftType==Zanshin::StandardTodo && rightType!=Zanshin::StandardTodo)
            || (leftType==Zanshin::ProjectTodo && rightType==Zanshin::Collection)
            || (leftType == rightType && QSortFilterProxyModel::lessThan(left, right));
    }
};

class TypeFilterProxyModel : public QSortFilterProxyModel
{
public:
    TypeFilterProxyModel(GroupSortingProxyModel *sorting, QObject *parent = 0)
        : QSortFilterProxyModel(parent), m_sorting(sorting)
    {
        setDynamicSortFilter(true);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        QModelIndex sourceChild = sourceModel()->index(sourceRow, 0, sourceParent);
        int type = sourceChild.data(Zanshin::ItemTypeRole).toInt();

        QSize sizeHint = sourceChild.data(Qt::SizeHintRole).toSize();

        return type!=Zanshin::Collection
            && type!=Zanshin::CategoryRoot
            && !sizeHint.isNull(); // SelectionProxyModel uses the null size for items we shouldn't display
    }

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
    {
        m_sorting->sort(column, order);
    }

private:
    GroupSortingProxyModel *m_sorting;
};


class ActionListEditorView : public TodoTreeView
{
public:
    ActionListEditorView(QWidget *parent = 0)
        : TodoTreeView(parent) { }

protected:
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
    {
        QModelIndex index = currentIndex();

        if (index.isValid() && modifiers==Qt::NoModifier) {
            QModelIndex newIndex;
            int newColumn = index.column();

            switch (cursorAction) {
            case MoveLeft:
                do {
                    newColumn--;
                } while (isColumnHidden(newColumn)
                      && newColumn>=0);
                break;

            case MoveRight:
                do {
                    newColumn++;
                } while (isColumnHidden(newColumn)
                      && newColumn<header()->count());
                break;

            default:
                return Akonadi::EntityTreeView::moveCursor(cursorAction, modifiers);
            }

            newIndex = index.sibling(index.row(), newColumn);

            if (newIndex.isValid()) {
                return newIndex;
            }
        }

        return Akonadi::EntityTreeView::moveCursor(cursorAction, modifiers);
    }
};

class ActionListEditorModel : public KDescendantsProxyModel
{
public:
    ActionListEditorModel(QObject *parent = 0)
        : KDescendantsProxyModel(parent)
    {
    }

    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
    {
        if (!sourceModel()) {
            return QAbstractProxyModel::dropMimeData(data, action, row, column, parent);
        }
        QModelIndex sourceParent = mapToSource(parent);
        return sourceModel()->dropMimeData(data, action, row, column, sourceParent);
    }
};

ActionListEditorPage::ActionListEditorPage(QAbstractItemModel *model,
                                           ModelStack *models,
                                           Zanshin::ApplicationMode mode,
                                           const QList<QAction*> &contextActions,
                                           QWidget *parent)
    : QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout(this));
    layout()->setContentsMargins(0, 0, 0, 0);

    m_treeView = new ActionListEditorView(this);

    GroupLabellingProxyModel *labelling = new GroupLabellingProxyModel(this);
    labelling->setSourceModel(model);

    GroupSortingProxyModel *sorting = new GroupSortingProxyModel(this);
    sorting->setSourceModel(labelling);

    ActionListEditorModel *descendants = new ActionListEditorModel(this);
    descendants->setSourceModel(sorting);

    TypeFilterProxyModel *filter = new TypeFilterProxyModel(sorting, this);
    filter->setSourceModel(descendants);

    m_treeView->setModel(filter);
    m_treeView->setItemDelegate(new ActionListDelegate(models, m_treeView));

    m_treeView->header()->setSortIndicatorShown(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);

    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setItemsExpandable(false);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setEditTriggers(m_treeView->editTriggers() | QAbstractItemView::DoubleClicked);

    connect(m_treeView->model(), SIGNAL(modelReset()),
            m_treeView, SLOT(expandAll()));
    connect(m_treeView->model(), SIGNAL(layoutChanged()),
            m_treeView, SLOT(expandAll()));
    connect(m_treeView->model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
            m_treeView, SLOT(expandAll()));

    layout()->addWidget(m_treeView);

    QTimer::singleShot(0, this, SLOT(onAutoHideColumns()));

    connect(m_treeView->header(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(onColumnsGeometryChanged()));

    m_treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_treeView->addActions(contextActions);
}

QItemSelectionModel *ActionListEditorPage::selectionModel() const
{
    return m_treeView->selectionModel();
}

void ActionListEditorPage::saveColumnsState(KConfigGroup &config, const QString &key) const
{
    config.writeEntry(key+"/Normal", m_normalStateCache.toBase64());
    config.writeEntry(key+"/NoCollection", m_noCollectionStateCache.toBase64());
}

void ActionListEditorPage::restoreColumnsState(const KConfigGroup &config, const QString &key)
{
    if (config.hasKey(key+"/Normal")) {
        m_normalStateCache = QByteArray::fromBase64(config.readEntry(key+"/Normal", QByteArray()));
    } else {
        m_normalStateCache = QByteArray::fromBase64(_z_defaultColumnStateCache);
    }

    if (config.hasKey(key+"/NoCollection")) {
        m_noCollectionStateCache = QByteArray::fromBase64(config.readEntry(key+"/NoCollection", QByteArray()));
    }

    if (!m_treeView->isColumnHidden(4)) {
        m_treeView->header()->restoreState(m_normalStateCache);
    } else {
        m_treeView->header()->restoreState(m_noCollectionStateCache);
    }
}

void ActionListEditorPage::addNewTodo(const QString &summary)
{
    if (summary.isEmpty()) return;

    QModelIndex current = m_treeView->selectionModel()->currentIndex();

    if (!current.isValid()) {
        kWarning() << "Oops, nothing selected in the list!";
        return;
    }

    int type = current.data(Zanshin::ItemTypeRole).toInt();

    while (current.isValid() && type==Zanshin::StandardTodo) {
        current = current.sibling(current.row()-1, current.column());
        type = current.data(Zanshin::ItemTypeRole).toInt();
    }

    Akonadi::Collection collection;
    QString parentUid;
    QString category;

    switch (type) {
    case Zanshin::StandardTodo:
        kFatal() << "Can't possibly happen!";
        break;

    case Zanshin::ProjectTodo:
        parentUid = current.data(Zanshin::UidRole).toString();
        collection = current.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>();
        break;

    case Zanshin::Collection:
        collection = current.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        break;

    case Zanshin::Category:
        category = current.data(Zanshin::CategoryPathRole).toString();
        // fallthrough
    case Zanshin::Inbox:
    case Zanshin::CategoryRoot:
        collection = m_defaultCollection;
        break;
    }

    TodoHelpers::addTodo(summary, parentUid, category, collection);
}

void ActionListEditorPage::removeCurrentTodo()
{
    QModelIndex current = m_treeView->selectionModel()->currentIndex();
    removeTodo(current);
}

void ActionListEditorPage::removeTodo(const QModelIndex &current)
{
    int type = current.data(Zanshin::ItemTypeRole).toInt();

    if (!current.isValid() || type!=Zanshin::StandardTodo) {
        return;
    }

    TodoHelpers::removeProject(this, current);
}

void ActionListEditorPage::dissociateTodo(const QModelIndex &current)
{
    int type = current.data(Zanshin::ItemTypeRole).toInt();

    if (!current.isValid()
     || type!=Zanshin::StandardTodo
     || m_mode==Zanshin::ProjectMode) {
        return;
    }

    for (int i=current.row(); i>=0; --i) {
        QModelIndex index = m_treeView->model()->index(i, 0);
        int type = index.data(Zanshin::ItemTypeRole).toInt();
        if (type==Zanshin::Category) {
            QString category = index.data(Zanshin::CategoryPathRole).toString();
            if (CategoryManager::instance().dissociateTodoFromCategory(current, category)) {
                break;
            }
        }
    }
}

Zanshin::ApplicationMode ActionListEditorPage::mode()
{
    return m_mode;
}

void ActionListEditorPage::onAutoHideColumns()
{
    switch (m_mode) {
    case Zanshin::ProjectMode:
        m_treeView->hideColumn(1);
        m_treeView->showColumn(2);
        break;
    case Zanshin::CategoriesMode:
        m_treeView->showColumn(1);
        m_treeView->hideColumn(2);
        break;
    }
}

void ActionListEditorPage::setCollectionColumnHidden(bool hidden)
{
    QByteArray state = hidden ? m_noCollectionStateCache : m_normalStateCache;

    if (!state.isEmpty()) {
        m_treeView->header()->restoreState(state);
    } else {
        m_treeView->setColumnHidden(4, hidden);
    }
}

void ActionListEditorPage::onColumnsGeometryChanged()
{
    if (!m_treeView->isColumnHidden(4)) {
        m_normalStateCache = m_treeView->header()->saveState();
    } else {
        m_noCollectionStateCache = m_treeView->header()->saveState();
    }
}

void ActionListEditorPage::setDefaultCollection(const Akonadi::Collection &collection)
{
    m_defaultCollection = collection;
}

bool ActionListEditorPage::selectSiblingIndex(const QModelIndex &index)
{
    QModelIndex sibling = m_treeView->indexBelow(index);
    if (!sibling.isValid()) {
        sibling = m_treeView->indexAbove(index);
    }
    if (sibling.isValid()) {
        m_treeView->selectionModel()->setCurrentIndex(sibling, QItemSelectionModel::Select|QItemSelectionModel::Rows);
        return true;
    }
    return false;
}

void ActionListEditorPage::selectFirstIndex()
{
    QTimer::singleShot(0, this, SLOT(onSelectFirstIndex()));
}

void ActionListEditorPage::onSelectFirstIndex()
{
    // Clear selection to avoid multiple selections when a widget is in edit mode.
    m_treeView->selectionModel()->clearSelection();
    QModelIndex root = m_treeView->model()->index(0, 0);
    if (root.isValid()) {
        m_treeView->selectionModel()->setCurrentIndex(root, QItemSelectionModel::Select|QItemSelectionModel::Rows);
    }
}
