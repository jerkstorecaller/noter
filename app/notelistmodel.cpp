#include <cassert>
#include <algorithm>
#include "app.h"
#include "notelistmodel.h"
#include <QDebug>

using namespace std;

NoteListModel::NoteListModel(Repository& repository) : repository(repository)
{
	connect(&repository, &Repository::noteCreated, this, &NoteListModel::onNoteCreated);
	connect(&repository, &Repository::noteUpdated, this, &NoteListModel::onNoteUpdated);
	connect(&repository, &Repository::noteDeleted, this, &NoteListModel::onNoteDeleted);
}

QVariant NoteListModel::data(const QModelIndex &index, int role) const
{
	if (!this->results || index.row() < 0 || index.row() >= this->results->size()) {
		return QVariant();
	}
	if (role == NoteListModel::TITLE) {
		return this->results->at(index.row())->getTitle();
	}
	if (role == NoteListModel::UPDATED) {
		return this->results->at(index.row())->getUpdatedAt();
	}
	return QVariant();
}

Note* NoteListModel::get(int index) const
{
	if (!this->results || index < 0 || index >= this->results->size()) {
		return nullptr;
	}
	Note* note = this->results->at(index).get();
	assert(note);
	App::instance()->getQmlEngine().setObjectOwnership(note, QQmlEngine::CppOwnership);
	return note;
}

void NoteListModel::query(const QString &query)
{
	beginResetModel();
	this->results = this->repository.findNotes(query);
	endResetModel();
}

QHash<int, QByteArray> NoteListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[NoteListModel::TITLE] = "title";
	roles[NoteListModel::UPDATED] = "updatedAt";
	return roles;
}

void NoteListModel::onNoteUpdated(Note* note)
{
	if (!this->results) {
		return;
	}

	int index = this->findIndex(note);
	if (index > 0) {
		beginMoveRows(QModelIndex(), index, index, QModelIndex(), 0);
		swap(this->results->at(index), this->results->at(0));
		endMoveRows();
		index = 0;
	}
	if (index >= 0) {
		auto modelIndex = createIndex(index, 0);
		QVector<int> roles{ NoteListModel::TITLE, NoteListModel::UPDATED };
		emit dataChanged(modelIndex, modelIndex, roles);
	}
}

void NoteListModel::onNoteCreated(Repository::NotePtr note)
{
	if (!this->results) {
		this->results.reset(new Repository::ResultSet);
	}
	beginInsertRows(QModelIndex(), 0, 0);
	this->results->insert(this->results->begin(), note);
	endInsertRows();
}

void NoteListModel::onNoteDeleted(Note* note)
{
	if (!this->results) {
		return;
	}
	int index = this->findIndex(note);
	if (index >= 0) {
		beginRemoveRows(QModelIndex(), index, index);
		this->results->erase(this->results->begin() + index);
		endRemoveRows();
	}
}

int NoteListModel::findIndex(const Note* note)
{
	for (int i = 0; i < this->results->size(); i++) {
		if (this->results->at(i).get() == note) {
			return i;
		}
	}
	return -1;
}
