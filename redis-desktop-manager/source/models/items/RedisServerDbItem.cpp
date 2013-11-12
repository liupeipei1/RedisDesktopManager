#include "RedisServerDbItem.h"
#include "RedisServerItem.h"
#include "RedisKeyItem.h"
#include "RedisKeyNamespace.h"
#include <QStandardItem>

RedisServerDbItem::RedisServerDbItem(QString name, int keysCount, RedisServerItem * parent) 
	: server(parent), isKeysLoaded(false), dbIndex(0), keysCount(keysCount), name(name), 
	  keyIcon(":/images/key.png"), namespaceIcon(":/images/namespace.png")
{	
	setNormalIcon();
	setText(QString("%1 (%2)").arg(name).arg(keysCount));

	QRegExp getDbIndex("(\\d+)");		

	if (getDbIndex.indexIn(name) > -1) {				
		dbIndex = getDbIndex.cap(1).toInt();
	}

	setEditable(false);	
}

void RedisServerDbItem::loadKeys()
{
	if (isKeysLoaded) return;

	setBusyIcon();


// 	if (!connection->isConnected() && !connection->connect()) {
// 		setNormalIcon();
// 		server->error(QString("Can not load keys: %1").arg(connection->getLastError()));
// 		return;
// 	}		

	//wait for signal from connection	
	connect(server->connection, SIGNAL(responseResieved(const QVariant &, QObject *)), this, SLOT(keysLoaded(const QVariant &, QObject *)));
	
	server->connection->addCommand(Command("keys *", this, dbIndex));
}

void RedisServerDbItem::keysLoaded(const QVariant &keys, QObject *owner)
{
	if (owner != this) {
		return;
	}

	server->connection->disconnect(this);

	rawKeys = keys.toStringList();

	int resultSize = rawKeys.size();

	if (resultSize == 0) {
		server->unlockUI();
		setNormalIcon();
		return;
	}

	if (resultSize != keysCount) {
		server->error(QString("Loaded keys: %2 of %3. Error - %4 <br /> Check <a href='https://github.com/uglide/RedisDesktopManager/wiki/Known-issues'>documentation</a>")
			.arg(resultSize)
			.arg(keysCount)
			.arg(server->connection->getLastError()));
	}

	renderKeys(rawKeys);

	setNormalIcon();
	isKeysLoaded = true;	
	server->unlockUI();
}

void RedisServerDbItem::setFilter(QRegExp &pattern)
{
	filter = pattern;

	removeRows(0, rowCount());

	renderKeys(rawKeys);
}

void RedisServerDbItem::resetFilter()
{
	filter.setPattern("");

	removeRows(0, rowCount());

	renderKeys(rawKeys);
}

void RedisServerDbItem::renderKeys(QStringList &rawKeys)
{
	for (QString rawKey : rawKeys) {

		//if filter enabled - skip keys
		if (!filter.isEmpty() && !rawKey.contains(filter)) {
			continue;
		}

		renderNamaspacedKey(this, rawKey, rawKey);		
	}
	this->sortChildren(0);
}

void RedisServerDbItem::renderNamaspacedKey(QStandardItem * currItem, 
											QString notProcessedKeyPart, QString fullKey)
{
	if (!notProcessedKeyPart.contains(":")) {
		QStandardItem * newKey = new RedisKeyItem(fullKey, this, keyIcon);
		currItem->appendRow(newKey);	
		return;
	}

	int indexOfNaspaceSeparator = notProcessedKeyPart.indexOf(":");

	QString firstNamespaceName = notProcessedKeyPart.mid(0, indexOfNaspaceSeparator);

	QStandardItem * namespaceItem = nullptr;

	for (int i=0; i < currItem->rowCount(); ++i)
	{
		QStandardItem * child = currItem->child(i);

		if (child->type() == RedisKeyNamespace::TYPE 
			&& child->text() == firstNamespaceName) {
				namespaceItem = child;
				break;
		}
	}

	if (namespaceItem == nullptr) {
		namespaceItem = new RedisKeyNamespace(firstNamespaceName, namespaceIcon);
		currItem->appendRow(namespaceItem);
	}

	renderNamaspacedKey(namespaceItem, notProcessedKeyPart.mid(indexOfNaspaceSeparator+1), fullKey);	

	namespaceItem->sortChildren(0);
}

void RedisServerDbItem::setBusyIcon()
{
	setIcon(QIcon(":/images/wait.png"));
}

void RedisServerDbItem::setNormalIcon()
{
	setIcon(QIcon(":/images/db.png"));
}

int RedisServerDbItem::type() const
{
	return TYPE;
}

bool RedisServerDbItem::operator<(const QStandardItem & other) const
{
	if (other.type() == TYPE) {
		const RedisServerDbItem * another = dynamic_cast<const RedisServerDbItem *>(&other);

		return dbIndex < another->getDbIndex();
	}	

	return this->text() < other.text();
}

int RedisServerDbItem::getDbIndex() const
{
	return dbIndex;
}