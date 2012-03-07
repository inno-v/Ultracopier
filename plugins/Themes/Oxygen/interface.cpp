/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QMessageBox>

#include "interface.h"
#include "ui_interface.h"

Themes::Themes(bool checkBoxShowSpeed,FacilityInterface * facilityEngine) :
	ui(new Ui::interfaceCopy())
{
	this->facilityEngine=facilityEngine;
	ui->setupUi(this);
	ui->tabWidget->setCurrentIndex(0);
	ui->checkBoxShowSpeed->setChecked(checkBoxShowSpeed);
	currentFile	= 0;
	totalFile	= 0;
	currentSize	= 0;
	totalSize	= 0;
	haveError	= false;
	this->show();
	menu=new QMenu(this);
	ui->add->setMenu(menu);
	on_checkBoxShowSpeed_toggled(ui->checkBoxShowSpeed->isChecked());
	currentSpeed	= -1;
	updateSpeed();
	storeIsInPause	= false;
	isInPause(false);
	modeIsForced	= false;
	haveStarted	= false;
	connect(ui->limitSpeed,		SIGNAL(valueChanged(int)),	this,	SLOT(uiUpdateSpeed()));
	connect(ui->checkBox_limitSpeed,SIGNAL(toggled(bool)),		this,	SLOT(uiUpdateSpeed()));

	//setup the search part
	closeTheSearchBox();
	TimerForSearch  = new QTimer(this);
	TimerForSearch->setInterval(500);
	TimerForSearch->setSingleShot(true);
	searchShortcut  = new QShortcut(QKeySequence("Ctrl+F"),this);
	searchShortcut2 = new QShortcut(QKeySequence("F3"),this);
	searchShortcut3 = new QShortcut(QKeySequence("Escape"),this);//Qt::Key_Escape

	//connect the search part
	connect(TimerForSearch,			SIGNAL(timeout()),	this,	SLOT(hilightTheSearch()));
	connect(searchShortcut,			SIGNAL(activated()),	this,	SLOT(searchBoxShortcut()));
	connect(searchShortcut2,		SIGNAL(activated()),	this,	SLOT(on_pushButtonSearchNext_clicked()));
	connect(ui->pushButtonCloseSearch,	SIGNAL(clicked()),	this,	SLOT(closeTheSearchBox()));
	connect(searchShortcut3,		SIGNAL(activated()),	this,	SLOT(closeTheSearchBox()));

	//reload directly untranslatable text
	newLanguageLoaded();

	//unpush the more button
	ui->moreButton->setChecked(false);
	on_moreButton_toggled(false);

	/// \note important for drag and drop, \see dropEvent()
	setAcceptDrops(true);

	// try set the OS icon
	QIcon tempIcon;

	tempIcon=QIcon::fromTheme("application-exit");
	if(!tempIcon.isNull())
	{
		ui->cancelButton->setIcon(tempIcon);
		ui->pushButtonCloseSearch->setIcon(tempIcon);
		ui->shutdown->setIcon(tempIcon);
	}

	tempIcon=QIcon::fromTheme("edit-delete");
	if(!tempIcon.isNull())
		ui->del->setIcon(tempIcon);

	tempIcon=QIcon::fromTheme("media-playback-pause");
	if(!tempIcon.isNull())
	{
		player_pause=tempIcon;
		ui->pauseButton->setIcon(tempIcon);
	}
	else
		player_pause=QIcon(":/resources/player_pause.png");

	tempIcon=QIcon::fromTheme("media-playback-play");
	if(!tempIcon.isNull())
		player_play=tempIcon;
	else
		player_play=QIcon(":/resources/player_play.png");

	tempIcon=QIcon::fromTheme("media-skip-forward");
	if(!tempIcon.isNull())
		ui->skipButton->setIcon(tempIcon);

	tempIcon=QIcon::fromTheme("edit-find");
	if(!tempIcon.isNull())
		ui->searchButton->setIcon(tempIcon);

	tempIcon=QIcon::fromTheme("document-open");
	if(!tempIcon.isNull())
		ui->importTransferList->setIcon(tempIcon);

	tempIcon=QIcon::fromTheme("document-save");
	if(!tempIcon.isNull())
		ui->exportTransferList->setIcon(tempIcon);

	tempIcon=QIcon::fromTheme("list-add");
	if(!tempIcon.isNull())
	{
		ui->add->setIcon(tempIcon);
		ui->actionAddFile->setIcon(tempIcon);
		ui->actionAddFileToCopy->setIcon(tempIcon);
		ui->actionAddFileToMove->setIcon(tempIcon);
		ui->actionAddFolder->setIcon(tempIcon);
		ui->actionAddFolderToCopy->setIcon(tempIcon);
		ui->actionAddFolderToMove->setIcon(tempIcon);
	}

	shutdown=facilityEngine->haveFunctionality("shutdown");
	ui->shutdown->setVisible(shutdown);
}

