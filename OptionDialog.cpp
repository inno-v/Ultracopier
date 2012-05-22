/** \file OptionDialog.cpp
\brief To have an interface to control the options
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "OptionDialog.h"
#include "ui_OptionDialog.h"

#include <QDomElement>
#include <QFileDialog>

OptionDialog::OptionDialog() :
	ui(new Ui::OptionDialog)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	ignoreCopyEngineListEdition=false;
	allPluginsIsLoaded=false;
	ui->setupUi(this);
	ui->treeWidget->topLevelItem(0)->setSelected(true);
	ui->treeWidget->topLevelItem(2)->setExpanded(true);
	ui->pluginList->topLevelItem(0)->setExpanded(true);
	ui->pluginList->topLevelItem(1)->setExpanded(true);
	ui->pluginList->topLevelItem(2)->setExpanded(true);
	ui->pluginList->topLevelItem(3)->setExpanded(true);
	ui->pluginList->topLevelItem(4)->setExpanded(true);
	ui->pluginList->topLevelItem(5)->setExpanded(true);
	on_treeWidget_itemSelectionChanged();

	//load the plugins
	plugins->lockPluginListEdition();
	QList<PluginsAvailable> list=plugins->getPlugins();
	qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
	connect(this,SIGNAL(previouslyPluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)),Qt::QueuedConnection);
	connect(plugins,	SIGNAL(onePluginAdded(PluginsAvailable)),		this,	SLOT(onePluginAdded(PluginsAvailable)));
	connect(plugins,	SIGNAL(onePluginWillBeRemoved(PluginsAvailable)),	this,	SLOT(onePluginWillBeRemoved(PluginsAvailable)),Qt::DirectConnection);
	connect(plugins,	SIGNAL(pluginListingIsfinish()),			this,	SLOT(loadOption()),Qt::QueuedConnection);
	connect(options,	SIGNAL(newOptionValue(QString,QString,QVariant)),	this,	SLOT(newOptionValue(QString,QString,QVariant)));
	foreach(PluginsAvailable currentPlugin,list)
		emit previouslyPluginAdded(currentPlugin);
	plugins->unlockPluginListEdition();
	defaultImportBackend=PluginsManager::ImportBackend_File;
	#ifndef ULTRACOPIER_PLUGIN_SUPPORT
	ui->pluginAdd->show();
	ui->pluginRemove->show();
	#endif
}

OptionDialog::~OptionDialog()
{
	delete ui;
}

//plugin management
void OptionDialog::onePluginAdded(PluginsAvailable plugin)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+plugin.name+" ("+QString::number(plugin.category)+")");
	pluginStore newItem;
	newItem.path=plugin.path;
	newItem.item=new QTreeWidgetItem(QStringList() << plugin.name << plugin.version);
	newItem.isWritable=plugin.isWritable;
	pluginLink<<newItem;
	switch(plugin.category)
	{
		case PluginType_CopyEngine:
			ui->pluginList->topLevelItem(0)->addChild(newItem.item);
		break;
		case PluginType_Languages:
			ui->pluginList->topLevelItem(1)->addChild(newItem.item);
			addLanguage(plugin);
		break;
		case PluginType_Listener:
			ui->pluginList->topLevelItem(2)->addChild(newItem.item);
		break;
		case PluginType_PluginLoader:
			ui->pluginList->topLevelItem(3)->addChild(newItem.item);
		break;
		case PluginType_SessionLoader:
			ui->pluginList->topLevelItem(4)->addChild(newItem.item);
		break;
		case PluginType_Themes:
			ui->pluginList->topLevelItem(5)->addChild(newItem.item);
			addTheme(plugin);
		break;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"category not found for: "+plugin.path);
	}
}

void OptionDialog::onePluginWillBeRemoved(PluginsAvailable plugin)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	index=0;
	loop_size=pluginLink.size();
	while(index<loop_size)
	{
		if(pluginLink.at(index).path==plugin.path)
		{
			delete pluginLink.at(index).item;
			if(plugin.category==PluginType_Languages)
				removeLanguage(plugin);
			else if(plugin.category==PluginType_Themes)
				removeTheme(plugin);
			else if(plugin.category==PluginType_CopyEngine)
				removeCopyEngine(plugin);
			pluginLink.removeAt(index);
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"not found!");
}

void OptionDialog::addLanguage(PluginsAvailable plugin)
{
	QList<QPair<QString,QString> > listChildAttribute;
	QPair<QString,QString> temp;
	temp.first = "mainCode";
	temp.second = "true";
	listChildAttribute << temp;
	ui->Language->addItem(QIcon(plugin.path+"flag.png"),plugins->getDomSpecific(plugin.categorySpecific,"fullName"),plugins->getDomSpecific(plugin.categorySpecific,"shortName",listChildAttribute));
}

void OptionDialog::removeLanguage(PluginsAvailable plugin)
{
	QList<QPair<QString,QString> > listChildAttribute;
	QPair<QString,QString> temp;
	temp.first = "mainCode";
	temp.second = "true";
	listChildAttribute << temp;
	int index=ui->Language->findData(plugins->getDomSpecific(plugin.categorySpecific,"shortName",listChildAttribute));
	if(index!=-1)
		ui->Language->removeItem(index);
}

void OptionDialog::addTheme(PluginsAvailable plugin)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"plugin.name: "+plugin.name);
	ui->Ultracopier_current_theme->addItem(plugin.name,plugin.name);
}

void OptionDialog::removeTheme(PluginsAvailable plugin)
{
	int index=ui->Ultracopier_current_theme->findData(plugin.name);
	if(index!=-1)
		ui->Ultracopier_current_theme->removeItem(index);
}

void OptionDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"retranslate the ui");
		ui->retranslateUi(this);
		index=0;
		loop_size=copyEngineList.size();
		while(index<loop_size)
		{
			if(copyEngineList.at(index).options!=NULL)
				ui->treeWidget->topLevelItem(2)->addChild(copyEngineList.at(index).item);
			else
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("the copy engine %1 have not the options").arg(index));
			index++;
		}
		ui->labelLoadAtSession->setToolTip(tr("Disabled because you have any SessionLoader plugin"));
		ui->LoadAtSessionStarting->setToolTip(tr("Disabled because you have any SessionLoader plugin"));
		ui->ActionOnManualOpen->setItemText(0,tr("Do nothing"));
		ui->ActionOnManualOpen->setItemText(1,tr("Ask source as folder"));
		ui->ActionOnManualOpen->setItemText(2,tr("Ask sources as files"));
		ui->GroupWindowWhen->setItemText(0,tr("Never"));
		ui->GroupWindowWhen->setItemText(1,tr("When source is same"));
		ui->GroupWindowWhen->setItemText(2,tr("When destination is same"));
		ui->GroupWindowWhen->setItemText(3,tr("When source and destination are same"));
		ui->GroupWindowWhen->setItemText(4,tr("When source or destination are same"));
		ui->GroupWindowWhen->setItemText(5,tr("Always"));
		break;
	default:
		break;
	}
}

void OptionDialog::on_treeWidget_itemSelectionChanged()
{
	QList<QTreeWidgetItem *> listSelectedItem=ui->treeWidget->selectedItems();
	if(listSelectedItem.size()!=1)
		return;
	QTreeWidgetItem * selectedItem=listSelectedItem.first();
	if(selectedItem==ui->treeWidget->topLevelItem(0))
		ui->stackedWidget->setCurrentIndex(0);
	else if(selectedItem==ui->treeWidget->topLevelItem(1))
		ui->stackedWidget->setCurrentIndex(1);
	else if(selectedItem==ui->treeWidget->topLevelItem(2))
		ui->stackedWidget->setCurrentIndex(2);
	else if(selectedItem==ui->treeWidget->topLevelItem(3))
		ui->stackedWidget->setCurrentIndex(3);
	else if(selectedItem==ui->treeWidget->topLevelItem(4))
		ui->stackedWidget->setCurrentIndex(4);
	else
	{
		loadedCopyEnginePlugin=0;
		index=0;
		loop_size=copyEngineList.size();
		while(index<loop_size)
		{
			if(copyEngineList.at(index).options!=NULL)
				loadedCopyEnginePlugin++;
			if(copyEngineList.at(index).item->isSelected())
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"ui->stackedWidget->setCurrentIndex("+QString::number(3+loadedCopyEnginePlugin)+")");
				ui->stackedWidget->setCurrentIndex(4+loadedCopyEnginePlugin);
				return;
			}
			index++;
		}
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"selection into option list cat not found");
	}
}

void OptionDialog::on_buttonBox_clicked(QAbstractButton *button)
{
	if(ui->buttonBox->buttonRole(button)==QDialogButtonBox::ResetRole)
		options->queryResetOptions();
	else
		this->close();
}

void OptionDialog::loadOption()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	newOptionValue("Themes",	"Ultracopier_current_theme",	options->getOptionValue("Themes","Ultracopier_current_theme"));
	newOptionValue("Ultracopier",	"ActionOnManualOpen",		options->getOptionValue("Ultracopier","ActionOnManualOpen"));
	newOptionValue("Ultracopier",	"GroupWindowWhen",		options->getOptionValue("Ultracopier","GroupWindowWhen"));
	newOptionValue("Language",	"Language",			options->getOptionValue("Language","Language"));
	newOptionValue("Language",	"Language_autodetect",		options->getOptionValue("Language","Language_autodetect"));
	newOptionValue("SessionLoader",	"LoadAtSessionStarting",	options->getOptionValue("SessionLoader","LoadAtSessionStarting"));
	newOptionValue("CopyListener",	"CatchCopyAsDefault",		options->getOptionValue("CopyListener","CatchCopyAsDefault"));
	newOptionValue("CopyEngine",	"List",				options->getOptionValue("CopyEngine","List"));
	if(resources->getWritablePath()=="")
		ui->checkBox_Log->setEnabled(false);
	else
	{
		newOptionValue("Write_log",	"enabled",			options->getOptionValue("Write_log","enabled"));
		newOptionValue("Write_log",	"file",				options->getOptionValue("Write_log","file"));
		newOptionValue("Write_log",	"transfer",			options->getOptionValue("Write_log","transfer"));
		newOptionValue("Write_log",	"error",			options->getOptionValue("Write_log","error"));
		newOptionValue("Write_log",	"folder",			options->getOptionValue("Write_log","folder"));
		newOptionValue("Write_log",	"transfer_format",		options->getOptionValue("Write_log","transfer_format"));
		newOptionValue("Write_log",	"error_format",			options->getOptionValue("Write_log","error_format"));
		newOptionValue("Write_log",	"folder_format",		options->getOptionValue("Write_log","folder_format"));
		newOptionValue("Write_log",	"sync",				options->getOptionValue("Write_log","sync"));
	}
	on_checkBox_Log_clicked();
	if(plugins->getPluginsByCategory(PluginType_SessionLoader).size()>0)
	{
		ui->labelLoadAtSession->setToolTip("");
		ui->LoadAtSessionStarting->setToolTip("");
		ui->labelLoadAtSession->setEnabled(true);
		ui->LoadAtSessionStarting->setEnabled(true);
	}
	else
	{
		ui->labelLoadAtSession->setToolTip(tr("Disabled because you have any SessionLoader plugin"));
		ui->LoadAtSessionStarting->setToolTip(tr("Disabled because you have any SessionLoader plugin"));
		ui->labelLoadAtSession->setEnabled(false);
		ui->LoadAtSessionStarting->setEnabled(false);
	}
	allPluginsIsLoaded=true;
}

void OptionDialog::newOptionValue(QString group,QString name,QVariant value)
{
	if(group=="Themes")
	{
		if(name=="Ultracopier_current_theme")
		{
			int index=ui->Ultracopier_current_theme->findData(value.toString());
			if(index!=-1)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Themes located: "+value.toString());
				ui->Ultracopier_current_theme->setCurrentIndex(index);
			}
			else
			{
				if(ui->Ultracopier_current_theme->count()>0)
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Default to the current value: "+ui->Ultracopier_current_theme->itemData(ui->Ultracopier_current_theme->currentIndex()).toString());
					options->setOptionValue("Themes","Ultracopier_current_theme",ui->Ultracopier_current_theme->itemData(ui->Ultracopier_current_theme->currentIndex()));
				}
				else
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"No themes: "+value.toString());
			}
		}
	}
	else if(group=="Language")
	{
		if(name=="Language")
		{
			int index=ui->Language->findData(value.toString());
			if(index!=-1)
				ui->Language->setCurrentIndex(index);
			else if(ui->Language->count()>0)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Language in settings: "+value.toString());
				options->setOptionValue("Language","Language",ui->Language->itemData(ui->Language->currentIndex()));
			}
		}
		else if(name=="Language_autodetect")
		{
			ui->Language_autodetect->setChecked(value.toBool());
			ui->Language->setDisabled(value.toBool());
		}
	}
	else if(group=="SessionLoader")
	{
		if(name=="LoadAtSessionStarting")
		{
			ui->LoadAtSessionStarting->setChecked(value.toBool());
		}
	}
	else if(group=="CopyListener")
	{
		if(name=="CatchCopyAsDefault")
		{
			ui->CatchCopyAsDefault->setChecked(value.toBool());
		}
	}
	else if(group=="CopyEngine")
	{
		if(name=="List")
		{
			if(!ignoreCopyEngineListEdition)
			{
				QStringList copyEngine=value.toStringList();
				copyEngine.removeDuplicates();
				int index=0;
				int loop_size=ui->CopyEngineList->count();
				while(index<loop_size)
				{
					copyEngine.removeOne(ui->CopyEngineList->item(index)->text());
					index++;
				}
				ui->CopyEngineList->addItems(copyEngine);
			}
		}
	}
	else if(group=="Write_log")
	{
		if(name=="enabled")
		{
			ui->checkBox_Log->setChecked(value.toBool());
		}
		else if(name=="file")
		{
			ui->lineEditLog_File->setText(value.toString());
		}
		else if(name=="transfer")
		{
			ui->checkBoxLog_transfer->setChecked(value.toBool());
		}
		else if(name=="sync")
		{
			ui->checkBoxLog_sync->setChecked(value.toBool());
		}
		else if(name=="error")
		{
			ui->checkBoxLog_error->setChecked(value.toBool());
		}
		else if(name=="folder")
		{
			ui->checkBoxLog_folder->setChecked(value.toBool());
		}
		else if(name=="transfer_format")
		{
			ui->lineEditLog_transfer_format->setText(value.toString());
		}
		else if(name=="error_format")
		{
			ui->lineEditLog_error_format->setText(value.toString());
		}
		else if(name=="folder_format")
		{
			ui->lineEditLog_folder_format->setText(value.toString());
		}
	}
	else if(group=="Ultracopier")
	{
		if(name=="ActionOnManualOpen")
		{
			ui->ActionOnManualOpen->setCurrentIndex(value.toInt());
		}
		if(name=="GroupWindowWhen")
		{
			ui->GroupWindowWhen->setCurrentIndex(value.toInt());
		}
	}
}

void OptionDialog::on_Ultracopier_current_theme_currentIndexChanged(int index)
{
	if(index!=-1 && allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"data value: "+ui->Ultracopier_current_theme->itemData(index).toString()+", string value: "+ui->Ultracopier_current_theme->itemText(index)+", index: "+QString::number(index));
		options->setOptionValue("Themes","Ultracopier_current_theme",ui->Ultracopier_current_theme->itemData(index));
	}
}

void OptionDialog::on_Language_currentIndexChanged(int index)
{
	if(index!=-1 && allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"data value: "+ui->Language->itemData(index).toString()+", string value: "+ui->Language->itemText(index)+", index: "+QString::number(index));
		options->setOptionValue("Language","Language",ui->Language->itemData(index));
	}
}

void OptionDialog::on_Language_autodetect_toggled(bool checked)
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Language","Language_autodetect",checked);
	}
}

void OptionDialog::on_CatchCopyAsDefault_toggled(bool checked)
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("CopyListener","CatchCopyAsDefault",checked);
	}
}

void OptionDialog::on_LoadAtSessionStarting_toggled(bool checked)
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("SessionLoader","LoadAtSessionStarting",checked);
	}
}

void OptionDialog::on_CopyEngineList_itemSelectionChanged()
{
	if(ui->CopyEngineList->selectedItems().size()!=0 && ui->CopyEngineList->count()>1)
	{
		ui->toolButtonUp->setEnabled(true);
		ui->toolButtonDown->setEnabled(true);
	}
	else
	{
		ui->toolButtonUp->setEnabled(false);
		ui->toolButtonDown->setEnabled(false);
	}
}

void OptionDialog::on_toolButtonDown_clicked()
{
	QListWidgetItem *item=ui->CopyEngineList->selectedItems().first();
	int position=ui->CopyEngineList->row(item);
	if((position+1)<ui->CopyEngineList->count())
	{
		QString text=item->text();
		item->setSelected(false);
		delete item;
		ui->CopyEngineList->insertItem(position+1,text);
		ui->CopyEngineList->item(position+1)->setSelected(true);
		ignoreCopyEngineListEdition=true;
		options->setOptionValue("CopyEngine","List",copyEngineStringList());
		ignoreCopyEngineListEdition=false;
	}
}

void OptionDialog::on_toolButtonUp_clicked()
{
	QListWidgetItem *item=ui->CopyEngineList->selectedItems().first();
	int position=ui->CopyEngineList->row(item);
	if(position>0)
	{
		QString text=item->text();
		item->setSelected(false);
		delete item;
		ui->CopyEngineList->insertItem(position-1,text);
		ui->CopyEngineList->item(position-1)->setSelected(true);
		ignoreCopyEngineListEdition=true;
		options->setOptionValue("CopyEngine","List",copyEngineStringList());
		ignoreCopyEngineListEdition=false;
	}
}

QStringList OptionDialog::copyEngineStringList()
{
	QStringList newList;
	int index=0;
	while(index<ui->CopyEngineList->count())
	{
		newList << ui->CopyEngineList->item(index)->text();
		index++;
	}
	return newList;
}

void OptionDialog::newThemeOptions(QWidget* theNewOptionsWidget,bool isLoaded,bool havePlugin)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	while(ui->stackedWidgetThemes->count()>3)
		ui->stackedWidgetThemes->removeWidget(ui->stackedWidgetThemes->widget(3));
	if(theNewOptionsWidget!=NULL)
	{
		ui->stackedWidgetThemes->addWidget(theNewOptionsWidget);
		ui->stackedWidgetThemes->setCurrentWidget(theNewOptionsWidget);
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"set the last page");
	}
	else
	{
		if(isLoaded)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"set the page, no option for this plugin");
			ui->stackedWidgetThemes->setCurrentIndex(1);
		}
		else if(!havePlugin)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"set the page, no plugin");
			ui->stackedWidgetThemes->setCurrentIndex(2);
		}
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"set the page, unable to load plugin options");
			ui->stackedWidgetThemes->setCurrentIndex(0);
		}
	}
}

void OptionDialog::removeCopyEngine(PluginsAvailable plugin)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	removeCopyEngineWidget(plugin.name);
}

void OptionDialog::addCopyEngineWidget(QString name,QWidget * options)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start: %1").arg(name));
	index=0;
	loop_size=copyEngineList.size();
	while(index<loop_size)
	{
		if(copyEngineList.at(index).name==name)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"already found: "+name);
			return;
		}
		index++;
	}
	//add to real list
	pluginCopyEngine temp;
	temp.name=name;
	temp.options=options;
	temp.item=new QTreeWidgetItem(QStringList() << name);
	copyEngineList << temp;
	//add the specific options
	ui->treeWidget->topLevelItem(2)->addChild(copyEngineList.at(index).item);
	ui->stackedWidget->addWidget(options);
	//but can loaded by the previous options!
	index=0;
	loop_size=ui->CopyEngineList->count();
	while(index<loop_size)
	{
		if(ui->CopyEngineList->item(index)->text()==name)
			break;
		index++;
	}
	if(index==loop_size)
		ui->CopyEngineList->addItems(QStringList() << name);
}

void OptionDialog::removeCopyEngineWidget(QString name)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	index=0;
	loop_size=copyEngineList.size();
	while(index<loop_size)
	{
		if(copyEngineList.at(index).name==name)
		{
			if(copyEngineList.at(index).options!=NULL)
				ui->stackedWidget->removeWidget(copyEngineList.at(index).options);
			if(copyEngineList.at(index).item->isSelected())
			{
				copyEngineList.at(index).item->setSelected(false);
				ui->treeWidget->topLevelItem(0)->setSelected(true);
			}
			delete copyEngineList.at(index).item;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"ui->stackedWidget->count(): "+QString::number(ui->stackedWidget->count()));
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"not found is the list: "+name);
}

void OptionDialog::on_pluginList_itemSelectionChanged()
{
	if(ui->pluginList->selectedItems().size()==0)
	{
		ui->pluginRemove->setEnabled(false);
		ui->pluginInformation->setEnabled(false);
	}
	else
	{
		treeWidgetItem=ui->pluginList->selectedItems().first();
		index=0;
		loop_size=pluginLink.size();
		while(index<loop_size)
		{
			if(pluginLink.at(index).item==treeWidgetItem)
			{
				ui->pluginRemove->setEnabled(pluginLink.at(index).isWritable);
				ui->pluginInformation->setEnabled(true);
				return;
			}
			index++;
		}
	}
}

void OptionDialog::on_pluginRemove_clicked()
{
	treeWidgetItem=ui->pluginList->selectedItems().first();
	index=0;
	loop_size=pluginLink.size();
	while(index<loop_size)
	{
		if(pluginLink.at(index).item==treeWidgetItem)
		{
			plugins->removeThePluginSelected(pluginLink.at(index).path);
			return;
		}
		index++;
	}
}

void OptionDialog::on_pluginInformation_clicked()
{
	treeWidgetItem=ui->pluginList->selectedItems().first();
	index=0;
	loop_size=pluginLink.size();
	while(index<loop_size)
	{
		if(pluginLink.at(index).item==treeWidgetItem)
		{
			plugins->showInformation(pluginLink.at(index).path);
			return;
		}
		index++;
	}
}

void OptionDialog::on_pluginAdd_clicked()
{
	plugins->addPlugin(defaultImportBackend);
}

void OptionDialog::on_checkBox_Log_clicked()
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Write_log","enabled",ui->checkBox_Log->isChecked());
	}
	ui->lineEditLog_transfer_format->setEnabled(ui->checkBoxLog_transfer->isChecked() && ui->checkBox_Log->isChecked());
	ui->lineEditLog_error_format->setEnabled(ui->checkBoxLog_error->isChecked() && ui->checkBox_Log->isChecked());
	ui->lineEditLog_folder_format->setEnabled(ui->checkBoxLog_folder->isChecked() && ui->checkBox_Log->isChecked());
}

void OptionDialog::on_lineEditLog_File_editingFinished()
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Write_log","file",ui->lineEditLog_File->text());
	}
}

void OptionDialog::on_lineEditLog_transfer_format_editingFinished()
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Write_log","transfer_format",ui->lineEditLog_transfer_format->text());
	}
}

void OptionDialog::on_lineEditLog_error_format_editingFinished()
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Write_log","error_format",ui->lineEditLog_error_format->text());
	}
}

void OptionDialog::on_checkBoxLog_transfer_clicked()
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Write_log","transfer",ui->checkBoxLog_transfer->isChecked());
	}
}

void OptionDialog::on_checkBoxLog_error_clicked()
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Write_log","error",ui->checkBoxLog_error->isChecked());
	}
}

void OptionDialog::on_checkBoxLog_folder_clicked()
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Write_log","folder",ui->checkBoxLog_folder->isChecked());
	}
}

void OptionDialog::on_pushButton_clicked()
{
	QString file=QFileDialog::getSaveFileName(this,tr("Save logs as: "),resources->getWritablePath());
	if(file!="")
	{
		ui->lineEditLog_File->setText(file);
		on_lineEditLog_File_editingFinished();
	}
}

void OptionDialog::on_checkBoxLog_sync_clicked()
{
	if(allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		options->setOptionValue("Write_log","sync",ui->checkBoxLog_sync->isChecked());
	}
}

void OptionDialog::on_ActionOnManualOpen_currentIndexChanged(int index)
{
	if(index!=-1 && allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"data value: "+ui->ActionOnManualOpen->itemData(index).toString()+", string value: "+ui->ActionOnManualOpen->itemText(index)+", index: "+QString::number(index));
		options->setOptionValue("Ultracopier","ActionOnManualOpen",index);
	}
}

void OptionDialog::on_GroupWindowWhen_currentIndexChanged(int index)
{
	if(index!=-1 && allPluginsIsLoaded)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"data value: "+ui->GroupWindowWhen->itemData(index).toString()+", string value: "+ui->GroupWindowWhen->itemText(index)+", index: "+QString::number(index));
		options->setOptionValue("Ultracopier","GroupWindowWhen",index);
	}
}
