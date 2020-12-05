/*
    Copyright (C) 2019 Doug McLain

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "dudestar.h"
#include "audioengine.h"
#include "serialambe.h"
#include "ui_dudestar.h"
#include "SHA256.h"
#include "crs129.h"
#include "cbptc19696.h"
#include "cgolay2087.h"
#include <iostream>
#include <QMessageBox>
#include <QFileDialog>
#include <QSerialPortInfo>
#include <time.h>

#define ENDLINE "\n"

#define LOBYTE(w)			((uint8_t)(uint16_t)(w & 0x00FF))
#define HIBYTE(w)			((uint8_t)((((uint16_t)(w)) >> 8) & 0xFF))
#define LOWORD(dw)			((uint16_t)(uint32_t)(dw & 0x0000FFFF))
#define HIWORD(dw)			((uint16_t)((((uint32_t)(dw)) >> 16) & 0xFFFF))
#define DEBUG
#define CHANNEL_FRAME_TX    0x1001
#define CODEC_FRAME_TX      0x1002
#define CHANNEL_FRAME_RX    0x2001
#define CODEC_FRAME_RX      0x2002

DudeStar::DudeStar(QWidget *parent) :
    QMainWindow(parent),
	ui(new Ui::DudeStar),
	m_update_host_files(false)
{
	dmrslot = 2;
	dmrcc = 1;
	dmrcalltype = 0;
	muted = false;
	config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#ifndef Q_OS_WIN
	config_path += "/dudestar";
#endif
    ui->setupUi(this);
    init_gui();
    connect_status = DISCONNECTED;
	user_data.resize(21);
	check_host_files();
	process_settings();
	srand(time(0));
}

DudeStar::~DudeStar()
{
	QFile f(config_path + "/settings.conf");
	f.open(QIODevice::WriteOnly);
	QTextStream stream(&f);
	stream << "PLAYBACK:" << ui->AudioOutCombo->currentText() << ENDLINE;
	stream << "CAPTURE:" << ui->AudioInCombo->currentText() << ENDLINE;
	stream << "MODE:" << ui->modeCombo->currentText() << ENDLINE;
	stream << "REFHOST:" << saved_refhost << ENDLINE;
	stream << "DCSHOST:" << saved_dcshost << ENDLINE;
	stream << "XRFHOST:" << saved_xrfhost << ENDLINE;
	stream << "YSFHOST:" << saved_ysfhost << ENDLINE;
	stream << "DMRHOST:" << saved_dmrhost << ENDLINE;
	stream << "P25HOST:" << saved_p25host << ENDLINE;
	stream << "NXDNHOST:" << saved_nxdnhost << ENDLINE;
	stream << "M17HOST:" << saved_m17host << ENDLINE;
	stream << "MODULE:" << ui->comboMod->currentText() << ENDLINE;
	stream << "CALLSIGN:" << ui->callsignEdit->text() << ENDLINE;
	stream << "DMRID:" << ui->dmridEdit->text() << ENDLINE;
	stream << "DMRPASSWORD:" << ui->dmrpwEdit->text() << ENDLINE;
	stream << "DMRTGID:" << ui->dmrtgEdit->text() << ENDLINE;
	stream << "MYCALL:" << ui->mycallEdit->text().simplified() << ENDLINE;
	stream << "URCALL:" << ui->urcallEdit->text().simplified() << ENDLINE;
	stream << "RPTR1:" << ui->rptr1Edit->text().simplified() << ENDLINE;
	stream << "RPTR2:" << ui->rptr2Edit->text().simplified() << ENDLINE;
	stream << "USRTXT:" << ui->usertxtEdit->text() << ENDLINE;
	f.close();
	delete ui;
}

void DudeStar::about()
{
    QMessageBox::about(this, tr("About DUDE-Star"),
					   tr("DUDE-Star git build %1\nCopyright (C) 2019 Doug McLain AD8DP\n\n"
                          "This program is free software; you can redistribute it"
                          "and/or modify it under the terms of the GNU General Public "
                          "License as published by the Free Software Foundation; "
                          "version 2.\n\nThis program is distributed in the hope that it "
                          "will be useful, but WITHOUT ANY WARRANTY; without even "
                          "the implied warranty of MERCHANTABILITY or FITNESS FOR A "
                          "PARTICULAR PURPOSE. See the GNU General Public License for "
                          "more details.\n\nYou should have received a copy of the GNU "
                          "General Public License along with this program. "
						  "If not, see <http://www.gnu.org/licenses/>").arg(VERSION_NUMBER));
}

void DudeStar::init_gui()
{
	QPalette palette;
	palette.setColor(QPalette::Window, QColor(53, 53, 53));
	palette.setColor(QPalette::WindowText, Qt::white);
	palette.setColor(QPalette::Base, QColor(25, 25, 25));
	palette.setColor(QPalette::Disabled, QPalette::Base, QColor(53, 53, 53));
	palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
	palette.setColor(QPalette::ToolTipBase, Qt::black);
	palette.setColor(QPalette::ToolTipText, Qt::white);
	palette.setColor(QPalette::Text, Qt::white);
	palette.setColor(QPalette::Disabled, QPalette::Text, QColor(150, 150, 150));
	palette.setColor(QPalette::Button, QColor(53, 53, 53));
	palette.setColor(QPalette::ButtonText, Qt::white);
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 150, 150));
	palette.setColor(QPalette::BrightText, Qt::red);
	palette.setColor(QPalette::Link, QColor(42, 130, 218));
	palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	palette.setColor(QPalette::HighlightedText, Qt::black);
	qApp->setPalette(palette);
	status_txt = new QLabel("Not connected");
	tts_voices = new QButtonGroup();
	tts_voices->addButton(ui->checkBoxTTSOff, 0);
	tts_voices->addButton(ui->checkBoxKal, 1);
	tts_voices->addButton(ui->checkBoxRms, 2);
	tts_voices->addButton(ui->checkBoxAwb, 3);
	tts_voices->addButton(ui->checkBoxSlt, 4);
#ifdef USE_FLITE
	connect(tts_voices, SIGNAL(buttonClicked(int)), this, SLOT(tts_changed(int)));
	connect(ui->TTSEdit, SIGNAL(textChanged(QString)), this, SLOT(tts_text_changed(QString)));
#endif
#ifndef USE_FLITE
	ui->checkBoxTTSOff->hide();
	ui->checkBoxKal->hide();
	ui->checkBoxRms->hide();
	ui->checkBoxAwb->hide();
	ui->checkBoxSlt->hide();
	ui->TTSEdit->hide();
#endif
	ui->txButton->setAutoFillBackground(true);
	ui->txButton->setStyleSheet("QPushButton:enabled { background-color: rgb(128, 195, 66); color: rgb(0,0,0); } QPushButton:pressed { background-color: rgb(180, 0, 0); color: rgb(0,0,0); }");
	ui->txButton->update();
	ui->checkBoxTTSOff->setCheckState(Qt::Checked);
	ui->volumeSlider->setRange(0, 100);
	ui->volumeSlider->setValue(100);
	ui->involSlider->setRange(0, 100);
	ui->involSlider->setValue(100);
	ui->txButton->setDisabled(true);
	m17rates = new QButtonGroup();
	m17rates->addButton(ui->m17VoiceFull, 1);
	m17rates->addButton(ui->m17VoiceData, 0);
	ui->m17VoiceFull->setChecked(true);
	connect(m17rates, SIGNAL(buttonClicked(int)), this, SLOT(m17_rate_changed(int)));
	//ui->m17VoiceData->setEnabled(false);
	connect(ui->dmrtgEdit, SIGNAL(textChanged(QString)), this, SLOT(tgid_text_changed(QString)));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui->actionUpdate_DMR_IDs, SIGNAL(triggered()), this, SLOT(update_dmr_ids()));
	connect(ui->actionUpdate_host_files, SIGNAL(triggered()), this, SLOT(update_host_files()));
    connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(process_connect()));
	connect(ui->muteButton, SIGNAL(clicked()), this, SLOT(process_mute_button()));
	connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(process_volume_changed(int)));
	connect(ui->inmuteButton, SIGNAL(clicked()), this, SLOT(process_input_mute_button()));
	connect(ui->involSlider, SIGNAL(valueChanged(int)), this, SLOT(process_input_volume_changed(int)));
	ui->statusBar->insertPermanentWidget(0, status_txt, 1);
	//connect(ui->checkBoxSWRX, SIGNAL(stateChanged(int)), this, SLOT(swrx_state_changed(int)));
	connect(ui->checkBoxSWTX, SIGNAL(stateChanged(int)), this, SLOT(swtx_state_changed(int)));
    ui->rptr1->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->rptr2->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->mycall->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->urcall->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->streamid->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->usertxt->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->modeCombo->addItem("REF");
	ui->modeCombo->addItem("DCS");
	ui->modeCombo->addItem("XRF");
	ui->modeCombo->addItem("YSF");
	ui->modeCombo->addItem("DMR");
	ui->modeCombo->addItem("P25");
	ui->modeCombo->addItem("NXDN");
	ui->modeCombo->addItem("M17");
	ui->dmrccEdit->setText(QString::number(dmrcc));
	ui->dmrslotEdit->setText(QString::number(dmrslot));
	connect(ui->modeCombo, SIGNAL(currentTextChanged(const QString &)), this, SLOT(process_mode_change(const QString &)));
	connect(ui->hostCombo, SIGNAL(currentTextChanged(const QString &)), this, SLOT(process_host_change(const QString &)));

	for(char m = 0x41; m < 0x5b; ++m){
		ui->comboMod->addItem(QString(m));
	}

	ui->hostCombo->setEditable(true);
	ui->dmrtgEdit->setEnabled(false);

	discover_vocoders();
	ui->AudioOutCombo->addItem("OS Default");
	ui->AudioOutCombo->addItems(AudioEngine::discover_audio_devices(AUDIO_OUT));
	ui->AudioInCombo->addItem("OS Default");
	ui->AudioInCombo->addItems(AudioEngine::discover_audio_devices(AUDIO_IN));
}

void DudeStar::download_file(QString f)
{
	HttpManager *http = new HttpManager(f);
	QThread *httpThread = new QThread;
	http->moveToThread(httpThread);
	connect(httpThread, SIGNAL(started()), http, SLOT(process()));
	connect(http, SIGNAL(file_downloaded(QString)), this, SLOT(file_downloaded(QString)));
	connect(httpThread, SIGNAL(finished()), http, SLOT(deleteLater()));
	httpThread->start();
}

void DudeStar::file_downloaded(QString filename)
{
	qDebug() << "DudeStar::file_downloaded() " << filename;
	QString m = ui->modeCombo->currentText();
	{
		if(filename == "dplus.txt" && m == "REF"){
			process_ref_hosts();
		}
		else if(filename == "dextra.txt" && m == "XRF"){
			process_xrf_hosts();
		}
		else if(filename == "dcs.txt" && m == "DCS"){
			process_dcs_hosts();
		}
		else if(filename == "YSFHosts.txt" && m == "YSF"){
			process_ysf_hosts();
		}
		else if(filename == "FCSHosts.txt" && m == "YSF"){
			process_ysf_hosts();
		}
		else if(filename == "P25Hosts.txt" && m == "P25"){
			process_p25_hosts();
		}
		else if(filename == "DMRHosts.txt" && m == "DMR"){
			process_dmr_hosts();
		}
		else if(filename == "NXDNHosts.txt" && m == "NXDN"){
			process_nxdn_hosts();
		}
		else if(filename == "M17Hosts.txt" && m == "M17"){
			process_m17_hosts();
		}
		else if(filename == "DMRIDs.dat"){
			process_dmr_ids();
		}
		else if(filename == "NXDN.csv"){
			process_nxdn_ids();
		}
	}
}

void DudeStar::process_host_change(const QString &h)
{
	if(ui->modeCombo->currentText().simplified() == "REF"){
		saved_refhost = h.simplified();
	}
	if(ui->modeCombo->currentText().simplified() == "DCS"){
		saved_dcshost = h.simplified();
	}
	if(ui->modeCombo->currentText().simplified() == "XRF"){
		saved_xrfhost = h.simplified();
	}
	if(ui->modeCombo->currentText().simplified() == "YSF"){
		saved_ysfhost = h.simplified();
	}
	if(ui->modeCombo->currentText().simplified() == "DMR"){
		saved_dmrhost = h.simplified();
	}
	if(ui->modeCombo->currentText().simplified() == "P25"){
		saved_p25host = h.simplified();
	}
	if(ui->modeCombo->currentText().simplified() == "NXDN"){
		saved_nxdnhost = h.simplified();
	}
	if(ui->modeCombo->currentText().simplified() == "M17"){
		saved_m17host = h.simplified();
	}
}

void DudeStar::process_mode_change(const QString &m)
{
	if(m == "REF"){
		process_ref_hosts();
		ui->comboMod->setEnabled(true);
		ui->dmridEdit->setEnabled(false);
		ui->dmrpwEdit->setEnabled(false);
		ui->dmrtgEdit->setEnabled(false);
		ui->dmrccEdit->setEnabled(false);
		ui->dmrslotEdit->setEnabled(false);
		ui->checkBoxDMRPC->setEnabled(false);
		ui->mycallEdit->setEnabled(true);
		ui->urcallEdit->setEnabled(true);
		ui->rptr1Edit->setEnabled(true);
		ui->rptr2Edit->setEnabled(true);
		ui->usertxtEdit->setEnabled(true);
		//ui->m17VoiceFull->setEnabled(false);
		//ui->m17VoiceData->setEnabled(false);
		ui->label_1->setText("MYCALL");
		ui->label_2->setText("URCALL");
		ui->label_3->setText("RPTR1");
		ui->label_4->setText("RPTR2");
		ui->label_5->setText("Stream ID");
		ui->label_6->setText("User txt");
	}
	if(m == "DCS"){
		process_dcs_hosts();
		ui->comboMod->setEnabled(true);
		ui->dmridEdit->setEnabled(false);
		ui->dmrpwEdit->setEnabled(false);
		ui->dmrtgEdit->setEnabled(false);
		ui->dmrccEdit->setEnabled(false);
		ui->dmrslotEdit->setEnabled(false);
		ui->checkBoxDMRPC->setEnabled(false);
		ui->mycallEdit->setEnabled(true);
		ui->urcallEdit->setEnabled(true);
		ui->rptr1Edit->setEnabled(true);
		ui->rptr2Edit->setEnabled(true);
		ui->usertxtEdit->setEnabled(true);
		//ui->m17VoiceFull->setEnabled(false);
		//ui->m17VoiceData->setEnabled(false);
		ui->label_1->setText("MYCALL");
		ui->label_2->setText("URCALL");
		ui->label_3->setText("RPTR1");
		ui->label_4->setText("RPTR2");
		ui->label_5->setText("Stream ID");
		ui->label_6->setText("User txt");
	}
	if(m == "XRF"){
		process_xrf_hosts();
		ui->comboMod->setEnabled(true);
		ui->dmridEdit->setEnabled(false);
		ui->dmrpwEdit->setEnabled(false);
		ui->dmrtgEdit->setEnabled(false);
		ui->dmrccEdit->setEnabled(false);
		ui->dmrslotEdit->setEnabled(false);
		ui->checkBoxDMRPC->setEnabled(false);
		ui->mycallEdit->setEnabled(true);
		ui->urcallEdit->setEnabled(true);
		ui->rptr1Edit->setEnabled(true);
		ui->rptr2Edit->setEnabled(true);
		ui->usertxtEdit->setEnabled(true);
		//ui->m17VoiceFull->setEnabled(false);
		//ui->m17VoiceData->setEnabled(false);
		ui->label_1->setText("MYCALL");
		ui->label_2->setText("URCALL");
		ui->label_3->setText("RPTR1");
		ui->label_4->setText("RPTR2");
		ui->label_5->setText("Stream ID");
		ui->label_6->setText("User txt");
	}
	else if(m == "YSF"){
		process_ysf_hosts();
		ui->comboMod->setEnabled(false);
		ui->dmridEdit->setEnabled(false);
		ui->dmrpwEdit->setEnabled(false);
		ui->dmrtgEdit->setEnabled(false);
		ui->dmrccEdit->setEnabled(false);
		ui->dmrslotEdit->setEnabled(false);
		ui->checkBoxDMRPC->setEnabled(false);
		ui->mycallEdit->setEnabled(false);
		ui->urcallEdit->setEnabled(false);
		ui->rptr1Edit->setEnabled(false);
		ui->rptr2Edit->setEnabled(false);
		ui->usertxtEdit->setEnabled(false);
		//ui->m17VoiceFull->setEnabled(false);
		//ui->m17VoiceData->setEnabled(false);
		ui->label_1->setText("Gateway");
		ui->label_2->setText("Callsign");
		ui->label_3->setText("Dest");
		ui->label_4->setText("Type");
		ui->label_5->setText("Path");
		ui->label_6->setText("Frame#");
	}
	else if(m == "DMR"){
		process_dmr_hosts();
		ui->comboMod->setEnabled(false);
		ui->dmridEdit->setEnabled(true);
		ui->dmrpwEdit->setEnabled(true);
		ui->dmrtgEdit->setEnabled(true);
		ui->dmrccEdit->setEnabled(true);
		ui->dmrslotEdit->setEnabled(true);
		ui->checkBoxDMRPC->setEnabled(true);
		ui->mycallEdit->setEnabled(false);
		ui->urcallEdit->setEnabled(false);
		ui->rptr1Edit->setEnabled(false);
		ui->rptr2Edit->setEnabled(false);
		ui->usertxtEdit->setEnabled(false);
		//ui->m17VoiceFull->setEnabled(false);
		//ui->m17VoiceData->setEnabled(false);
		ui->label_1->setText("Callsign");
		ui->label_2->setText("SrcID");
		ui->label_3->setText("DestID");
		ui->label_4->setText("GWID");
		ui->label_5->setText("Seq#");
		ui->label_6->setText("");
	}
	else if(m == "P25"){
		process_p25_hosts();
		ui->comboMod->setEnabled(false);
		ui->dmridEdit->setEnabled(true);
		ui->dmrpwEdit->setEnabled(false);
		ui->dmrtgEdit->setEnabled(true);
		ui->dmrccEdit->setEnabled(false);
		ui->dmrslotEdit->setEnabled(false);
		ui->checkBoxDMRPC->setEnabled(false);
		ui->mycallEdit->setEnabled(false);
		ui->urcallEdit->setEnabled(false);
		ui->rptr1Edit->setEnabled(false);
		ui->rptr2Edit->setEnabled(false);
		ui->usertxtEdit->setEnabled(false);
		//ui->m17VoiceFull->setEnabled(false);
		//ui->m17VoiceData->setEnabled(false);
		ui->label_1->setText("Callsign");
		ui->label_2->setText("SrcID");
		ui->label_3->setText("DestID");
		ui->label_4->setText("GWID");
		ui->label_5->setText("Seq#");
		ui->label_6->setText("");
	}
	else if(m == "NXDN"){
		process_nxdn_hosts();
		ui->comboMod->setEnabled(false);
		ui->dmridEdit->setEnabled(true);
		ui->dmrtgEdit->setEnabled(false);
		ui->dmrccEdit->setEnabled(false);
		ui->dmrslotEdit->setEnabled(false);
		ui->checkBoxDMRPC->setEnabled(false);
		ui->mycallEdit->setEnabled(false);
		ui->urcallEdit->setEnabled(false);
		ui->rptr1Edit->setEnabled(false);
		ui->rptr2Edit->setEnabled(false);
		ui->usertxtEdit->setEnabled(false);
		//ui->m17VoiceFull->setEnabled(false);
		//ui->m17VoiceData->setEnabled(false);
		ui->label_1->setText("Callsign");
		ui->label_2->setText("SrcID");
		ui->label_3->setText("DestID");
		ui->label_4->setText("");
		ui->label_5->setText("Seq#");
		ui->label_6->setText("");
	}
	else if(m == "M17"){
		process_m17_hosts();
		ui->comboMod->setEnabled(true);
		ui->dmridEdit->setEnabled(false);
		ui->dmrtgEdit->setEnabled(false);
		ui->dmrccEdit->setEnabled(false);
		ui->dmrslotEdit->setEnabled(false);
		ui->checkBoxDMRPC->setEnabled(false);
		ui->mycallEdit->setEnabled(false);
		ui->urcallEdit->setEnabled(false);
		ui->rptr1Edit->setEnabled(false);
		ui->rptr2Edit->setEnabled(false);
		ui->usertxtEdit->setEnabled(false);
		//ui->m17VoiceFull->setEnabled(true);
		//ui->m17VoiceData->setEnabled(true);
		ui->label_1->setText("SrcID");
		ui->label_2->setText("DstID");
		ui->label_3->setText("Type");
		ui->label_4->setText("Frame #");
		ui->label_5->setText("Stream ID");
		ui->label_6->setText("");
	}
}

void DudeStar::tts_changed(int b)
{
	qDebug() << "tts_changed() called";
	emit input_source_changed(b, ui->TTSEdit->text());
}

void DudeStar::tgid_text_changed(QString s)
{
	qDebug() << "dmrid_text_changed() called s == " << s;
	emit dmr_tgid_changed(s.toUInt());
}

void DudeStar::tts_text_changed(QString s)
{
	emit input_source_changed(tts_voices->checkedId(), s);
}

void DudeStar::m17_rate_changed(int r)
{
	emit rate_changed(r);
}

void DudeStar::swrx_state_changed(int s)
{
	if(s == Qt::Unchecked){
		hwrx = true;
	}
	else{
		hwrx = false;
	}
}

void DudeStar::swtx_state_changed(int s)
{
	if(s == Qt::Unchecked){
		hwtx = true;
	}
	else{
		hwtx = false;
	}
}

void DudeStar::process_ref_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/dplus.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/dplus.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					//ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":20001");
					hostmap[ll.at(0).simplified()] = ll.at(1) + ":20001";
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_refhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		download_file("/dplus.txt");
	}
}

void DudeStar::process_dcs_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/dcs.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/dcs.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					//ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":30051");
					hostmap[ll.at(0).simplified()] = ll.at(1) + ":30051";
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_dcshost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		download_file("/dcs.txt");
	}
}

void DudeStar::process_xrf_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/dextra.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/dextra.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					//ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":30001");
					hostmap[ll.at(0).simplified()] = ll.at(1) + ":30001";
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_xrfhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		download_file("/dextra.txt");
	}
}

void DudeStar::process_ysf_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/YSFHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/YSFHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split(';');
				if(ll.size() > 4){
					//ui->hostCombo->addItem(ll.at(1).simplified() + " - " + ll.at(2).simplified(), ll.at(3) + ":" + ll.at(4));
					hostmap[ll.at(1).simplified() + " - " + ll.at(2).simplified()] = ll.at(3) + ":" + ll.at(4);
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		if(saved_ysfhost.left(3) != "FCS"){
			int i = ui->hostCombo->findText(saved_ysfhost);
			ui->hostCombo->setCurrentIndex(i);
		}
		ui->hostCombo->blockSignals(false);
		process_fcs_rooms();
	}
	else{
		download_file("/YSFHosts.txt");
	}
}

void DudeStar::process_fcs_rooms()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/FCSHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/FCSHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			//ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split(';');
				if(ll.size() > 4){
					//ui->hostCombo->addItem(ll.at(0).simplified() + " - " + ll.at(1).simplified(), ll.at(2).left(6).toLower() + ".xreflector.net:62500");
					hostmap[ll.at(0).simplified() + " - " + ll.at(1).simplified()] = ll.at(2).left(6).toLower() + ".xreflector.net:62500";
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		if(saved_ysfhost.left(3) == "FCS"){
			int i = ui->hostCombo->findText(saved_ysfhost);
			ui->hostCombo->setCurrentIndex(i);
		}
		ui->hostCombo->blockSignals(false);
	}
	else{
		download_file("/FCSHosts.txt");
	}
}

void DudeStar::process_dmr_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/DMRHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/DMRHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 4){
					//qDebug() << ll.at(0).simplified() << " " <<  ll.at(2) + ":" + ll.at(4);
					if( (ll.at(0).simplified() != "DMRGateway")
					 && (ll.at(0).simplified() != "DMR2YSF")
					 && (ll.at(0).simplified() != "DMR2NXDN"))
					{
						//ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(2) + ":" + ll.at(4) + ":" + ll.at(3));
						hostmap[ll.at(0).simplified()] = ll.at(2) + ":" + ll.at(4) + ":" + ll.at(3);
					}
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		//qDebug() << "saved_dmrhost == " << saved_dmrhost;
		int i = ui->hostCombo->findText(saved_dmrhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		download_file("/DMRHosts.txt");
	}
}

void DudeStar::process_p25_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/P25Hosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/P25Hosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 2){
					//qDebug() << ll.at(0).simplified() << " " <<  ll.at(2) + ":" + ll.at(4);
					//ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":" + ll.at(2));
					hostmap[ll.at(0).simplified()] = ll.at(1) + ":" + ll.at(2);
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		//qDebug() << "saved_p25Host == " << saved_p25host;
		int i = ui->hostCombo->findText(saved_p25host);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		download_file("/P25Hosts.txt");
	}
}

void DudeStar::process_nxdn_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/NXDNHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/NXDNHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 2){
					//qDebug() << ll.at(0).simplified() << " " <<  ll.at(2) + ":" + ll.at(4);
					//ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":" + ll.at(2));
					hostmap[ll.at(0).simplified()] = ll.at(1) + ":" + ll.at(2);
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_nxdnhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		download_file("/NXDNHosts.txt");
	}
}

void DudeStar::process_m17_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/M17Hosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->hostCombo->blockSignals(true);
		QFile f(config_path + "/M17Hosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->hostCombo->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 2){
					hostmap[ll.at(0).simplified()] = ll.at(1) + ":" + ll.at(2);
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->hostCombo->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();

		int i = ui->hostCombo->findText(saved_m17host);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		download_file("/M17Hosts.txt");
	}
}

void DudeStar::update_host_files()
{
	m_update_host_files = true;
	check_host_files();
}

void DudeStar::check_host_files()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

	QFileInfo check_file(config_path + "/dplus.txt");
	if( (!check_file.exists() && !(check_file.isFile())) || m_update_host_files ){
		download_file("/dplus.txt");
	}

	check_file.setFile(config_path + "/dextra.txt");
	if( (!check_file.exists() && !check_file.isFile() ) || m_update_host_files  ){
		download_file("/dextra.txt");
	}

	check_file.setFile(config_path + "/dcs.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file( "/dcs.txt");
	}

	check_file.setFile(config_path + "/YSFHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/YSFHosts.txt");
	}

	check_file.setFile(config_path + "/FCSHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/FCSHosts.txt");
	}

	check_file.setFile(config_path + "/DMRHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/DMRHosts.txt");
	}

	check_file.setFile(config_path + "/P25Hosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/P25Hosts.txt");
	}

	check_file.setFile(config_path + "/NXDNHosts.txt");
	if((!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/NXDNHosts.txt");
	}

	check_file.setFile(config_path + "/M17Hosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/M17Hosts.txt");
	}

	check_file.setFile(config_path + "/DMRIDs.dat");
	if(!check_file.exists() && !check_file.isFile()){
		download_file("/DMRIDs.dat");
	}
	else {
		process_dmr_ids();
	}

	check_file.setFile(config_path + "/NXDN.csv");
	if(!check_file.exists() && !check_file.isFile()){
		download_file("/NXDN.csv");
	}
	else{
		process_nxdn_ids();
	}
	m_update_host_files = false;
	//process_mode_change(ui->modeCombo->currentText().simplified());
}

void DudeStar::process_dmr_ids()
{
	QFileInfo check_file(config_path + "/DMRIDs.dat");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/DMRIDs.dat");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				//qDebug() << ll.at(0).simplified() << " " <<  ll.at(2) + ":" + ll.at(4);
				m_dmrids[ll.at(0).toUInt()] = ll.at(1);
			}
		}
		f.close();
	}
	else{
		download_file("/DMRIDs.dat");
	}
}

void DudeStar::update_dmr_ids()
{
	QFileInfo check_file(config_path + "/DMRIDs.dat");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/DMRIDs.dat");
		f.remove();
	}
	process_dmr_ids();
}

void DudeStar::process_nxdn_ids()
{
	QFileInfo check_file(config_path + "/NXDN.csv");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/NXDN.csv");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(',');
				if(ll.size() > 1){
					//qDebug() << ll.at(0).simplified() << " " <<  ll.at(1) + ":" + ll.at(2);
					nxdnids[ll.at(0).toUInt()] = ll.at(1);
				}
			}
		}
		f.close();
	}
	else{
		download_file("/NXDN.csv");
	}
}

void DudeStar::update_nxdn_ids()
{
	QFileInfo check_file(config_path + "/NXDN.csv");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/NXDN.csv");
		f.remove();
	}
	process_dmr_ids();
}

void DudeStar::process_settings()
{
	QFileInfo check_file(config_path + "/settings.conf");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/settings.conf");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString s = f.readLine();
				QStringList sl = s.split(':');
				if(sl.at(0) == "PLAYBACK"){
					ui->AudioOutCombo->setCurrentText(sl.at(1).simplified());
				}
				if(sl.at(0) == "CAPTURE"){
					ui->AudioInCombo->setCurrentText(sl.at(1).simplified());
				}
				if(sl.at(0) == "MODE"){
					ui->modeCombo->blockSignals(true);
					int i = ui->modeCombo->findText(sl.at(1).simplified());
					ui->modeCombo->setCurrentIndex(i);
					process_mode_change(sl.at(1).simplified());

					if(i == 0){
						process_ref_hosts();
					}
					else if(i == 1){
						process_dcs_hosts();
					}
					else if(i == 2){
						process_xrf_hosts();
					}
					else if(i == 3){
						process_ysf_hosts();
					}
					else if(i == 4){
						process_dmr_hosts();
					}
					else if(i == 5){
						process_p25_hosts();
					}
					else if(i == 6){
						process_nxdn_hosts();
					}
				}
				ui->modeCombo->blockSignals(false);
				ui->hostCombo->blockSignals(true);
				if(sl.at(0) == "REFHOST"){
					saved_refhost = sl.at(1).simplified();
					if(ui->modeCombo->currentText().simplified() == "REF"){
						int i = ui->hostCombo->findText(saved_refhost);
						ui->hostCombo->setCurrentIndex(i);
					}
				}
				if(sl.at(0) == "DCSHOST"){
					saved_dcshost = sl.at(1).simplified();
					if(ui->modeCombo->currentText().simplified() == "DCS"){
						int i = ui->hostCombo->findText(saved_dcshost);
						ui->hostCombo->setCurrentIndex(i);
					}
				}
				if(sl.at(0) == "XRFHOST"){
					saved_xrfhost = sl.at(1).simplified();
					if(ui->modeCombo->currentText().simplified() == "XRF"){
						int i = ui->hostCombo->findText(saved_xrfhost);
						ui->hostCombo->setCurrentIndex(i);
					}
				}
				if(sl.at(0) == "YSFHOST"){
					saved_ysfhost = sl.at(1).simplified();
					if(ui->modeCombo->currentText().simplified() == "YSF"){
						int i = ui->hostCombo->findText(saved_ysfhost);
						ui->hostCombo->setCurrentIndex(i);
					}
				}
				if(sl.at(0) == "DMRHOST"){
					saved_dmrhost = sl.at(1).simplified();
					if(ui->modeCombo->currentText().simplified() == "DMR"){
						int i = ui->hostCombo->findText(saved_dmrhost);
						ui->hostCombo->setCurrentIndex(i);
					}
				}
				if(sl.at(0) == "P25HOST"){
					saved_p25host = sl.at(1).simplified();
					if(ui->modeCombo->currentText().simplified() == "P25"){
						int i = ui->hostCombo->findText(saved_p25host);
						ui->hostCombo->setCurrentIndex(i);
					}
				}
				if(sl.at(0) == "NXDNHOST"){
					saved_nxdnhost = sl.at(1).simplified();
					if(ui->modeCombo->currentText().simplified() == "NXDN"){
						int i = ui->hostCombo->findText(saved_nxdnhost);
						ui->hostCombo->setCurrentIndex(i);
					}
				}
				if(sl.at(0) == "M17HOST"){
					saved_m17host = sl.at(1).simplified();
					if(ui->modeCombo->currentText().simplified() == "M17"){
						int i = ui->hostCombo->findText(saved_m17host);
						ui->hostCombo->setCurrentIndex(i);
					}
				}
				if(sl.at(0) == "MODULE"){
					ui->comboMod->setCurrentText(sl.at(1).simplified());
				}
				if(sl.at(0) == "CALLSIGN"){
					ui->callsignEdit->setText(sl.at(1).simplified());
				}
				if(sl.at(0) == "DMRID"){
					ui->dmridEdit->setText(sl.at(1).simplified());
				}
				if(sl.at(0) == "DMRPASSWORD"){
					ui->dmrpwEdit->setText(sl.at(1).simplified());
				}
				if(sl.at(0) == "DMRTGID"){
					ui->dmrtgEdit->setText(sl.at(1).simplified());
				}
				if(sl.at(0) == "MYCALL"){
					ui->mycallEdit->setText(sl.at(1).simplified());
				}
				if(sl.at(0) == "URCALL"){
					ui->urcallEdit->setText(sl.at(1).simplified());
				}
				if(sl.at(0) == "RPTR1"){
					ui->rptr1Edit->setText(sl.at(1).simplified());
				}
				if(sl.at(0) == "RPTR2"){
					ui->rptr2Edit->setText(sl.at(1).simplified());
				}
				if(sl.at(0) == "USRTXT"){
					ui->usertxtEdit->setText(sl.at(1).simplified());
				}
				ui->hostCombo->blockSignals(false);
			}
		}
	}
	else{ //No settings.conf file, first time launch
		//process_ref_hosts();
	}
}

void DudeStar::discover_vocoders()
{
	QMap<QString, QString> l = SerialAMBE::discover_devices();
	QMap<QString, QString>::const_iterator i = l.constBegin();
	ui->AmbeCombo->addItem("Software vocoder", "");
	while (i != l.constEnd()) {
		ui->AmbeCombo->addItem(i.value(), i.key());
		++i;
	}
}

void DudeStar::process_connect()
{
	//fprintf(stderr, "process_connect() called connect_status == %d\n", connect_status);fflush(stderr);
    if(connect_status != DISCONNECTED){
		connect_status = DISCONNECTED;
		m_modethread->quit();
		//delete m_modethread;
        ui->connectButton->setText("Connect");
        ui->mycall->clear();
        ui->urcall->clear();
        ui->rptr1->clear();
        ui->rptr2->clear();
		ui->streamid->clear();
		ui->usertxt->clear();
		ui->AmbeCombo->setEnabled(true);
		ui->checkBoxSWRX->setEnabled(true);
		ui->checkBoxSWTX->setEnabled(true);
		ui->AudioOutCombo->setEnabled(true);
		ui->AudioInCombo->setEnabled(true);
		ui->modeCombo->setEnabled(true);
        ui->hostCombo->setEnabled(true);
        ui->callsignEdit->setEnabled(true);
		ui->dmridEdit->setEnabled(true);
		ui->txButton->setDisabled(true);
		status_txt->setText("Not connected");

		if((protocol == "DCS") || (protocol == "XRF") || (protocol == "M17")){
			ui->comboMod->setEnabled(true);
		}
    }
    else{
		QStringList sl = ui->hostCombo->currentData().toString().simplified().split(':');
		connect_status = CONNECTING;
		status_txt->setText("Connecting...");
		//ui->connectButton->setEnabled(false);
		ui->connectButton->setText("Connecting");
		host = sl.at(0).simplified();
		port = sl.at(1).toInt();
		callsign = ui->callsignEdit->text().toUpper();
		ui->callsignEdit->setText(callsign);
		module = ui->comboMod->currentText().toStdString()[0];
		protocol = ui->modeCombo->currentText();
		hostname = ui->hostCombo->currentText().simplified();

		if(protocol == "REF"){
			m_ref = new REFCodec(callsign, hostname, host, port, ui->AmbeCombo->currentData().toString().simplified(), ui->AudioInCombo->currentText(), ui->AudioOutCombo->currentText());
			m_modethread = new QThread;
			m_ref->moveToThread(m_modethread);
			connect(m_ref, SIGNAL(update()), this, SLOT(update_ref_data()));
			connect(m_modethread, SIGNAL(started()), m_ref, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_ref, SLOT(deleteLater()));
			//connect(m_modethread, SIGNAL(finished()), m_modethread, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_ref, SLOT(input_src_changed(int, QString)));
			connect(ui->comboMod, SIGNAL(currentIndexChanged(int)), m_ref, SLOT(module_changed(int)));
			connect(ui->mycallEdit, SIGNAL(textChanged(QString)), m_ref, SLOT(mycall_changed(QString)));
			connect(ui->urcallEdit, SIGNAL(textChanged(QString)), m_ref, SLOT(urcall_changed(QString)));
			connect(ui->rptr1Edit, SIGNAL(textChanged(QString)), m_ref, SLOT(rptr1_changed(QString)));
			connect(ui->rptr2Edit, SIGNAL(textChanged(QString)), m_ref, SLOT(rptr2_changed(QString)));
			connect(ui->checkBoxSWRX, SIGNAL(stateChanged(int)), m_ref, SLOT(swrx_state_changed(int)));
			connect(ui->checkBoxSWTX, SIGNAL(stateChanged(int)), m_ref, SLOT(swtx_state_changed(int)));
			connect(ui->txButton, SIGNAL(pressed()), m_ref, SLOT(start_tx()));
			connect(ui->txButton, SIGNAL(released()), m_ref, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_ref, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_ref, SLOT(in_audio_vol_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->TTSEdit->text());
			emit ui->comboMod->currentIndexChanged(ui->comboMod->currentIndex());
			emit ui->mycallEdit->textChanged(ui->mycallEdit->text());
			emit ui->urcallEdit->textChanged(ui->urcallEdit->text());
			emit ui->rptr1Edit->textChanged(ui->rptr1Edit->text());
			emit ui->rptr2Edit->textChanged(ui->rptr2Edit->text());
			m_modethread->start();
		}
		if(protocol == "DCS"){
			m_dcs = new DCSCodec(callsign, hostname, host, port, ui->AmbeCombo->currentData().toString().simplified(), ui->AudioInCombo->currentText(), ui->AudioOutCombo->currentText());
			m_modethread = new QThread;
			m_dcs->moveToThread(m_modethread);
			connect(m_dcs, SIGNAL(update()), this, SLOT(update_dcs_data()));
			connect(m_modethread, SIGNAL(started()), m_dcs, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_dcs, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_dcs, SLOT(input_src_changed(int, QString)));
			connect(ui->comboMod, SIGNAL(currentIndexChanged(int)), m_dcs, SLOT(module_changed(int)));
			connect(ui->mycallEdit, SIGNAL(textChanged(QString)), m_dcs, SLOT(mycall_changed(QString)));
			connect(ui->urcallEdit, SIGNAL(textChanged(QString)), m_dcs, SLOT(urcall_changed(QString)));
			connect(ui->rptr1Edit, SIGNAL(textChanged(QString)), m_dcs, SLOT(rptr1_changed(QString)));
			connect(ui->rptr2Edit, SIGNAL(textChanged(QString)), m_dcs, SLOT(rptr2_changed(QString)));
			connect(ui->checkBoxSWRX, SIGNAL(stateChanged(int)), m_dcs, SLOT(swrx_state_changed(int)));
			connect(ui->checkBoxSWTX, SIGNAL(stateChanged(int)), m_dcs, SLOT(swtx_state_changed(int)));
			connect(ui->txButton, SIGNAL(pressed()), m_dcs, SLOT(start_tx()));
			connect(ui->txButton, SIGNAL(released()), m_dcs, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_dcs, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_dcs, SLOT(in_audio_vol_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->TTSEdit->text());
			emit ui->comboMod->currentIndexChanged(ui->comboMod->currentIndex());
			emit ui->mycallEdit->textChanged(ui->mycallEdit->text());
			emit ui->urcallEdit->textChanged(ui->urcallEdit->text());
			emit ui->rptr1Edit->textChanged(ui->rptr1Edit->text());
			emit ui->rptr2Edit->textChanged(ui->rptr2Edit->text());
			m_modethread->start();
		}
		if(protocol == "XRF"){
			m_xrf = new XRFCodec(callsign, hostname, host, port, ui->AmbeCombo->currentData().toString().simplified(), ui->AudioInCombo->currentText(), ui->AudioOutCombo->currentText());
			m_modethread = new QThread;
			m_xrf->moveToThread(m_modethread);
			connect(m_xrf, SIGNAL(update()), this, SLOT(update_xrf_data()));
			connect(m_modethread, SIGNAL(started()), m_xrf, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_xrf, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_xrf, SLOT(input_src_changed(int, QString)));
			connect(ui->comboMod, SIGNAL(currentIndexChanged(int)), m_xrf, SLOT(module_changed(int)));
			connect(ui->mycallEdit, SIGNAL(textChanged(QString)), m_xrf, SLOT(mycall_changed(QString)));
			connect(ui->urcallEdit, SIGNAL(textChanged(QString)), m_xrf, SLOT(urcall_changed(QString)));
			connect(ui->rptr1Edit, SIGNAL(textChanged(QString)), m_xrf, SLOT(rptr1_changed(QString)));
			connect(ui->rptr2Edit, SIGNAL(textChanged(QString)), m_xrf, SLOT(rptr2_changed(QString)));
			connect(ui->checkBoxSWRX, SIGNAL(stateChanged(int)), m_xrf, SLOT(swrx_state_changed(int)));
			connect(ui->checkBoxSWTX, SIGNAL(stateChanged(int)), m_xrf, SLOT(swtx_state_changed(int)));
			connect(ui->txButton, SIGNAL(pressed()), m_xrf, SLOT(start_tx()));
			connect(ui->txButton, SIGNAL(released()), m_xrf, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_xrf, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_xrf, SLOT(in_audio_vol_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->TTSEdit->text());
			emit ui->comboMod->currentIndexChanged(ui->comboMod->currentIndex());
			emit ui->mycallEdit->textChanged(ui->mycallEdit->text());
			emit ui->urcallEdit->textChanged(ui->urcallEdit->text());
			emit ui->rptr1Edit->textChanged(ui->rptr1Edit->text());
			emit ui->rptr2Edit->textChanged(ui->rptr2Edit->text());
			m_modethread->start();
		}
		if(protocol == "YSF"){
			m_ysf = new YSFCodec(callsign, hostname, host, port, ui->AmbeCombo->currentData().toString().simplified(), ui->AudioInCombo->currentText(), ui->AudioOutCombo->currentText());
			m_modethread = new QThread;
			m_ysf->moveToThread(m_modethread);
			connect(m_ysf, SIGNAL(update()), this, SLOT(update_ysf_data()));
			connect(m_modethread, SIGNAL(started()), m_ysf, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_ysf, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_ysf, SLOT(input_src_changed(int, QString)));
			connect(ui->checkBoxSWRX, SIGNAL(stateChanged(int)), m_ysf, SLOT(swrx_state_changed(int)));
			connect(ui->checkBoxSWTX, SIGNAL(stateChanged(int)), m_ysf, SLOT(swtx_state_changed(int)));
			connect(ui->txButton, SIGNAL(pressed()), m_ysf, SLOT(start_tx()));
			connect(ui->txButton, SIGNAL(released()), m_ysf, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_ysf, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_ysf, SLOT(in_audio_vol_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->TTSEdit->text());
			m_modethread->start();
		}
		if(protocol == "DMR"){
			//dmrid = dmrids.key(callsign);
			//dmr_password = sl.at(2).simplified();
			dmrid = ui->dmridEdit->text().toUInt();
			dmr_password = (ui->dmrpwEdit->text().isEmpty()) ? sl.at(2).simplified() : ui->dmrpwEdit->text();
			dmr_destid = ui->dmrtgEdit->text().toUInt();
            dmr_options = ui->dmrOptions->text();
            dmr_repeater = ui->dmrRepeaterId->text();
            QString aux_repeater = "";
            aux_repeater.append(ui->dmridEdit->text());
            aux_repeater.append(dmr_repeater);
            m_dmr = new DMRCodec(callsign, dmrid, dmr_password, dmr_options, aux_repeater.toUInt(), dmr_destid, host, port, ui->AmbeCombo->currentData().toString().simplified(), ui->AudioInCombo->currentText(), ui->AudioOutCombo->currentText());
			m_modethread = new QThread;
			m_dmr->moveToThread(m_modethread);
			connect(m_dmr, SIGNAL(update()), this, SLOT(update_dmr_data()));
			connect(m_modethread, SIGNAL(started()), m_dmr, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_dmr, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_dmr, SLOT(input_src_changed(int, QString)));
			connect(this, SIGNAL(dmr_tgid_changed(unsigned int)), m_dmr, SLOT(dmr_tgid_changed(unsigned int)));
			connect(ui->checkBoxDMRPC, SIGNAL(stateChanged(int)), m_dmr, SLOT(dmrpc_state_changed(int)));
			connect(ui->checkBoxSWRX, SIGNAL(stateChanged(int)), m_dmr, SLOT(swrx_state_changed(int)));
			connect(ui->checkBoxSWTX, SIGNAL(stateChanged(int)), m_dmr, SLOT(swtx_state_changed(int)));
			connect(ui->txButton, SIGNAL(pressed()), m_dmr, SLOT(start_tx()));
			connect(ui->txButton, SIGNAL(released()), m_dmr, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_dmr, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_dmr, SLOT(in_audio_vol_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->TTSEdit->text());
			m_modethread->start();
		}
		if(protocol == "P25"){
			dmrid = ui->dmridEdit->text().toUInt();
			dmr_destid = ui->dmrtgEdit->text().toUInt();
			m_p25 = new P25Codec(callsign, dmrid, dmr_destid, host, port, ui->AudioInCombo->currentText(), ui->AudioOutCombo->currentText());
			m_modethread = new QThread;
			m_p25->moveToThread(m_modethread);
			connect(m_p25, SIGNAL(update()), this, SLOT(update_p25_data()));
			connect(m_modethread, SIGNAL(started()), m_p25, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_p25, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_p25, SLOT(input_src_changed(int, QString)));
			connect(this, SIGNAL(dmr_tgid_changed(unsigned int)), m_p25, SLOT(dmr_tgid_changed(unsigned int)));
			connect(ui->txButton, SIGNAL(pressed()), m_p25, SLOT(start_tx()));
			connect(ui->txButton, SIGNAL(released()), m_p25, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_p25, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_p25, SLOT(in_audio_vol_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->TTSEdit->text());
			m_modethread->start();
		}
		if(protocol == "NXDN"){
			dmrid = nxdnids.key(callsign);
			dmr_destid = ui->hostCombo->currentText().toUInt();
			m_nxdn = new NXDNCodec(callsign, dmr_destid, host, port, ui->AmbeCombo->currentData().toString().simplified(), ui->AudioInCombo->currentText(), ui->AudioOutCombo->currentText());
			m_modethread = new QThread;
			m_nxdn->moveToThread(m_modethread);
			connect(m_nxdn, SIGNAL(update()), this, SLOT(update_nxdn_data()));
			connect(m_modethread, SIGNAL(started()), m_nxdn, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_nxdn, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_nxdn, SLOT(input_src_changed(int, QString)));
			connect(ui->checkBoxSWRX, SIGNAL(stateChanged(int)), m_nxdn, SLOT(swrx_state_changed(int)));
			connect(ui->checkBoxSWTX, SIGNAL(stateChanged(int)), m_nxdn, SLOT(swtx_state_changed(int)));
			connect(ui->txButton, SIGNAL(pressed()), m_nxdn, SLOT(start_tx()));
			connect(ui->txButton, SIGNAL(released()), m_nxdn, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_nxdn, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_nxdn, SLOT(in_audio_vol_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->TTSEdit->text());
			m_modethread->start();
		}
		if(protocol == "M17"){
			m_m17 = new M17Codec(callsign, module, hostname, host, port, ui->AudioInCombo->currentText(), ui->AudioOutCombo->currentText());
			m_modethread = new QThread;
			m_m17->moveToThread(m_modethread);
			connect(m_m17, SIGNAL(update()), this, SLOT(update_m17_data()));
			connect(this, SIGNAL(rate_changed(int)), m_m17, SLOT(rate_changed(int)));
			connect(m_modethread, SIGNAL(started()), m_m17, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_m17, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_m17, SLOT(input_src_changed(int, QString)));
			connect(ui->txButton, SIGNAL(pressed()), m_m17, SLOT(start_tx()));
			connect(ui->txButton, SIGNAL(released()), m_m17, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_m17, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_m17, SLOT(in_audio_vol_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->TTSEdit->text());
			m_modethread->start();
		}
    }
}

void DudeStar::process_volume_changed(int v)
{
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(!muted){
		emit out_audio_vol_changed(linear_vol);
	}
	//qDebug("volume == %d : %4.2f", v, linear_vol);
}

void DudeStar::process_mute_button()
{
	int v = ui->volumeSlider->value();
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(muted){
		muted = false;
		ui->muteButton->setText("Mute");
		emit out_audio_vol_changed(linear_vol);
		//audio->setVolume(linear_vol);
	}
	else{
		muted = true;
		ui->muteButton->setText("Unmute");
		emit out_audio_vol_changed(0.0);
		//audio->setVolume(0.0);
	}
}

void DudeStar::process_input_volume_changed(int v)
{
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(!input_muted){
		emit in_audio_vol_changed(linear_vol);
	}
	//qDebug("volume == %d : %4.2f", v, linear_vol);
}

void DudeStar::process_input_mute_button()
{
	int v = ui->volumeSlider->value();
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(input_muted){
		input_muted = false;
		ui->inmuteButton->setText("Mute");
		emit in_audio_vol_changed(linear_vol);
		//audioin->setVolume(linear_vol);
	}
	else{
		input_muted = true;
		ui->inmuteButton->setText("Unmute");
		emit in_audio_vol_changed(linear_vol);
		//audioin->setVolume(0.0);
	}
}

void DudeStar::update_m17_data()
{
	if(connect_status == DISCONNECTED){
		qDebug() << "update_m17_data() called after disconnected";
		return;
	}
	if((connect_status == CONNECTING) && (m_m17->get_status() == DISCONNECTED)){
		process_connect();
		QMessageBox::warning(this, tr("Connection refused"), tr("M17 connection refused.  Check callsign and confirm this callsign or IP is not already connected to this reflector"));
		return;
	}
	if( (connect_status == CONNECTING) && ( m_m17->get_status() == CONNECTED_RW)){
		connect_status = CONNECTED_RW;
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->AmbeCombo->setEnabled(false);
		ui->AudioOutCombo->setEnabled(false);
		ui->AudioInCombo->setEnabled(false);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->comboMod->setEnabled(false);
		ui->txButton->setDisabled(false);
		ui->checkBoxSWRX->setChecked(true);
		ui->checkBoxSWTX->setChecked(true);
		ui->checkBoxSWRX->setEnabled(false);
		ui->checkBoxSWTX->setEnabled(false);
	}
	status_txt->setText(" Host: " + m_m17->get_host() + ":" + QString::number( m_m17->get_port()) + " Ping: " + QString::number(m_m17->get_cnt()));
	ui->mycall->setText(m_m17->get_src());
	ui->urcall->setText(m_m17->get_dst());
	ui->rptr1->setText(m_m17->get_type());
	if(m_m17->get_fn()){
		QString n = QString("%1").arg(m_m17->get_fn(), 4, 16, QChar('0'));
		ui->rptr2->setText(n);
	}
	if(m_m17->get_streamid()){
		ui->streamid->setText(QString::number(m_m17->get_streamid(), 16));
	}
}

void DudeStar::update_ysf_data()
{
	if(connect_status == DISCONNECTED){
		qDebug() << "update_ysf_data() called after disconnected";
		return;
	}
	if( (connect_status == CONNECTING) && ( m_ysf->get_status() == CONNECTED_RW)){
		connect_status = CONNECTED_RW;
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->AmbeCombo->setEnabled(false);
		ui->AudioOutCombo->setEnabled(false);
		ui->AudioInCombo->setEnabled(false);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->comboMod->setEnabled(false);
		ui->txButton->setDisabled(false);

		ui->checkBoxSWRX->setChecked(!(m_ysf->get_hwrx()));
		if(!(m_ysf->get_hwrx())){
			ui->checkBoxSWRX->setEnabled(false);
		}
		ui->checkBoxSWTX->setChecked(!(m_ysf->get_hwtx()));
		if(!(m_ysf->get_hwtx())){
			ui->checkBoxSWTX->setEnabled(false);
		}
	}

	status_txt->setText(" Host: " + m_ysf->get_host() + ":" + QString::number( m_ysf->get_port()) + " Ping: " + QString::number(m_ysf->get_cnt()));
	ui->mycall->setText(m_ysf->get_gateway());
	ui->urcall->setText(m_ysf->get_src());
	ui->rptr1->setText(m_ysf->get_dst());
	if(m_ysf->get_type() == 0){
		ui->rptr2->setText("V/D mode 1");
	}
	else if(m_ysf->get_type() == 1){
		ui->rptr2->setText("Data Full Rate");
	}
	else if(m_ysf->get_type() == 2){
		ui->rptr2->setText("V/D mode 2");
	}
	else if(m_ysf->get_type() == 3){
		ui->rptr2->setText("Voice Full Rate");
	}
	else{
		ui->rptr2->setText("");
	}
	if(m_ysf->get_type() >= 0){
		ui->streamid->setText(m_ysf->get_path()  ? "Internet" : "Local");
		ui->usertxt->setText(QString::number(m_ysf->get_fn()) + "/" + QString::number(m_ysf->get_ft()));
	}
}

void DudeStar::update_nxdn_data()
{
	if(connect_status == DISCONNECTED){
		qDebug() << "update_nxdn_data() called after disconnected";
		return;
	}
	if( (connect_status == CONNECTING) && ( m_nxdn->get_status() == CONNECTED_RW)){
		connect_status = CONNECTED_RW;
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->AmbeCombo->setEnabled(false);
		ui->AudioOutCombo->setEnabled(false);
		ui->AudioInCombo->setEnabled(false);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->comboMod->setEnabled(false);
		ui->txButton->setDisabled(false);

		ui->checkBoxSWRX->setChecked(!(m_nxdn->get_hwrx()));
		if(!(m_nxdn->get_hwrx())){
			ui->checkBoxSWRX->setEnabled(false);
		}
		ui->checkBoxSWTX->setChecked(!(m_nxdn->get_hwtx()));
		if(!(m_nxdn->get_hwtx())){
			ui->checkBoxSWTX->setEnabled(false);
		}
	}
	status_txt->setText(" Host: " + m_nxdn->get_host() + ":" + QString::number( m_nxdn->get_port()) + " Ping: " + QString::number(m_nxdn->get_cnt()));
	if(m_nxdn->get_src()){
		ui->mycall->setText(m_dmrids[m_nxdn->get_src()]);
		ui->urcall->setText(QString::number(m_nxdn->get_src()));
	}
	ui->rptr1->setText(m_nxdn->get_dst() ? QString::number(m_nxdn->get_dst()) : "");
	if(m_nxdn->get_fn()){
		QString n = QString("%1").arg(m_nxdn->get_fn(), 2, 16, QChar('0'));
		ui->streamid->setText(n);
	}
}

void DudeStar::update_p25_data()
{
	if(connect_status == DISCONNECTED){
		qDebug() << "update_p25_data() called after disconnected";
		return;
	}
	if( (connect_status == CONNECTING) && ( m_p25->get_status() == CONNECTED_RW)){
		connect_status = CONNECTED_RW;
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->AmbeCombo->setEnabled(false);
		ui->AudioOutCombo->setEnabled(false);
		ui->AudioInCombo->setEnabled(false);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->dmridEdit->setEnabled(false);
		ui->comboMod->setEnabled(false);
		ui->txButton->setDisabled(false);
		ui->checkBoxSWRX->setChecked(true);
		ui->checkBoxSWTX->setChecked(true);
		ui->checkBoxSWRX->setEnabled(false);
		ui->checkBoxSWTX->setEnabled(false);
	}
	status_txt->setText(" Host: " + m_p25->get_host() + ":" + QString::number( m_p25->get_port()) + " Ping: " + QString::number(m_p25->get_cnt()));
	if(m_p25->get_src()){
		ui->mycall->setText(m_dmrids[m_p25->get_src()]);
		ui->urcall->setText(QString::number(m_p25->get_src()));
		ui->rptr2->setText(QString::number(m_p25->get_src()));
	}
	ui->rptr1->setText(m_p25->get_dst() ? QString::number(m_p25->get_dst()) : "");
	if(m_p25->get_fn()){
		QString n = QString("%1").arg(m_p25->get_fn(), 2, 16, QChar('0'));
		ui->streamid->setText(n);
	}
}

void DudeStar::update_dmr_data()
{
	if(connect_status == DISCONNECTED){
		qDebug() << "update_dmr_data() called after disconnected";
		return;
	}
	if((connect_status == CONNECTING) && (m_dmr->get_status() == DISCONNECTED)){
		process_connect();
		QMessageBox::warning(this, tr("Connection refused"), tr("DMR connection refused.  Check callsign, DMR ID, or password"));
		return;
	}
	if((connect_status == CONNECTING) && (m_dmr->get_status() == CONNECTED_RW)){
		connect_status = CONNECTED_RW;
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->AmbeCombo->setEnabled(false);
		ui->AudioOutCombo->setEnabled(false);
		ui->AudioInCombo->setEnabled(false);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->dmridEdit->setEnabled(false);
		ui->dmrpwEdit->setEnabled(false);
		ui->txButton->setDisabled(false);
		//ui->dmrtgEdit->setEnabled(false);
		ui->checkBoxSWRX->setChecked(!(m_dmr->get_hwrx()));
		if(!(m_dmr->get_hwrx())){
			ui->checkBoxSWRX->setEnabled(false);
		}
		ui->checkBoxSWTX->setChecked(!(m_dmr->get_hwtx()));
		if(!(m_dmr->get_hwtx())){
			ui->checkBoxSWTX->setEnabled(false);
		}
	}
	status_txt->setText(" Host: " + m_dmr->get_host() + ":" + QString::number( m_dmr->get_port()) + " Ping: " + QString::number(m_dmr->get_cnt()));
	if(m_dmr->get_src()){
		ui->mycall->setText(m_dmrids[m_dmr->get_src()]);
		ui->urcall->setText(QString::number(m_dmr->get_src()));
	}
	ui->rptr1->setText(m_dmr->get_dst() ? QString::number(m_dmr->get_dst()) : "");
	ui->rptr2->setText(m_dmr->get_gw() ? QString::number(m_dmr->get_gw()) : "");
	if(m_dmr->get_fn()){
		QString n = QString("%1").arg(m_dmr->get_fn(), 2, 16, QChar('0'));
		ui->streamid->setText(n);
	}
}

void DudeStar::update_ref_data()
{
	if(connect_status == DISCONNECTED){
		qDebug() << "update_ref_data() called after disconnected";
		return;
	}
	if((connect_status == CONNECTING) && (m_ref->get_status() == CONNECTED_RW)){
		connect_status = CONNECTED_RW;
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->AmbeCombo->setEnabled(false);
		ui->AudioOutCombo->setEnabled(false);
		ui->AudioInCombo->setEnabled(false);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->dmridEdit->setEnabled(false);
		ui->dmrpwEdit->setEnabled(false);
		ui->txButton->setDisabled(false);
		//ui->dmrtgEdit->setEnabled(false);
		ui->checkBoxSWRX->setChecked(!(m_ref->get_hwrx()));
		if(!(m_ref->get_hwrx())){
			ui->checkBoxSWRX->setEnabled(false);
		}
		ui->checkBoxSWTX->setChecked(!(m_ref->get_hwtx()));
		if(!(m_ref->get_hwtx())){
			ui->checkBoxSWTX->setEnabled(false);
		}
	}
	ui->mycall->setText(m_ref->get_mycall());
	ui->urcall->setText(m_ref->get_urcall());
	ui->rptr1->setText(m_ref->get_rptr1());
	ui->rptr2->setText(m_ref->get_rptr2());
	ui->streamid->setText(QString::number(m_ref->get_streamid(), 16) + " " + QString::number(m_ref->get_fn(), 16));
	ui->usertxt->setText(m_ref->get_usertxt());
	status_txt->setText(" Host: " + m_ref->get_host() + ":" + QString::number( m_ref->get_port()) + " Ping: " + QString::number(m_ref->get_cnt()));
}

void DudeStar::update_dcs_data()
{
	if(connect_status == DISCONNECTED){
		qDebug() << "update_dcs_data() called after disconnected";
		return;
	}
	if((connect_status == CONNECTING) && (m_dcs->get_status() == CONNECTED_RW)){
		connect_status = CONNECTED_RW;
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->AmbeCombo->setEnabled(false);
		ui->AudioOutCombo->setEnabled(false);
		ui->AudioInCombo->setEnabled(false);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->dmridEdit->setEnabled(false);
		ui->dmrpwEdit->setEnabled(false);
		ui->txButton->setDisabled(false);

		ui->checkBoxSWRX->setChecked(!(m_dcs->get_hwrx()));
		if(!(m_dcs->get_hwrx())){
			ui->checkBoxSWRX->setEnabled(false);
		}
		ui->checkBoxSWTX->setChecked(!(m_dcs->get_hwtx()));
		if(!(m_dcs->get_hwtx())){
			ui->checkBoxSWTX->setEnabled(false);
		}
	}
	ui->mycall->setText(m_dcs->get_mycall());
	ui->urcall->setText(m_dcs->get_urcall());
	ui->rptr1->setText(m_dcs->get_rptr1());
	ui->rptr2->setText(m_dcs->get_rptr2());
	ui->streamid->setText(QString::number(m_dcs->get_streamid(), 16) + " " + QString::number(m_dcs->get_fn(), 16));
	ui->usertxt->setText(m_dcs->get_usertxt());
	status_txt->setText(" Host: " + m_dcs->get_host() + ":" + QString::number( m_dcs->get_port()) + " Ping: " + QString::number(m_dcs->get_cnt()));
}

void DudeStar::update_xrf_data()
{
	if(connect_status == DISCONNECTED){
		qDebug() << "update_xrf_data() called after disconnected";
		return;
	}
	if((connect_status == CONNECTING) && (m_xrf->get_status() == CONNECTED_RW)){
		connect_status = CONNECTED_RW;
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->AmbeCombo->setEnabled(false);
		ui->AudioOutCombo->setEnabled(false);
		ui->AudioInCombo->setEnabled(false);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->dmridEdit->setEnabled(false);
		ui->dmrpwEdit->setEnabled(false);
		ui->txButton->setDisabled(false);
		ui->checkBoxSWRX->setChecked(!(m_xrf->get_hwrx()));
		if(!(m_xrf->get_hwrx())){
			ui->checkBoxSWRX->setEnabled(false);
		}
		ui->checkBoxSWTX->setChecked(!(m_xrf->get_hwtx()));
		if(!(m_xrf->get_hwtx())){
			ui->checkBoxSWTX->setEnabled(false);
		}
	}
	ui->mycall->setText(m_xrf->get_mycall());
	ui->urcall->setText(m_xrf->get_urcall());
	ui->rptr1->setText(m_xrf->get_rptr1());
	ui->rptr2->setText(m_xrf->get_rptr2());
	ui->streamid->setText(QString::number(m_xrf->get_streamid(), 16) + " " + QString::number(m_xrf->get_fn(), 16));
	ui->usertxt->setText(m_xrf->get_usertxt());
	status_txt->setText(" Host: " + m_xrf->get_host() + ":" + QString::number( m_xrf->get_port()) + " Ping: " + QString::number(m_xrf->get_cnt()));
}

void DudeStar::handleStateChanged(QAudio::State)
{
}