void Themes::uiUpdateSpeed()
{
	if(!ui->checkBoxShowSpeed->isChecked())
		emit newSpeedLimitation(0);
	else
		emit newSpeedLimitation(ui->limitSpeed->value());
}

Themes::~Themes()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	disconnect(ui->actionAddFile);
	disconnect(ui->actionAddFolder);
	delete menu;
}

QWidget * Themes::getOptionsEngineWidget()
{
	return &optionEngineWidget;
}

void Themes::getOptionsEngineEnabled(bool isEnabled)
{
	if(isEnabled)
		ui->tabWidget->addTab(&optionEngineWidget,tr("Copy engine"));
}

void Themes::closeEvent(QCloseEvent *event)
{
	event->ignore();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->hide();
	emit cancel();
}

void Themes::updateOverallInformation()
{
	ui->overall->setText(tr("File %1/%2, size: %3/%4").arg(currentFile).arg(totalFile).arg(facilityEngine->sizeToString(currentSize)).arg(facilityEngine->sizeToString(totalSize)));
}

void Themes::actionInProgess(EngineActionInProgress action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"start: "+QString::number(action));
	this->action=action;
	switch(action)
	{
		case Copying:
		case CopyingAndListing:
			ui->progressBar_all->setMaximum(65535);
			ui->progressBar_all->setMinimum(0);
		break;
		case Listing:
			ui->progressBar_all->setMaximum(0);
			ui->progressBar_all->setMinimum(0);
		break;
		case Idle:
			if(haveStarted)
			{
				if(shutdown && ui->shutdown->isChecked())
				{
					facilityEngine->callFunctionality("shutdown");
					return;
				}
				switch(ui->comboBox_copyEnd->currentIndex())
				{
					case 2:
						emit cancel();
					break;
					case 0:
						if(!haveError)
							emit cancel();
					break;
					default:
					break;
				}
			}
		break;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Very wrong switch case!");
		break;
	}
	switch(action)
	{
		case Copying:
		case CopyingAndListing:
			ui->pauseButton->setEnabled(true);
			haveStarted=true;
			ui->cancelButton->setText(tr("Quit"));
		break;
		case Idle:
			ui->pauseButton->setEnabled(false);
		break;
		default:
		break;
	}
}

void Themes::newTransferStart(const ItemOfCopyList &item)
{
	index=0;
	loop_size=0;
	//is too heavy for normal usage, then enable it only in debug mode to develop
	#ifdef ULTRACOPIER_DEBUG
	loop_size=graphicItemList.size();
	while(index<loop_size)
	{
		if(graphicItemList.at(index).id==item.id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Duplicate start transfer : "+QString::number(item.id));
			return;
		}
		index++;
	}
	#endif // ULTRACOPIER_DEBUG
	ItemOfCopyListWithMoreInformations newItem;
	newItem.currentProgression=0;
	newItem.generalData=item;
	currentProgressList<<newItem;
	ui->skipButton->setEnabled(true);
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Start transfer new item: "+QString::number(currentProgressList.last().generalData.id)+", with size: "+QString::number(currentProgressList.last().generalData.size));
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size)
	{
		if(graphicItemList.at(index).id==item.id)
		{
			graphicItemList.at(index).item->setIcon(0,player_play);
			break;
		}
		index++;
	}
	updateCurrentFileInformation();
}

//is stopped, example: because error have occurred, and try later, don't remove the item!
void Themes::newTransferStop(const quint64 &id)
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+QString::number(id));

	//update the icon, but the item can be removed before from the transfer list, then not found
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size)
	{
		if(graphicItemList.at(index).id==id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"item to remove found");
			graphicItemList.at(index).item->setIcon(0,player_pause);
			break;
		}
		index++;
	}

	//update the internal transfer list
	index=0;
	loop_size=currentProgressList.size();
	while(index<loop_size)
	{
		if(currentProgressList.at(index).generalData.id==id)
		{
			//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"at index: "+QString::number(index)+", remove item: "+QString::number(currentProgressList.at(index).generalData.id)+", with size: "+QString::number(currentProgressList.at(index).generalData.size));
			currentProgressList.removeAt(index);
			updateCurrentFileInformation();
			break;
		}
		index++;
	}

	//update skip button if needed
	if(currentProgressList.size()==0)
		ui->skipButton->setEnabled(false);
}

