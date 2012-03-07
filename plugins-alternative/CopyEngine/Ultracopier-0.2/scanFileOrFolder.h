/** \file scanFileOrFolder.h
\brief Thread changed to list recursively the folder
\author alpha_one_x86
\version 0.3
\date 2011 */

#include <QThread>
#include <QStringList>
#include <QString>
#include <QList>
#include <QFileInfo>
#include <QDir>
#include <QSemaphore>

#include "Environment.h"

#ifndef SCANFILEORFOLDER_H
#define SCANFILEORFOLDER_H

/// \brief Thread changed to list recursively the folder
class scanFileOrFolder : public QThread
{
	Q_OBJECT
public:
	explicit scanFileOrFolder(QObject *parent = 0);
	~scanFileOrFolder();
	/// \brief to the a folder listing
	void stop();
	/// \brief to get if is finished
	bool isFinished();
	/// \brief set action if Folder are same or exists
	void setFolderExistsAction(FolderExistsAction action);
	/// \brief set action if error
	void setFolderErrorAction(FileErrorAction action);
	/// \brief set if need check if the destination exists
	void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
signals:
	void folderTransfer(QString source,QString destination,int numberOfItem);
	void fileTransfer(QFileInfo source,QFileInfo destination);
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	void folderAlreadyExists(QFileInfo source,bool isSame,QFileInfo destination);
	void errorOnFolder(QFileInfo fileInfo,QString errorString);
public slots:
	void addToList(const QStringList& sources,const QString& destination);
protected:
	void run();
private:
	QStringList		sources;
	QString			destination;
	volatile bool		stopIt;
        void			listFolder(const QString& source,const QString& destination,const QString& sourceSuffixPath,QString destinationSuffixPath);
	volatile bool		stopped;
	QSemaphore		waitOneAction;
	FolderExistsAction	folderExistsAction;
	FileErrorAction		fileErrorAction;
	volatile bool		checkDestinationExists;
};

#endif // SCANFILEORFOLDER_H
