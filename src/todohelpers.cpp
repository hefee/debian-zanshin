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

#include "todohelpers.h"

#include <KDE/Akonadi/Collection>
#include <KDE/Akonadi/Item>
#include <KDE/Akonadi/ItemCreateJob>
#include <KDE/Akonadi/ItemDeleteJob>
#include <KDE/Akonadi/ItemFetchJob>
#include <KDE/Akonadi/ItemFetchScope>
#include <KDE/Akonadi/ItemModifyJob>
#include <KDE/Akonadi/ItemMoveJob>
#include <KDE/Akonadi/EntityTreeModel>
#include <KDE/Akonadi/TransactionSequence>
#include <KDE/KCalCore/Todo>
#include <KDE/KLocale>
#include <KDE/KMessageBox>

#include "categorymanager.h"
#include "globaldefs.h"

Akonadi::Item TodoHelpers::fetchFullItem(const Akonadi::Item &item)
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(item);
    Akonadi::ItemFetchScope scope;
    scope.setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    scope.fetchFullPayload();
    job->setFetchScope(scope);

    if ( !job->exec() ) {
        return Akonadi::Item();
    }

    Q_ASSERT(job->items().size()==1);

    return job->items().first();
}

void TodoHelpers::addTodo(const QString &summary, const QString &parentUid, const QString &category, const Akonadi::Collection &collection)
{
    if (!(collection.rights() & Akonadi::Collection::CanCreateItem)) {
        return;
    }

    KCalCore::Todo::Ptr todo(new KCalCore::Todo);
    todo->setSummary(summary);
    if (!parentUid.isEmpty()) {
        todo->setRelatedTo(parentUid);
    }
    if (!category.isEmpty()) {
        todo->setCategories(category);
    }

    Akonadi::Item item;
    item.setMimeType("application/x-vnd.akonadi.calendar.todo");
    item.setPayload<KCalCore::Todo::Ptr>(todo);

    new Akonadi::ItemCreateJob(item, collection);
}

void TodoHelpers::addProject(const QString &summary, const Akonadi::Collection &collection)
{
    if (!(collection.rights() & Akonadi::Collection::CanCreateItem)) {
        return;
    }

    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    todo->setSummary(summary);
    todo->addComment("X-Zanshin-Project");

    Akonadi::Item item;
    item.setMimeType("application/x-vnd.akonadi.calendar.todo");
    item.setPayload<KCalCore::Todo::Ptr>(todo);

    Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob(item, collection);
    job->start();
}

void TodoHelpers::addProject(const QString &summary, const QModelIndex &parentItem)
{
    // We use ParentCollectionRole instead of Akonadi::Item::parentCollection() because the
    // information about the rights is not valid on retrieved items.
    Akonadi::Collection collection = parentItem.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>();
    if (!(collection.rights() & Akonadi::Collection::CanCreateItem)) {
        return;
    }

    Akonadi::Item parentProject = parentItem.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();

    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    todo->setSummary(summary);
    todo->addComment("X-Zanshin-Project");

    KCalCore::Todo::Ptr parentTodo = parentProject.payload<KCalCore::Todo::Ptr>();
    todo->setRelatedTo(parentTodo->uid());

    Akonadi::Item item;
    item.setMimeType("application/x-vnd.akonadi.calendar.todo");
    item.setPayload<KCalCore::Todo::Ptr>(todo);

    Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob(item, collection);
    job->start();
}

void removeCurrentTodo(const QModelIndex &project, QModelIndexList children, Akonadi::TransactionSequence *sequence)
{
    // We use ParentCollectionRole instead of Akonadi::Item::parentCollection() because the
    // information about the rights is not valid on retrieved items.
    Akonadi::Collection collection = project.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>();
    if (!(collection.rights() & Akonadi::Collection::CanDeleteItem)) {
        return;
    }

    foreach (QModelIndex child, children) {
        QModelIndexList childList = child.data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
        removeCurrentTodo(child, childList, sequence);
    }

    Akonadi::Item item = project.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    new Akonadi::ItemDeleteJob(item, sequence);
}

bool TodoHelpers::removeProject(QWidget *parent, const QModelIndex &project)
{
    QModelIndexList projects;
    projects << project;
    return removeProjects(parent, projects);
}

bool TodoHelpers::removeProjects(QWidget *parent, const QModelIndexList &projects)
{
    if (projects.isEmpty()) {
        return false;
    }

    bool canRemove = true;
    QString summary;
    if (projects.size() > 1) {
        QStringList projectList;
        foreach (QModelIndex project, projects) {
            projectList << project.data().toString();
        }
        summary = projectList.join(", ");
    } else {
        QModelIndexList children = projects[0].data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
        if (!children.isEmpty()) {
            summary = projects[0].data().toString();
        }
    }

    if (!summary.isEmpty()) {
        QString title;
        QString text;

        if (projects.size() > 1) {
            title = i18n("Delete Projects");
            text = i18n("Do you really want to delete the projects '%1', with all its actions?", summary);
        } else {
            title = i18n("Delete Project");
            text = i18n("Do you really want to delete the project '%1', with all its actions?", summary);
        }


        int button = KMessageBox::questionYesNo(parent, text, title);
        canRemove = (button==KMessageBox::Yes);
    }

    if (!canRemove) return false;

    Akonadi::TransactionSequence *sequence = new Akonadi::TransactionSequence();
    foreach (QModelIndex project, projects) {

        // We use ParentCollectionRole instead of Akonadi::Item::parentCollection() because the
        // information about the rights is not valid on retrieved items.
        Akonadi::Collection collection = project.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>();
        if (!(collection.rights() & Akonadi::Collection::CanDeleteItem)) {
            continue;
        }

        QModelIndexList children = project.data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
        removeCurrentTodo(project, children, sequence);
    }
    sequence->start();
    return true;
}