void Themes::newFolderListing(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(action==Listing)
		ui->from->setText(path);
}

void Themes::detectedSpeed(const quint64 &speed)//in byte per seconds
{
	ui->currentSpeed->setText(facilityEngine->speedToString(speed));
}

void Themes::remainingTime(const int &remainingSeconds)
{
	if(remainingSeconds==-1)
		ui->labelTimeRemaining->setText("<html><body>&#8734;</body></html>");
	else
	{
		TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(remainingSeconds);
		ui->labelTimeRemaining->setText(QString::number(time.hour)+":"+QString::number(time.minute)+":"+QString::number(time.second));
	}
}

void Themes::newCollisionAction(const QString &action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(ui->comboBox_fileCollisions->findData(action)!=-1)
		ui->comboBox_fileCollisions->setCurrentIndex(ui->comboBox_fileCollisions->findData(action));
}

void Themes::newErrorAction(const QString &action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(ui->comboBox_copyErrors->findData(action)!=-1)
		ui->comboBox_copyErrors->setCurrentIndex(ui->comboBox_copyErrors->findData(action));
}

void Themes::errorDetected()
{
	haveError=true;
}

//speed limitation
bool Themes::setSpeedLimitation(const qint64 &speedLimitation)
{
	currentSpeed=speedLimitation;
	updateSpeed();
	return true;
}

//get information about the copy
void Themes::setGeneralProgression(const quint64 &current,const quint64 &total)
{
	currentSize=current;
	totalSize=total;
	if(total>0)
	{
		int newIndicator=((double)current/total)*65535;
		ui->progressBar_all->setValue(newIndicator);
	}
	else
		ui->progressBar_all->setValue(0);
}

void Themes::setFileProgression(const quint64 &id,const quint64 &current,const quint64 &total)
{
	index=0;
	loop_size=currentProgressList.size();
	while(index<loop_size)
	{
		if(currentProgressList.at(index).generalData.id==id)
		{
			currentProgressList[index].generalData.size=total;
			currentProgressList[index].currentProgression=current;
			if(index==0)
				updateCurrentFileInformation();
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to found the file");
}

void Themes::setCollisionAction(const QList<QPair<QString,QString> > &list)
{
	ui->comboBox_fileCollisions->clear();
	index=0;
	loop_size=list.size();
	while(index<loop_size)
	{
		ui->comboBox_fileCollisions->addItem(list.at(index).first,list.at(index).second);
		index++;
	}
}

void Themes::setErrorAction(const QList<QPair<QString,QString> > &list)
{
	ui->comboBox_fileCollisions->clear();
	index=0;
	loop_size=list.size();
	while(index<loop_size)
	{
		ui->comboBox_copyErrors->addItem(list.at(index).first,list.at(index).second);
		index++;
	}
}

//edit the transfer list
void Themes::getActionOnList(const QList<returnActionOnCopyList> &returnActions)
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, returnActions.size(): "+QString::number(returnActions.size()));
	indexAction=0;
	index=0;
	loop_size=returnActions.size();
	while(indexAction<loop_size)
	{
		if(returnActions.at(indexAction).type==AddingItem)
		{
			graphicItem newItem;
			newItem.id=returnActions.at(indexAction).addAction.id;
			QStringList listString;
			listString << returnActions.at(indexAction).addAction.sourceFullPath << facilityEngine->sizeToString(returnActions.at(indexAction).addAction.size) << returnActions.at(indexAction).addAction.destinationFullPath;
			newItem.item=new QTreeWidgetItem(listString);
			ui->CopyList->addTopLevelItem(newItem.item);
			totalFile++;
			graphicItemList<<newItem;
			totalSize+=returnActions.at(indexAction).addAction.size;
			//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Add item: "+QString::number(newItem.id)+", with size: "+QString::number(returnActions.at(indexAction).addAction.size));
			updateOverallInformation();
		}
		else
		{
			index=0;
			loop_sub_size=graphicItemList.size();
			while(index<loop_sub_size)
			{
				if(graphicItemList.at(index).id==returnActions.at(indexAction).userAction.id)
				{
					int pos=ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item);
					if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Warning, graphical item not located");
					}
					else
					{
						bool isSelected=graphicItemList.at(index).item->isSelected();
						switch(returnActions.at(indexAction).userAction.type)
						{
							case MoveItem:
								ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"MoveItem: "+QString::number(returnActions.at(indexAction).userAction.id)+", position: "+QString::number(returnActions.at(indexAction).position));
								ui->CopyList->insertTopLevelItem(returnActions.at(indexAction).position,ui->CopyList->takeTopLevelItem(pos));
								graphicItemList.at(index).item->setSelected(isSelected);
								graphicItemList.move(index,returnActions.at(indexAction).position);
							break;
							case RemoveItem:
								//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Remove: "+QString::number(returnActions.at(indexAction).userAction.id));
								delete graphicItemList.at(index).item;
								graphicItemList.removeAt(index);
								currentFile++;
								updateOverallInformation();
							break;
						}
					}
					break;
				}
				index++;
			}
		}
		indexAction++;
	}
	if(graphicItemList.size()==0)
	{
		ui->progressBar_all->setValue(65535);
		ui->progressBar_file->setValue(65535);
		currentSize=totalSize;
		updateOverallInformation();
	}
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"graphicItemList.size(): "+QString::number(graphicItemList.size()));
}

void Themes::setCopyType(CopyType type)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->type=type;
	updateModeAndType();
}

void Themes::forceCopyMode(CopyMode mode)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	modeIsForced=true;
	this->mode=mode;
	if(mode==Copy)
		this->setWindowTitle("Ultracopier - "+facilityEngine->translateText("Copy"));
	else
		this->setWindowTitle("Ultracopier - "+facilityEngine->translateText("Move"));
	updateModeAndType();
}

void Themes::setTransferListOperation(TransferListOperation transferListOperation)
{
	ui->exportTransferList->setVisible(transferListOperation & TransferListOperation_Export);
	ui->importTransferList->setVisible(transferListOperation & TransferListOperation_Import);
}

void Themes::haveExternalOrder()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
//	ui->moreButton->toggle();
}

void Themes::isInPause(bool isInPause)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"isInPause: "+QString::number(isInPause));
	//resume in auto the pause
	storeIsInPause=isInPause;
	if(isInPause)
	{
		ui->pauseButton->setIcon(player_play);
		ui->pauseButton->setText(facilityEngine->translateText("Resume"));
	}
	else
	{
		ui->pauseButton->setIcon(player_pause);
		ui->pauseButton->setText(facilityEngine->translateText("Pause"));
	}
}

void Themes::updateCurrentFileInformation()
{
	if(currentProgressList.size()>0)
	{
		ui->from->setText(currentProgressList.first().generalData.sourceFullPath);
		ui->to->setText(currentProgressList.first().generalData.destinationFullPath);
		ui->current_file->setText(currentProgressList.first().generalData.destinationFileName+", "+facilityEngine->sizeToString(currentProgressList.first().generalData.size));
		if(currentProgressList.first().generalData.size>0)
			ui->progressBar_file->setValue(((double)currentProgressList.first().currentProgression/currentProgressList.first().generalData.size)*65535);
		else
			ui->progressBar_file->setValue(0);
	}
	else
	{
		ui->from->setText("");
		ui->to->setText("");
		ui->current_file->setText("-");
		if(haveStarted)
			ui->progressBar_file->setValue(65535);
		else
			ui->progressBar_file->setValue(0);
	}
}


void Themes::on_putOnTop_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)

			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit moveItemsOnTop(ids);
}

void Themes::on_pushUp_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit moveItemsUp(ids);
}

void Themes::on_pushDown_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit moveItemsDown(ids);
}

void Themes::on_putOnBottom_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit moveItemsOnBottom(ids);
}

void Themes::on_del_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit removeItems(ids);
}

void Themes::on_cancelButton_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->hide();
	emit cancel();
}

void Themes::on_checkBoxShowSpeed_toggled(bool checked)
{
	if(checked==checked)
		updateSpeed();
}

void Themes::on_SliderSpeed_valueChanged(int value)
{
	switch(value)
	{
		case 0:
			currentSpeed=0;
		break;
		case 1:
			currentSpeed=1024;
		break;
		case 2:
			currentSpeed=1024*4;
		break;
		case 3:
			currentSpeed=1024*16;
		break;
		case 4:
			currentSpeed=1024*64;
		break;
		case 5:
			currentSpeed=1024*128;
		break;
	}
	emit newSpeedLimitation(currentSpeed);
}