static Akonadi::Item::List collectChildItemsRecHelper(const Akonadi::Item &item, const Akonadi::Item::List &items)
{
    Akonadi::Item::List result;

    Akonadi::Item::List itemsToProcess = items;
    Q_ASSERT(item.isValid() && item.hasPayload<KCalCore::Todo::Ptr>());
    KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();

    for (Akonadi::Item::List::Iterator it = itemsToProcess.begin();
         it!=itemsToProcess.end(); ++it) {
        Akonadi::Item currentItem = *it;

        if (!currentItem.hasPayload<KCalCore::Todo::Ptr>()
         || currentItem == item) {
            it = itemsToProcess.erase(it);
            --it;

        } else {
           KCalCore::Todo::Ptr currentTodo = currentItem.payload<KCalCore::Todo::Ptr>();

            if (currentTodo->relatedTo()==todo->uid()) {
                it = itemsToProcess.erase(it);
                --it;

                result << currentItem;
                result+= collectChildItemsRecHelper(currentItem, itemsToProcess);
            }
        }
    }

    return result;
}

static Akonadi::Item::List collectChildItems(const Akonadi::Item &item)
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(item.parentCollection());
    Akonadi::ItemFetchScope scope;
    scope.setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    scope.fetchFullPayload();
    job->setFetchScope(scope);

    if (!job->exec()) {
        return Akonadi::Item::List();
    }

    return collectChildItemsRecHelper(item, job->items());
}

bool TodoHelpers::moveTodoToProject(const QModelIndex &index, const QString &parentUid, const Zanshin::ItemType parentType, const Akonadi::Collection &parentCollection)
{
    Zanshin::ItemType itemType = (Zanshin::ItemType)index.data(Zanshin::ItemTypeRole).toInt();
    const Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();

    if (!todo) {
        return false;
    }

    if ((itemType == Zanshin::StandardTodo && parentType == Zanshin::StandardTodo)
     || (itemType == Zanshin::ProjectTodo && parentType == Zanshin::StandardTodo)
     || (itemType == Zanshin::Collection && parentType == Zanshin::ProjectTodo)
     || (itemType == Zanshin::Collection && parentType == Zanshin::StandardTodo)) {
         return false;
    }

    return moveTodoToProject(item, parentUid, parentType, parentCollection);
}

bool TodoHelpers::moveTodoToProject(const Akonadi::Item &item, const QString &parentUid, const Zanshin::ItemType parentType, const Akonadi::Collection &parentCollection)
{
    if (!(parentCollection.rights() & Akonadi::Collection::CanCreateItem)) {
        return false;
    }

    KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();

    if (!todo) {
        return false;
    }

    if ((!parentUid.isEmpty() && todo->relatedTo()==parentUid)
     || parentType == Zanshin::StandardTodo) {
        return false;
    }


    if (parentType == Zanshin::Inbox || parentType == Zanshin::Collection) {
        todo->setRelatedTo("");
    } else {
        todo->setRelatedTo(parentUid);
    }

    const int itemCollectonId = item.parentCollection().id();
    const int parentCollectionId = parentCollection.id();
    const bool shouldMoveItems = (parentType != Zanshin::Inbox)
                              && (itemCollectonId != parentCollectionId);

    Akonadi::Item::List childItems;
    if (shouldMoveItems) {
        // Collect first, as the inner fetch job has to be
        // done before any transaction is created
        childItems = collectChildItems(item);
    }

    // Make the whole modification and move happen in a single transaction
    Akonadi::TransactionSequence *transaction = new Akonadi::TransactionSequence();

    new Akonadi::ItemModifyJob(item, transaction);

    if (shouldMoveItems) {
        new Akonadi::ItemMoveJob(item, parentCollection, transaction);

        if (!childItems.isEmpty()) {
            new Akonadi::ItemMoveJob(childItems, parentCollection, transaction);
        }
    }
    return true;
}

bool TodoHelpers::promoteTodo(const QModelIndex &index)
{
    // We use ParentCollectionRole instead of Akonadi::Item::parentCollection() because the
    // information about the rights is not valid on retrieved items.
    Akonadi::Collection collection = index.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>();
    if (!(collection.rights() & Akonadi::Collection::CanChangeItem)) {
        return false;
    }

    Zanshin::ItemType itemType = (Zanshin::ItemType)index.data(Zanshin::ItemTypeRole).toInt();
    if (itemType!=Zanshin::StandardTodo) {
        return false;
    }

    const Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();

    if (!todo) {
        return false;
    }

    todo->addComment("X-Zanshin-Project");
    new Akonadi::ItemModifyJob(item);
    return true;
}