void Themes::updateSpeed()
{
	bool checked;
	if(currentSpeed==-1)
	{
		ui->checkBoxShowSpeed->setEnabled(false);
		checked=false;
	}
	else
	{
		ui->checkBoxShowSpeed->setEnabled(true);
		checked=ui->checkBox_limitSpeed->isChecked();
	}
	ui->label_Slider_speed->setVisible(checked);
	ui->SliderSpeed->setVisible(checked);
	ui->label_SpeedMaxValue->setVisible(checked);
	ui->checkBox_limitSpeed->setEnabled(checked);
	if(checked)
	{
		ui->limitSpeed->setEnabled(false);
		if(currentSpeed==0)
		{
			ui->SliderSpeed->setValue(0);
			ui->label_SpeedMaxValue->setText(tr("Unlimited"));
		}
		else if(currentSpeed<=1024)
		{
			if(currentSpeed!=1024)
			{
				currentSpeed=1024;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(1);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*1));
		}
		else if(currentSpeed<=1024*4)
		{
			if(currentSpeed!=1024*4)
			{
				currentSpeed=1024*4;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(2);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*4));
		}
		else if(currentSpeed<=1024*16)
		{
			if(currentSpeed!=1024*16)
			{
				currentSpeed=1024*16;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(3);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*16));
		}
		else if(currentSpeed<=1024*64)
		{
			if(currentSpeed!=1024*64)
			{
				currentSpeed=1024*64;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(4);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*64));
		}
		else
		{
			if(currentSpeed!=1024*128)
			{
				currentSpeed=1024*128;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(5);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*128));
		}
	}
	else
	{
		ui->checkBox_limitSpeed->setChecked(currentSpeed>0);
		if(currentSpeed>0)
			ui->limitSpeed->setValue(currentSpeed);
		ui->checkBox_limitSpeed->setEnabled(currentSpeed!=-1);
		ui->limitSpeed->setEnabled(ui->checkBox_limitSpeed->isChecked());
	}
}

void Themes::on_limitSpeed_valueChanged(int value)
{
	currentSpeed=value;
	emit newSpeedLimitation(currentSpeed);
}

void Themes::on_checkBox_limitSpeed_clicked()
{
	if(ui->checkBox_limitSpeed->isChecked())
	{
		if(ui->checkBoxShowSpeed->isChecked())
			on_SliderSpeed_valueChanged(ui->SliderSpeed->value());
		else
			on_limitSpeed_valueChanged(ui->limitSpeed->value());
	}
	else
		currentSpeed=0;
}

void Themes::on_pauseButton_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(storeIsInPause)
		emit resume();
	else
		emit pause();
}

void Themes::on_skipButton_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(currentProgressList.size()>0)
		emit skip(currentProgressList.first().generalData.id);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to skip the transfer, because no transfer running");
}

void Themes::updateModeAndType()
{
	menu->clear();
	if(modeIsForced)
	{
		if(type==File || type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFile);
			connect(ui->actionAddFile,SIGNAL(triggered()),this,SLOT(forcedModeAddFile()));
		}
		if(type==Folder || type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFolder);
			connect(ui->actionAddFolder,SIGNAL(triggered()),this,SLOT(forcedModeAddFolder()));
		}
	}
	else
	{
		if(type==File || type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFileToCopy);
			menu->addAction(ui->actionAddFileToMove);
			connect(ui->actionAddFileToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToCopy()));
			connect(ui->actionAddFileToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToMove()));
		}
		if(type==Folder || type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFolderToCopy);
			menu->addAction(ui->actionAddFolderToMove);
			connect(ui->actionAddFolderToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToCopy()));
			connect(ui->actionAddFolderToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToMove()));

		}
	}
}

void Themes::forcedModeAddFile()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFile(mode);
}

void Themes::forcedModeAddFolder()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFolder(mode);
}

void Themes::forcedModeAddFileToCopy()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFile(Copy);
}

void Themes::forcedModeAddFolderToCopy()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFolder(Copy);
}

void Themes::forcedModeAddFileToMove()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFile(Move);
}

void Themes::forcedModeAddFolderToMove()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFolder(Move);
}

void Themes::newLanguageLoaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(modeIsForced)
		forceCopyMode(mode);
	ui->retranslateUi(this);
	if(!haveStarted)
		ui->current_file->setText(tr("File Name, 0KB"));
	else
		updateCurrentFileInformation();
	updateOverallInformation();
	updateSpeed();
	ui->tabWidget->setTabText(4,facilityEngine->translateText("Copy engine"));
	on_moreButton_toggled(ui->moreButton->isChecked());
}

void Themes::on_pushButtonCloseSearch_clicked()
{
	closeTheSearchBox();
}

//close the search box
void Themes::closeTheSearchBox()
{
	currentIndexSearch = -1;
	ui->lineEditSearch->clear();
	ui->lineEditSearch->hide();
	ui->pushButtonSearchPrev->hide();
	ui->pushButtonSearchNext->hide();
	ui->pushButtonCloseSearch->hide();
	ui->searchButton->setChecked(false);
	hilightTheSearch();
}

//search box shortcut
void Themes::searchBoxShortcut()
{
/*	if(ui->lineEditSearch->isHidden())
	{*/
		ui->lineEditSearch->show();
		ui->pushButtonSearchPrev->show();
		ui->pushButtonSearchNext->show();
		ui->pushButtonCloseSearch->show();
		ui->lineEditSearch->setFocus(Qt::ShortcutFocusReason);
		ui->searchButton->setChecked(true);
/*	}
	else
		closeTheSearchBox();*/
}

//hilight the search
void Themes::hilightTheSearch()
{
	QFont *fontNormal=new QFont();
	QTreeWidgetItem * item=NULL;
	//get the ids to do actions
	int i=0;
	loop_size=ui->CopyList->topLevelItemCount();
	if(ui->lineEditSearch->text().isEmpty())
	{
		while(i<loop_size)
		{
			item=ui->CopyList->topLevelItem(i);
			item->setBackgroundColor(0,QColor(255,255,255,0));
			item->setBackgroundColor(1,QColor(255,255,255,0));
			item->setBackgroundColor(2,QColor(255,255,255,0));
			item->setFont(0,*fontNormal);
			item->setFont(1,*fontNormal);
			item->setFont(2,*fontNormal);
			i++;
		}
		ui->lineEditSearch->setStyleSheet("");
	}
	else
	{
		bool itemFound=false;
		while(i<loop_size)
		{
			item=ui->CopyList->topLevelItem(i);
			if(item->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1 || item->text(2).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1)
			{
				itemFound=true;
				item->setBackgroundColor(0,QColor(255,255,0,100));
				item->setBackgroundColor(1,QColor(255,255,0,100));
				item->setBackgroundColor(2,QColor(255,255,0,100));
			}
			else
			{
				item->setBackgroundColor(0,QColor(255,255,255,0));
				item->setBackgroundColor(1,QColor(255,255,255,0));
				item->setBackgroundColor(2,QColor(255,255,255,0));
			}
			item->setFont(0,*fontNormal);
			item->setFont(1,*fontNormal);
			item->setFont(2,*fontNormal);
			i++;
		}
		if(!itemFound)
			ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
		else
			ui->lineEditSearch->setStyleSheet("");
	}
	delete fontNormal;
}

void Themes::on_pushButtonSearchPrev_clicked()
{
	if(!ui->lineEditSearch->text().isEmpty() && ui->CopyList->topLevelItemCount()>0)
	{
		hilightTheSearch();
		int searchStart;
		if(currentIndexSearch<0 || currentIndexSearch>=ui->CopyList->topLevelItemCount())
			searchStart=ui->CopyList->topLevelItemCount()-1;
		else
			searchStart=ui->CopyList->topLevelItemCount()-currentIndexSearch-2;
		int count=0;
		QTreeWidgetItem *curs=NULL;
		loop_size=ui->CopyList->topLevelItemCount();
		while(count<loop_size)
		{
			if(searchStart<0)
				searchStart+=ui->CopyList->topLevelItemCount();
			if(ui->CopyList->topLevelItem(searchStart)->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1 || ui->CopyList->topLevelItem(searchStart)->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1)
			{
				curs=ui->CopyList->topLevelItem(searchStart);
				break;
			}
			searchStart--;
			count++;
		}
		if(curs!=NULL)
		{
			currentIndexSearch=ui->CopyList->topLevelItemCount()-1-ui->CopyList->indexOfTopLevelItem(curs);
			ui->lineEditSearch->setStyleSheet("");
			QFont *bold=new QFont();
			bold->setBold(true);
			curs->setFont(0,*bold);
			curs->setFont(1,*bold);
			curs->setFont(2,*bold);
			curs->setBackgroundColor(0,QColor(255,255,0,200));
			curs->setBackgroundColor(1,QColor(255,255,0,200));
			curs->setBackgroundColor(2,QColor(255,255,0,200));
			ui->CopyList->scrollToItem(curs);
			delete bold;
		}
		else
			ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
	}
}

void Themes::on_pushButtonSearchNext_clicked()
{
	if(ui->lineEditSearch->text().isEmpty())
	{
		if(ui->lineEditSearch->isHidden())
			searchBoxShortcut();
	}
	else
	{
		if(ui->CopyList->topLevelItemCount()>0)
		{
			hilightTheSearch();
			int searchStart;
			if(currentIndexSearch<0 || currentIndexSearch>=ui->CopyList->topLevelItemCount())
				searchStart=0;
			else
				searchStart=ui->CopyList->topLevelItemCount()-currentIndexSearch;
			int count=0;
			QTreeWidgetItem *curs=NULL;
			loop_size=ui->CopyList->topLevelItemCount();
			while(count<loop_size)
			{
				if(searchStart>=ui->CopyList->topLevelItemCount())
					searchStart-=ui->CopyList->topLevelItemCount();
				if(ui->CopyList->topLevelItem(searchStart)->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1 || ui->CopyList->topLevelItem(searchStart)->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1)
				{
					curs=ui->CopyList->topLevelItem(searchStart);
					break;
				}
				searchStart++;
				count++;
			}
			if(curs!=NULL)
			{
				currentIndexSearch=ui->CopyList->topLevelItemCount()-1-ui->CopyList->indexOfTopLevelItem(curs);
				ui->lineEditSearch->setStyleSheet("");
				QFont *bold=new QFont();
				bold->setBold(true);
				curs->setFont(0,*bold);
				curs->setFont(1,*bold);
				curs->setFont(2,*bold);
				curs->setBackgroundColor(0,QColor(255,255,0,200));
				curs->setBackgroundColor(1,QColor(255,255,0,200));
				curs->setBackgroundColor(2,QColor(255,255,0,200));
				ui->CopyList->scrollToItem(curs);
				delete bold;
			}
			else
				ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
		}
	}
}

void Themes::on_lineEditSearch_returnPressed()
{
	hilightTheSearch();
}

void Themes::on_lineEditSearch_textChanged(QString text)
{
	if(text=="")
	{
		TimerForSearch->stop();
		hilightTheSearch();
	}
	else
		TimerForSearch->start();
}

void Themes::on_moreButton_toggled(bool checked)
{
	if(checked)
		this->setMaximumHeight(16777215);
	else
		this->setMaximumHeight(130);
	// usefull under windows
	this->updateGeometry();
	this->update();
	this->adjustSize();
}

void Themes::on_comboBox_copyErrors_currentIndexChanged(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit sendErrorAction(ui->comboBox_copyErrors->itemData(index).toString());
}

void Themes::on_comboBox_fileCollisions_currentIndexChanged(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit sendCollisionAction(ui->comboBox_fileCollisions->itemData(index).toString());
}

/* drag event processing

need setAcceptDrops(true); into the constructor
need implementation to accept the drop:
void dragEnterEvent(QDragEnterEvent* event);
void dragMoveEvent(QDragMoveEvent* event);
void dragLeaveEvent(QDragLeaveEvent* event);
*/
void Themes::dropEvent(QDropEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if(mimeData->hasUrls())
	{
		emit urlDropped(mimeData->urls());
		event->acceptProposedAction();
	}
}

void Themes::dragEnterEvent(QDragEnterEvent* event)
{
	// if some actions should not be usable, like move, this code must be adopted
	event->acceptProposedAction();
}

void Themes::dragMoveEvent(QDragMoveEvent* event)
{
	// if some actions should not be usable, like move, this code must be adopted
	event->acceptProposedAction();
}

void Themes::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}

void Themes::on_searchButton_toggled(bool checked)
{
	if(checked)
		searchBoxShortcut();
	else
		closeTheSearchBox();
}

void Themes::on_exportTransferList_clicked()
{
	emit exportTransferList();
}

void Themes::on_importTransferList_clicked()
{
	emit importTransferList();
}
