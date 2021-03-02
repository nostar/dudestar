/*
	Copyright (C) 2019-2021 Doug McLain

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

#ifdef Q_OS_MACOS
#include "micpermission.h"
#endif

#ifdef Q_OS_WIN64
#define ARCHNAME "Windows 64-bit"
#elif defined Q_OS_WIN32
#define ARCHNAME "Windows 32-bit"
#elif defined Q_OS_MACOS
#define ARCHNAME "macOS"
#elif defined Q_OS_LINUX
#define ARCHNAME "Linux"
#elif defined Q_OS_UNIX
#define ARCHNAME "Unix"
#else
#define ARCHNAME "Unknown"
#endif

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
	m_update_host_files(false),
	m_dmrcc(1),
	m_dmrslot(2),
	m_dmrcalltype(0),
	m_outlevel(0),
	m_rxcnt(0)
{
	qRegisterMetaType<Codec::MODEINFO>("Codec::MODEINFO");
	m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "dudestar", this);
	//m_reflocalhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "REFLocal", this);
	//m_dcslocalhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "DCSLocal", this);
	//m_xrflocalhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "XRFLocal", this);
	//m_dmrlocalhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "DMRLocal", this);
	//m_ysflocalhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "YSFLocal", this);
	//m_p25localhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "P25Local", this);
	//m_nxdnlocalhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "NXDNLocal", this);
	//m_m17localhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "M17Local", this);
	//m_iaxlocalhosts = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "IAXLocal", this);
	muted = false;
	config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#ifndef Q_OS_WIN
	config_path += "/dudetronics";
#endif
    ui->setupUi(this);
    init_gui();
	connect_status = Codec::DISCONNECTED;
	user_data.resize(21);
	check_host_files();
	process_settings();
	srand(time(0));
}

DudeStar::~DudeStar()
{
	save_settings();
	delete ui;
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
	m_levelmeter = new LevelMeter(this);
	m_labeldb = new QLabel();
	m_labeldb->setMaximumWidth(40);
	m_labeldb->setMaximumHeight(20);
	m_labeldb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_labeldb->setStyleSheet("QLabel { background-color : #353535; padding: 2;}");
	ui->labelCodecGain->setStyleSheet("QLabel { background-color : #353535; padding: 2;}");
	ui->levelmeterLayout->addWidget(m_levelmeter);
	ui->levelmeterLayout->addWidget(m_labeldb);
	m_levelmeter->setLevel(0.0);
	status_txt = new QLabel("Not connected");
	tts_voices = new QButtonGroup();
	tts_voices->addButton(ui->radioMic, 0);
	tts_voices->addButton(ui->radioTTS1, 1);
	tts_voices->addButton(ui->radioTTS2, 2);
	tts_voices->addButton(ui->radioTTS3, 3);
#ifdef USE_FLITE
	connect(tts_voices, SIGNAL(buttonClicked(int)), this, SLOT(tts_changed(int)));
	connect(ui->editTTSTXT, SIGNAL(textChanged(QString)), this, SLOT(tts_text_changed(QString)));
#endif
#ifndef USE_FLITE
	ui->radioMic->hide();
	ui->radioTTS1->hide();
	ui->radioTTS2->hide();
	ui->radioTTS3->hide();
	ui->editTTSTXT->hide();
#endif
	ui->pushTX->setAutoFillBackground(true);
	ui->pushTX->setStyleSheet("QPushButton:enabled { background-color: rgb(128, 195, 66); color: rgb(0,0,0); } QPushButton:pressed { background-color: rgb(180, 0, 0); color: rgb(0,0,0); }");
	ui->pushTX->update();
	ui->radioMic->setChecked(true);
	ui->sliderCodecGain->setRange(100, 800);
	ui->sliderCodecGain->setValue(100);
	ui->sliderVolume->setRange(0, 100);
	ui->sliderVolume->setValue(100);
	ui->sliderMic->setRange(0, 100);
	ui->sliderMic->setValue(100);
	ui->pushTX->setDisabled(true);
	m17rates = new QButtonGroup();
	m17rates->addButton(ui->radioButtonM173200, 1);
	m17rates->addButton(ui->radioButtonM171600, 0);
	ui->radioButtonM173200->setChecked(true);
	connect(m17rates, SIGNAL(buttonClicked(int)), this, SLOT(m17_rate_changed(int)));
	connect(ui->editTG, SIGNAL(textChanged(QString)), this, SLOT(tgid_text_changed(QString)));
	connect(ui->pushConnect, SIGNAL(clicked()), this, SLOT(process_connect()));
	connect(ui->sliderCodecGain, SIGNAL(valueChanged(int)), this, SLOT(process_codecgain_changed(int)));
	connect(ui->pushVolMute, SIGNAL(clicked()), this, SLOT(process_mute_button()));
	connect(ui->sliderVolume, SIGNAL(valueChanged(int)), this, SLOT(process_volume_changed(int)));
	connect(ui->pushMicMute, SIGNAL(clicked()), this, SLOT(process_mic_mute_button()));
	connect(ui->sliderMic, SIGNAL(valueChanged(int)), this, SLOT(process_mic_gain_changed(int)));
	ui->statusBar->insertPermanentWidget(0, status_txt, 1);
	connect(ui->checkSWRX, SIGNAL(stateChanged(int)), this, SLOT(swrx_state_changed(int)));
	connect(ui->checkSWTX, SIGNAL(stateChanged(int)), this, SLOT(swtx_state_changed(int)));
	connect(ui->pushUpdateHostFiles, SIGNAL(clicked()), this, SLOT(update_host_files()));
	connect(ui->pushUpdateDMRIDs, SIGNAL(clicked()), this, SLOT(update_dmr_ids()));
	connect(ui->pushIAXDTMF, SIGNAL(clicked()), this, SLOT(process_dtmf()));
	ui->data1->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->data2->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->data3->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->data4->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->data5->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->data6->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->comboMode->addItem("REF");
	ui->comboMode->addItem("DCS");
	ui->comboMode->addItem("XRF");
	ui->comboMode->addItem("YSF");
	ui->comboMode->addItem("FCS");
	ui->comboMode->addItem("DMR");
	ui->comboMode->addItem("P25");
	ui->comboMode->addItem("NXDN");
	ui->comboMode->addItem("M17");
	ui->comboMode->addItem("IAX");

	ui->comboCustomHostMode->addItem("REF");
	ui->comboCustomHostMode->addItem("DCS");
	ui->comboCustomHostMode->addItem("XRF");
	ui->comboCustomHostMode->addItem("YSF");
	ui->comboCustomHostMode->addItem("FCS");
	ui->comboCustomHostMode->addItem("DMR");
	ui->comboCustomHostMode->addItem("P25");
	ui->comboCustomHostMode->addItem("NXDN");
	ui->comboCustomHostMode->addItem("M17");
	ui->comboCustomHostMode->addItem("IAX");

	for(uint8_t cc = 1; cc < 8; ++cc){
		ui->comboCC->addItem(QString::number(cc));
	}

	ui->comboESSID->addItem("None");
	for(uint8_t essid = 0; essid < 100; ++essid){
		ui->comboESSID->addItem(QString::number(essid));
	}

	ui->comboSlot->addItem(QString::number(1));
	ui->comboSlot->addItem(QString::number(2));
	ui->comboSlot->setCurrentIndex(1);

	connect(ui->comboMode, SIGNAL(currentTextChanged(const QString &)), this, SLOT(process_mode_change(const QString &)));
	connect(ui->comboHost, SIGNAL(currentTextChanged(const QString &)), this, SLOT(process_host_change(const QString &)));

	for(char m = 0x41; m < 0x5b; ++m){
		ui->comboModule->addItem(QString(m));
	}

	ui->comboHost->setEditable(true);
	ui->editTG->setEnabled(false);

	discover_vocoders();
	ui->comboPlayback->addItem("OS Default");
	ui->comboPlayback->addItems(AudioEngine::discover_audio_devices(AUDIO_OUT));
	ui->comboCapture->addItem("OS Default");
	ui->comboCapture->addItems(AudioEngine::discover_audio_devices(AUDIO_IN));
	ui->comboESSID->setStyleSheet("combobox-popup: 0;");
	ui->comboModule->setStyleSheet("combobox-popup: 0;");
	//ui->comboPlayback->setStyleSheet("combobox-popup: 0;");
	//ui->comboCapture->setStyleSheet("combobox-popup: 0;");
	ui->textAbout->setHtml(tr("<p>DUDE-Star %1 git build %2</p><p>Copyright (C) 2019-2021 Doug McLain AD8DP</p>"
							  "<p>This program is free software; you can redistribute it "
							  "and/or modify it under the terms of the GNU General Public "
							  "License as published by the Free Software Foundation; "
							  "version 2.\n\nThis program is distributed in the hope that it "
							  "will be useful, but WITHOUT ANY WARRANTY; without even "
							  "the implied warranty of MERCHANTABILITY or FITNESS FOR A "
							  "PARTICULAR PURPOSE. See the GNU General Public License for "
							  "more details.\n\nYou should have received a copy of the GNU "
							  "General Public License along with this program. "
							  "If not, see <a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a></p>").arg(ARCHNAME).arg(VERSION_NUMBER));
	m_uitimer = new QTimer();
	connect(m_uitimer, SIGNAL(timeout()), this, SLOT(update_ui()));
	m_uitimer->start(10);
}

void DudeStar::update_ui()
{
	static uint8_t cnt = 0;
	static uint64_t last_rxcnt = 0;
	static uint16_t max = 0;

	if (cnt >= 25){
		if(m_rxcnt == last_rxcnt){
			m_outlevel = 0;
			m_rxcnt = 0;
			qreal db = 10 * log10f(0);
			m_labeldb->setText(QString::asprintf("%02.2f", db));
			//qDebug() << "EOT or TIMEOUT";
		}
		else{
			last_rxcnt = m_rxcnt;
		}
		max = 0;
		cnt = 0;
	}
	else{
		++cnt;
	}

	qreal l = (qreal)m_outlevel / 32767.0;
	m_levelmeter->setLevel(l);

	if(m_outlevel > max){
		max = m_outlevel;
		l = (qreal)max / 32767.0;
		qreal db = 10 * log10f(l);
		if(db > -0.2){
			m_labeldb->setStyleSheet("QLabel { background-color : red; padding: 2; }");
		}
		else {
			m_labeldb->setStyleSheet("QLabel { background-color : #353535; padding: 2;}");
		}
		m_labeldb->setText(QString::asprintf("%02.2f", db));//QString("  %1").arg(db, 1, 'g', 2));
	}
}

void DudeStar::save_settings()
{
	m_settings->setValue("PLAYBACK", ui->comboPlayback->currentText());
	m_settings->setValue("CAPTURE", ui->comboCapture->currentText());
	m_settings->setValue("IPV6", ui->checkIPv6->isChecked() ? "true" : "false");
	m_settings->setValue("MODE", ui->comboMode->currentText());
	m_settings->setValue("REFHOST", saved_refhost);
	m_settings->setValue("DCSHOST", saved_dcshost);
	m_settings->setValue("XRFHOST", saved_xrfhost);
	m_settings->setValue("YSFHOST", saved_ysfhost);
	m_settings->setValue("FCSHOST", saved_fcshost);
	m_settings->setValue("DMRHOST", saved_dmrhost);
	m_settings->setValue("P25HOST", saved_p25host);
	m_settings->setValue("NXDNHOST", saved_nxdnhost);
	m_settings->setValue("M17HOST", saved_m17host);
	m_settings->setValue("MODULE", ui->comboModule->currentText());
	m_settings->setValue("CALLSIGN", ui->editCallsign->text());
	m_settings->setValue("DMRID", ui->editDMRID->text());
	m_settings->setValue("ESSID", ui->comboESSID->currentText());
	m_settings->setValue("DMRPASSWORD", ui->editPassword->text());
	m_settings->setValue("DMRTGID", ui->editTG->text());
	m_settings->setValue("DMRLAT", ui->editLat->text());
	m_settings->setValue("DMRLONG", ui->editLong->text());
	m_settings->setValue("DMRLOC", ui->editLocation->text());
	m_settings->setValue("DMRDESC", ui->editDesc->text());
	m_settings->setValue("DMRFREQ", ui->editFreq->text());
	m_settings->setValue("DMRURL", ui->editURL->text());
	m_settings->setValue("DMRSWID", ui->editSWID->text());
	m_settings->setValue("DMRPKID", ui->editPKID->text());
	m_settings->setValue("DMROPTS", ui->editDMROptions->text());
	m_settings->setValue("MYCALL", ui->editMYCALL->text().simplified());
	m_settings->setValue("URCALL", ui->editURCALL->text().simplified());
	m_settings->setValue("RPTR1", ui->editRPTR1->text().simplified());
	m_settings->setValue("RPTR2", ui->editRPTR2->text().simplified());
	m_settings->setValue("XRF2REF", ui->checkIPv6->isChecked() ? "true" : "false");
	m_settings->setValue("USRTXT", ui->editUserTxt->text());
	m_settings->setValue("IAXUSER", ui->editIAXUsername->text());
	m_settings->setValue("IAXPASS", ui->editIAXPassword->text());
	m_settings->setValue("IAXNODE", ui->editIAXNode->text());
	m_settings->setValue("IAXHOST", ui->editIAXHost->text());
	m_settings->setValue("IAXPORT", ui->editIAXPort->text());
}

void DudeStar::download_file(QString f)
{
	HttpManager *http = new HttpManager("/"+f);
	QThread *httpThread = new QThread;
	http->moveToThread(httpThread);
	connect(httpThread, SIGNAL(started()), http, SLOT(process()));
	connect(http, SIGNAL(file_downloaded(QString)), this, SLOT(file_downloaded(QString)));
	connect(httpThread, SIGNAL(finished()), http, SLOT(deleteLater()));
	ui->textLog->append("Downloading " + f + " ...");
	httpThread->start();
}

void DudeStar::file_downloaded(QString filename)
{
	qDebug() << "DudeStar::file_downloaded() " << filename;
	ui->textLog->append("Downloaded " + filename);
	QString m = ui->comboMode->currentText();
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
		else if(filename == "FCSHosts.txt" && m == "FCS"){
			process_fcs_rooms();
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
		else if(filename == "M17Hosts-full.csv" && m == "M17"){
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
	if(ui->comboMode->currentText().simplified() == "REF"){
		saved_refhost = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "DCS"){
		saved_dcshost = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "XRF"){
		saved_xrfhost = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "YSF"){
		saved_ysfhost = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "FCS"){
		saved_fcshost = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "DMR"){
		saved_dmrhost = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "P25"){
		saved_p25host = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "NXDN"){
		saved_nxdnhost = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "M17"){
		saved_m17host = h.simplified();
	}
	if(ui->comboMode->currentText().simplified() == "IAX"){
		saved_iaxhost = h.simplified();
	}
}

void DudeStar::process_mode_change(const QString &m)
{
	if(m == "REF"){
		process_ref_hosts();
		ui->comboModule->setEnabled(true);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editPassword->setEnabled(false);
		ui->editTG->setEnabled(false);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(true);
		ui->editURCALL->setEnabled(true);
		ui->editRPTR1->setEnabled(true);
		ui->editRPTR2->setEnabled(true);
		ui->editUserTxt->setEnabled(true);
		ui->radioButtonM173200->setEnabled(false);
		ui->radioButtonM171600->setEnabled(false);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("MYCALL");
		ui->label2->setText("URCALL");
		ui->label3->setText("RPTR1");
		ui->label4->setText("RPTR2");
		ui->label5->setText("Stream ID");
		ui->label6->setText("User txt");
	}
	if(m == "DCS"){
		process_dcs_hosts();
		ui->comboModule->setEnabled(true);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editPassword->setEnabled(false);
		ui->editTG->setEnabled(false);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(true);
		ui->editURCALL->setEnabled(true);
		ui->editRPTR1->setEnabled(true);
		ui->editRPTR2->setEnabled(true);
		ui->editUserTxt->setEnabled(true);
		ui->radioButtonM173200->setEnabled(false);
		ui->radioButtonM171600->setEnabled(false);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("MYCALL");
		ui->label2->setText("URCALL");
		ui->label3->setText("RPTR1");
		ui->label4->setText("RPTR2");
		ui->label5->setText("Stream ID");
		ui->label6->setText("User txt");
	}
	if(m == "XRF"){
		process_xrf_hosts();
		ui->comboModule->setEnabled(true);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editPassword->setEnabled(false);
		ui->editTG->setEnabled(false);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(true);
		ui->editURCALL->setEnabled(true);
		ui->editRPTR1->setEnabled(true);
		ui->editRPTR2->setEnabled(true);
		ui->editUserTxt->setEnabled(true);
		ui->radioButtonM173200->setEnabled(false);
		ui->radioButtonM171600->setEnabled(false);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("MYCALL");
		ui->label2->setText("URCALL");
		ui->label3->setText("RPTR1");
		ui->label4->setText("RPTR2");
		ui->label5->setText("Stream ID");
		ui->label6->setText("User txt");
	}
	else if(m == "YSF"){
		process_ysf_hosts();
		ui->comboModule->setEnabled(false);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editPassword->setEnabled(false);
		ui->editTG->setEnabled(false);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(false);
		ui->editURCALL->setEnabled(false);
		ui->editRPTR1->setEnabled(false);
		ui->editRPTR2->setEnabled(false);
		ui->editUserTxt->setEnabled(false);
		ui->radioButtonM173200->setEnabled(true);
		ui->radioButtonM171600->setEnabled(true);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("Gateway");
		ui->label2->setText("Callsign");
		ui->label3->setText("Dest");
		ui->label4->setText("Type");
		ui->label5->setText("Path");
		ui->label6->setText("Frame#");
	}
	else if(m == "FCS"){
		process_fcs_rooms();
		ui->comboModule->setEnabled(false);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editPassword->setEnabled(false);
		ui->editTG->setEnabled(false);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(false);
		ui->editURCALL->setEnabled(false);
		ui->editRPTR1->setEnabled(false);
		ui->editRPTR2->setEnabled(false);
		ui->editUserTxt->setEnabled(false);
		ui->radioButtonM173200->setEnabled(true);
		ui->radioButtonM171600->setEnabled(true);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("Gateway");
		ui->label2->setText("Callsign");
		ui->label3->setText("Dest");
		ui->label4->setText("Type");
		ui->label5->setText("Path");
		ui->label6->setText("Frame#");
	}
	else if(m == "DMR"){
		process_dmr_hosts();
		ui->comboModule->setEnabled(false);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editPassword->setEnabled(true);
		ui->editTG->setEnabled(true);
		ui->comboCC->setEnabled(true);
		ui->comboSlot->setEnabled(true);
		ui->checkPrivate->setEnabled(true);
		ui->editMYCALL->setEnabled(false);
		ui->editURCALL->setEnabled(false);
		ui->editRPTR1->setEnabled(false);
		ui->editRPTR2->setEnabled(false);
		ui->editUserTxt->setEnabled(false);
		ui->radioButtonM173200->setEnabled(false);
		ui->radioButtonM171600->setEnabled(false);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("Callsign");
		ui->label2->setText("SrcID");
		ui->label3->setText("DestID");
		ui->label4->setText("GWID");
		ui->label5->setText("Seq#");
		ui->label6->setText("");
	}
	else if(m == "P25"){
		process_p25_hosts();
		ui->comboModule->setEnabled(false);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editPassword->setEnabled(false);
		ui->editTG->setEnabled(true);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(false);
		ui->editURCALL->setEnabled(false);
		ui->editRPTR1->setEnabled(false);
		ui->editRPTR2->setEnabled(false);
		ui->editUserTxt->setEnabled(false);
		ui->radioButtonM173200->setEnabled(false);
		ui->radioButtonM171600->setEnabled(false);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("Callsign");
		ui->label2->setText("SrcID");
		ui->label3->setText("DestID");
		ui->label4->setText("GWID");
		ui->label5->setText("Seq#");
		ui->label6->setText("");
	}
	else if(m == "NXDN"){
		process_nxdn_hosts();
		ui->comboModule->setEnabled(false);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editTG->setEnabled(false);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(false);
		ui->editURCALL->setEnabled(false);
		ui->editRPTR1->setEnabled(false);
		ui->editRPTR2->setEnabled(false);
		ui->editUserTxt->setEnabled(false);
		ui->radioButtonM173200->setEnabled(false);
		ui->radioButtonM171600->setEnabled(false);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("Callsign");
		ui->label2->setText("SrcID");
		ui->label3->setText("DestID");
		ui->label4->setText("");
		ui->label5->setText("Seq#");
		ui->label6->setText("");
	}
	else if(m == "M17"){
		process_m17_hosts();
		ui->comboModule->setEnabled(true);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(true);
		ui->editTG->setEnabled(false);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(false);
		ui->editURCALL->setEnabled(false);
		ui->editRPTR1->setEnabled(false);
		ui->editRPTR2->setEnabled(false);
		ui->editUserTxt->setEnabled(false);
		ui->radioButtonM173200->setEnabled(true);
		ui->radioButtonM171600->setEnabled(true);
		ui->labelHost->setVisible(true);
		ui->comboHost->setVisible(true);
		ui->labelIAXDTMF->setVisible(false);
		ui->editIAXDTMF->setVisible(false);
		ui->pushIAXDTMF->setVisible(false);
		ui->label1->setText("SrcID");
		ui->label2->setText("DstID");
		ui->label3->setText("Type");
		ui->label4->setText("Frame #");
		ui->label5->setText("Stream ID");
		ui->label6->setText("");
	}
	else if(m == "IAX"){
		//process_iax_hosts();
		ui->comboModule->setEnabled(false);
		ui->editDMRID->setEnabled(true);
		ui->comboESSID->setEnabled(false);
		ui->editTG->setEnabled(false);
		ui->comboCC->setEnabled(false);
		ui->comboSlot->setEnabled(false);
		ui->checkPrivate->setEnabled(false);
		ui->editMYCALL->setEnabled(false);
		ui->editURCALL->setEnabled(false);
		ui->editRPTR1->setEnabled(false);
		ui->editRPTR2->setEnabled(false);
		ui->editUserTxt->setEnabled(false);
		ui->radioButtonM173200->setEnabled(false);
		ui->radioButtonM171600->setEnabled(false);
		ui->labelHost->setVisible(false);
		ui->comboHost->setVisible(false);
		ui->labelIAXDTMF->setVisible(true);
		ui->editIAXDTMF->setVisible(true);
		ui->pushIAXDTMF->setVisible(true);
		ui->label1->setText("");
		ui->label2->setText("");
		ui->label3->setText("");
		ui->label4->setText("");
		ui->label5->setText("");
		ui->label6->setText("");
	}
}

void DudeStar::tts_changed(int b)
{
	qDebug() << "tts_changed() called";
	emit input_source_changed(b, ui->editTTSTXT->text());
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

void DudeStar::process_dtmf()
{
	QByteArray tx(ui->editIAXDTMF->text().simplified().toUtf8(), ui->editIAXDTMF->text().simplified().size());
	emit send_dtmf(tx);
}

void DudeStar::process_ref_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/dplus.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/dplus.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					hostmap[ll.at(0).simplified()] = ll.at(1) + ",20001";
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->comboHost->findText(saved_refhost);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("dplus.txt");
	}
}

void DudeStar::process_dcs_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/dcs.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/dcs.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					hostmap[ll.at(0).simplified()] = ll.at(1) + ",30051";
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->comboHost->findText(saved_dcshost);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("dcs.txt");
	}
}

void DudeStar::process_xrf_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/dextra.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/dextra.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					hostmap[ll.at(0).simplified()] = ll.at(1) + ",30001";
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->comboHost->findText(saved_xrfhost);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("dextra.txt");
	}
}

void DudeStar::process_ysf_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/YSFHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/YSFHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split(';');
				if(ll.size() > 4){
					hostmap[ll.at(1).simplified() + " - " + ll.at(2).simplified()] = ll.at(3) + "," + ll.at(4);
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->comboHost->findText(saved_ysfhost);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("YSFHosts.txt");
	}
}

void DudeStar::process_fcs_rooms()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/FCSHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/FCSHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split(';');
				if(ll.size() > 4){
					hostmap[ll.at(0).simplified() + " - " + ll.at(1).simplified()] = ll.at(2).left(6).toLower() + ".xreflector.net,62500";
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->comboHost->findText(saved_fcshost);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("FCSHosts.txt");
	}
}

void DudeStar::process_dmr_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/DMRHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/DMRHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 4){
					if( (ll.at(0).simplified() != "DMRGateway")
					 && (ll.at(0).simplified() != "DMR2YSF")
					 && (ll.at(0).simplified() != "DMR2NXDN"))
					{
						hostmap[ll.at(0).simplified()] = ll.at(2) + "," + ll.at(4) + "," + ll.at(3);
					}
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->comboHost->findText(saved_dmrhost);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("DMRHosts.txt");
	}
}

void DudeStar::process_p25_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/P25Hosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/P25Hosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 2){
					hostmap[ll.at(0).simplified()] = ll.at(1) + "," + ll.at(2);
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->comboHost->findText(saved_p25host);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("P25Hosts.txt");
	}
}

void DudeStar::process_nxdn_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/NXDNHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/NXDNHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 2){
					hostmap[ll.at(0).simplified()] = ll.at(1) + "," + ll.at(2);
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();
		int i = ui->comboHost->findText(saved_nxdnhost);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("NXDNHosts.txt");
	}
}

void DudeStar::process_m17_hosts()
{
	QMap<QString, QString> hostmap;
	QFileInfo check_file(config_path + "/M17Hosts-full.csv");
	if(check_file.exists() && check_file.isFile()){
		ui->comboHost->blockSignals(true);
		QFile f(config_path + "/M17Hosts-full.csv");
		if(f.open(QIODevice::ReadOnly)){
			ui->comboHost->clear();
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(',');
				if(ll.size() > 3){
					hostmap[ll.at(0).simplified()] = ll.at(2) + "," + ll.at(4) + "," + ll.at(3);
				}
			}
			QMap<QString, QString>::const_iterator i = hostmap.constBegin();
			while (i != hostmap.constEnd()) {
				ui->comboHost->addItem(i.key(), i.value());
				++i;
			}
		}
		f.close();

		int i = ui->comboHost->findText(saved_m17host);
		ui->comboHost->setCurrentIndex(i);
		ui->comboHost->blockSignals(false);
	}
	else{
		download_file("M17Hosts-full.csv");
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
		download_file("dplus.txt");
	}

	check_file.setFile(config_path + "/dextra.txt");
	if( (!check_file.exists() && !check_file.isFile() ) || m_update_host_files  ){
		download_file("dextra.txt");
	}

	check_file.setFile(config_path + "/dcs.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file( "dcs.txt");
	}

	check_file.setFile(config_path + "/YSFHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("YSFHosts.txt");
	}

	check_file.setFile(config_path + "/FCSHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("FCSHosts.txt");
	}

	check_file.setFile(config_path + "/DMRHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("DMRHosts.txt");
	}

	check_file.setFile(config_path + "/P25Hosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("P25Hosts.txt");
	}

	check_file.setFile(config_path + "/NXDNHosts.txt");
	if((!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("NXDNHosts.txt");
	}

	check_file.setFile(config_path + "/M17Hosts-full.csv");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("M17Hosts-full.csv");
	}

	check_file.setFile(config_path + "/DMRIDs.dat");
	if(!check_file.exists() && !check_file.isFile()){
		download_file("DMRIDs.dat");
	}
	else {
		process_dmr_ids();
	}

	check_file.setFile(config_path + "/NXDN.csv");
	if(!check_file.exists() && !check_file.isFile()){
		download_file("NXDN.csv");
	}
	else{
		process_nxdn_ids();
	}
	m_update_host_files = false;
	//process_mode_change(ui->comboMode->currentText().simplified());
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
				if(ll.size() > 1){
					m_dmrids[ll.at(0).toUInt()] = ll.at(1);
				}
			}
		}
		f.close();
	}
	else{
		download_file("DMRIDs.dat");
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
	update_nxdn_ids();
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
					nxdnids[ll.at(0).toUInt()] = ll.at(1);
				}
			}
		}
		f.close();
	}
	else{
		download_file("NXDN.csv");
	}
}

void DudeStar::update_nxdn_ids()
{
	QFileInfo check_file(config_path + "/NXDN.csv");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/NXDN.csv");
		f.remove();
	}
	process_nxdn_ids();
}

void DudeStar::process_settings()
{
	ui->comboPlayback->setCurrentText(m_settings->value("PLAYBACK").toString().simplified());
	ui->comboCapture->setCurrentText(m_settings->value("CAPTURE").toString().simplified());
	(m_settings->value("IPV6").toString().simplified() == "true") ? ui->checkIPv6->setChecked(true) : ui->checkIPv6->setChecked(false);
	ui->comboMode->blockSignals(true);
	int i = ui->comboMode->findText(m_settings->value("MODE").toString().simplified());
	ui->comboMode->setCurrentIndex(i);
	process_mode_change(m_settings->value("MODE").toString().simplified());

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
		process_fcs_rooms();
	}
	else if(i == 5){
		process_dmr_hosts();
	}
	else if(i == 6){
		process_p25_hosts();
	}
	else if(i == 7){
		process_nxdn_hosts();
	}
	else if(i == 8){
		process_m17_hosts();
	}
	ui->comboMode->blockSignals(false);
	ui->comboHost->blockSignals(true);

	saved_refhost = m_settings->value("REFHOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "REF"){
		int i = ui->comboHost->findText(saved_refhost);
		ui->comboHost->setCurrentIndex(i);
	}

	saved_dcshost = m_settings->value("DCSHOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "DCS"){
		int i = ui->comboHost->findText(saved_dcshost);
		ui->comboHost->setCurrentIndex(i);
	}

	saved_xrfhost = m_settings->value("XRFHOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "XRF"){
		int i = ui->comboHost->findText(saved_xrfhost);
		ui->comboHost->setCurrentIndex(i);
	}

	saved_ysfhost = m_settings->value("YSFHOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "YSF"){
		int i = ui->comboHost->findText(saved_ysfhost);
		ui->comboHost->setCurrentIndex(i);
	}

	saved_fcshost = m_settings->value("FCSHOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "FCS"){
		int i = ui->comboHost->findText(saved_fcshost);
		ui->comboHost->setCurrentIndex(i);
	}

	saved_dmrhost = m_settings->value("DMRHOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "DMR"){
		int i = ui->comboHost->findText(saved_dmrhost);
		ui->comboHost->setCurrentIndex(i);
	}

	saved_p25host = m_settings->value("P25HOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "P25"){
		int i = ui->comboHost->findText(saved_p25host);
		ui->comboHost->setCurrentIndex(i);
	}

	saved_nxdnhost = m_settings->value("NXDNHOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "NXDN"){
		int i = ui->comboHost->findText(saved_nxdnhost);
		ui->comboHost->setCurrentIndex(i);
	}

	saved_m17host = m_settings->value("M17HOST").toString().simplified();
	if(ui->comboMode->currentText().simplified() == "M17"){
		int i = ui->comboHost->findText(saved_m17host);
		ui->comboHost->setCurrentIndex(i);
	}

	ui->comboModule->setCurrentText(m_settings->value("MODULE").toString().simplified());
	ui->editCallsign->setText(m_settings->value("CALLSIGN").toString());
	ui->editDMRID->setText(m_settings->value("DMRID").toString());

	if(m_settings->value("ESSID").toString().simplified() == "None"){
		ui->comboESSID->setCurrentIndex(0);
	}
	else{
		ui->comboESSID->setCurrentIndex(m_settings->value("ESSID").toString().simplified().toInt() + 1);
	}

	ui->editPassword->setText(m_settings->value("DMRPASSWORD").toString().simplified());
	ui->editTG->setText(m_settings->value("DMRTGID", "4000").toString().simplified());
	ui->editLat->setText(m_settings->value("DMRLAT", "0").toString().simplified());
	ui->editLong->setText(m_settings->value("DMRLONG", "0").toString().simplified());
	ui->editLocation->setText(m_settings->value("DMRLOC").toString().simplified());
	ui->editDesc->setText(m_settings->value("DMRDESC", "DudeStar").toString().simplified());
	ui->editFreq->setText(m_settings->value("DMRFREQ", "438800000").toString().simplified());
	ui->editURL->setText(m_settings->value("DMRURL", "www.qrz.com").toString().simplified());
	ui->editSWID->setText(m_settings->value("DMRSWID", "20210131").toString().simplified());
	ui->editPKID->setText(m_settings->value("DMRPKID", "MMDVM_DMO").toString().simplified());
	ui->editDMROptions->setText(m_settings->value("DMROPTS").toString().simplified());
	ui->editMYCALL->setText(m_settings->value("MYCALL").toString().simplified());
	ui->editURCALL->setText(m_settings->value("URCALL").toString().simplified());
	ui->editRPTR1->setText(m_settings->value("RPTR1").toString().simplified());
	ui->editRPTR2->setText(m_settings->value("RPTR2").toString().simplified());
	ui->editUserTxt->setText(m_settings->value("USRTXT").toString().simplified());
	(m_settings->value("XRF2REF").toString().simplified() == "true") ? ui->checkXrf2Ref->setChecked(true) : ui->checkXrf2Ref->setChecked(false);
	ui->editIAXUsername->setText(m_settings->value("IAXUSER").toString().simplified());
	ui->editIAXPassword->setText(m_settings->value("IAXPASS").toString().simplified());
	ui->editIAXNode->setText(m_settings->value("IAXNODE").toString().simplified());
	ui->editIAXHost->setText(m_settings->value("IAXHOST").toString().simplified());
	ui->editIAXPort->setText(m_settings->value("IAXPORT", "4569").toString().simplified());
	ui->comboHost->blockSignals(false);
}

void DudeStar::discover_vocoders()
{
	QMap<QString, QString> l = SerialAMBE::discover_devices();
	QMap<QString, QString>::const_iterator i = l.constBegin();
	ui->comboVocoder->addItem("Software vocoder", "");
	while (i != l.constEnd()) {
		ui->comboVocoder->addItem(i.value(), i.key());
		++i;
	}
}

void DudeStar::process_connect()
{
	if(connect_status != Codec::DISCONNECTED){
		connect_status = Codec::DISCONNECTED;
		ui->textLog->append("Disconnected");
		m_outlevel = 0;
		m_modethread->quit();
		//delete m_modethread;
		ui->pushConnect->setText("Connect");
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
		ui->data6->clear();
		ui->comboVocoder->setEnabled(true);
		ui->checkSWRX->setEnabled(true);
		ui->checkSWTX->setEnabled(true);
		ui->comboPlayback->setEnabled(true);
		ui->comboCapture->setEnabled(true);
		ui->comboMode->setEnabled(true);
		ui->comboHost->setEnabled(true);
		ui->editCallsign->setEnabled(true);

		ui->pushTX->setDisabled(true);
		status_txt->setText("Not connected");

		if(m_protocol == "DMR"){
			ui->editDMRID->setEnabled(true);
			ui->comboESSID->setEnabled(true);
			ui->editPassword->setEnabled(true);
		}

		if( (m_protocol == "P25") || (m_protocol == "NXDN") ){
			ui->editDMRID->setEnabled(true);
		}

		if((m_protocol == "DCS") || (m_protocol == "XRF") || (m_protocol == "M17")){
			ui->comboModule->setEnabled(true);
		}
    }
	else if( (connect_status == Codec::DISCONNECTED) &&
			 (ui->comboHost->currentText().size() == 0) &&
			 (ui->comboMode->currentText() != "IAX") )
	{
		QMessageBox::warning(this, tr("Select host"), tr("No host selected"));
	}
    else{
#ifdef Q_OS_MACOS
        int r = MicPermission::check_permission();
        fprintf(stderr, "check_permission() returned %d\n", r);
        ui->textLog->append("check_permission() returned " + QString::number(r));
#endif
		callsign = ui->editCallsign->text().toUpper();
		dmrid = ui->editDMRID->text().toUInt();

		if( (callsign.size() < 4) ||
			(dmrid < 250000) ||
			(callsign != m_dmrids[dmrid]))
		{
			QMessageBox::warning(this, tr("Connection refused"),
								 tr("A valid callsign and DMR ID are required to use Dudestar on any mode, and they must match. "
									"If you have entered a valid DMR ID that matches the entered callsign, and you are still seeing "
									"this message, then you either have to click update ID files button or wait until your DMR ID "
									"is added to the ID file and try again."));
			return;
		}

		connect_status = Codec::CONNECTING;
		status_txt->setText("Connecting...");
		ui->pushConnect->setText("Connecting");
		m_protocol = ui->comboMode->currentText();
		callsign = ui->editCallsign->text().toUpper();
		ui->editCallsign->setText(callsign);
		ui->editMYCALL->setText(ui->editMYCALL->text().toUpper());
		ui->editURCALL->setText(ui->editURCALL->text().toUpper());
		ui->editRPTR1->setText(ui->editRPTR1->text().toUpper());
		module = ui->comboModule->currentText().toStdString()[0];
		hostname = ui->comboHost->currentText().simplified();
		QStringList sl = ui->comboHost->currentData().toString().simplified().split(',');

		if(m_protocol != "IAX"){
			if( (m_protocol == "M17") && (ui->checkIPv6->isChecked()) && (sl.size() > 2) && (sl.at(2) != "none") ){
				host = sl.at(2).simplified();
			}
			else{
				host = sl.at(0).simplified();
			}
			port = sl.at(1).toInt();
		}
		ui->textLog->append("Connecting to " + host + ":" + QString::number(port) + "...");
		if( (m_protocol == "REF") || ((m_protocol == "XRF") && (ui->checkXrf2Ref->isChecked())) ){
			const char m = ui->comboModule->currentIndex() + 0x41;
			m_ref = new REFCodec(callsign, hostname, m, host, 20001, false, ui->comboVocoder->currentData().toString().simplified(), ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_ref->moveToThread(m_modethread);
			connect(m_ref, SIGNAL(update(Codec::MODEINFO)), this, SLOT(update_ref_data(Codec::MODEINFO)));
			connect(m_ref, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			connect(m_modethread, SIGNAL(started()), m_ref, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_ref, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_ref, SLOT(input_src_changed(int, QString)));
			connect(ui->comboModule, SIGNAL(currentIndexChanged(int)), m_ref, SLOT(module_changed(int)));
			connect(ui->editMYCALL, SIGNAL(textChanged(QString)), m_ref, SLOT(mycall_changed(QString)));
			connect(ui->editURCALL, SIGNAL(textChanged(QString)), m_ref, SLOT(urcall_changed(QString)));
			connect(ui->editRPTR1, SIGNAL(textChanged(QString)), m_ref, SLOT(rptr1_changed(QString)));
			connect(ui->editRPTR2, SIGNAL(textChanged(QString)), m_ref, SLOT(rptr2_changed(QString)));
			connect(ui->checkSWRX, SIGNAL(stateChanged(int)), m_ref, SLOT(swrx_state_changed(int)));
			connect(ui->checkSWTX, SIGNAL(stateChanged(int)), m_ref, SLOT(swtx_state_changed(int)));
			connect(ui->pushTX, SIGNAL(pressed()), m_ref, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_ref, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_ref, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_ref, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_ref, SLOT(decoder_gain_changed(qreal)));
			ui->editRPTR2->setText(hostname + " " + module);
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			emit ui->editMYCALL->textChanged(ui->editMYCALL->text());
			emit ui->editURCALL->textChanged(ui->editURCALL->text());
			emit ui->editRPTR1->textChanged(ui->editRPTR1->text());
			emit ui->editRPTR2->textChanged(ui->editRPTR2->text());
			m_modethread->start();
		}
		if(m_protocol == "DCS"){
			const char m = ui->comboModule->currentIndex() + 0x41;
			m_dcs = new DCSCodec(callsign, hostname, m, host, port, false, ui->comboVocoder->currentData().toString().simplified(), ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_dcs->moveToThread(m_modethread);
			connect(m_dcs, SIGNAL(update(Codec::MODEINFO)), this, SLOT(update_dcs_data(Codec::MODEINFO)));
			connect(m_dcs, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			connect(m_modethread, SIGNAL(started()), m_dcs, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_dcs, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_dcs, SLOT(input_src_changed(int, QString)));
			connect(ui->comboModule, SIGNAL(currentIndexChanged(int)), m_dcs, SLOT(module_changed(int)));
			connect(ui->editMYCALL, SIGNAL(textChanged(QString)), m_dcs, SLOT(mycall_changed(QString)));
			connect(ui->editURCALL, SIGNAL(textChanged(QString)), m_dcs, SLOT(urcall_changed(QString)));
			connect(ui->editRPTR1, SIGNAL(textChanged(QString)), m_dcs, SLOT(rptr1_changed(QString)));
			connect(ui->editRPTR2, SIGNAL(textChanged(QString)), m_dcs, SLOT(rptr2_changed(QString)));
			connect(ui->checkSWRX, SIGNAL(stateChanged(int)), m_dcs, SLOT(swrx_state_changed(int)));
			connect(ui->checkSWTX, SIGNAL(stateChanged(int)), m_dcs, SLOT(swtx_state_changed(int)));
			connect(ui->pushTX, SIGNAL(pressed()), m_dcs, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_dcs, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_dcs, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_dcs, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_dcs, SLOT(decoder_gain_changed(qreal)));
			ui->editRPTR2->setText(hostname + " " + module);
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			emit ui->editMYCALL->textChanged(ui->editMYCALL->text());
			emit ui->editURCALL->textChanged(ui->editURCALL->text());
			emit ui->editRPTR1->textChanged(ui->editRPTR1->text());
			emit ui->editRPTR2->textChanged(ui->editRPTR2->text());
			m_modethread->start();
		}
		if( (m_protocol == "XRF") && (ui->checkXrf2Ref->isChecked() == false) ){
			const char m = ui->comboModule->currentIndex() + 0x41;
			m_xrf = new XRFCodec(callsign, hostname, m, host, port, false, ui->comboVocoder->currentData().toString().simplified(), ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_xrf->moveToThread(m_modethread);
			connect(m_xrf, SIGNAL(update(Codec::MODEINFO)), this, SLOT(update_xrf_data(Codec::MODEINFO)));
			connect(m_xrf, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			connect(m_modethread, SIGNAL(started()), m_xrf, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_xrf, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_xrf, SLOT(input_src_changed(int, QString)));
			connect(ui->editMYCALL, SIGNAL(textChanged(QString)), m_xrf, SLOT(mycall_changed(QString)));
			connect(ui->editURCALL, SIGNAL(textChanged(QString)), m_xrf, SLOT(urcall_changed(QString)));
			connect(ui->editRPTR1, SIGNAL(textChanged(QString)), m_xrf, SLOT(rptr1_changed(QString)));
			connect(ui->editRPTR2, SIGNAL(textChanged(QString)), m_xrf, SLOT(rptr2_changed(QString)));
			connect(ui->checkSWRX, SIGNAL(stateChanged(int)), m_xrf, SLOT(swrx_state_changed(int)));
			connect(ui->checkSWTX, SIGNAL(stateChanged(int)), m_xrf, SLOT(swtx_state_changed(int)));
			connect(ui->pushTX, SIGNAL(pressed()), m_xrf, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_xrf, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_xrf, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_xrf, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_xrf, SLOT(decoder_gain_changed(qreal)));
			ui->editRPTR2->setText(hostname + " " + module);
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			emit ui->editMYCALL->textChanged(ui->editMYCALL->text());
			emit ui->editURCALL->textChanged(ui->editURCALL->text());
			emit ui->editRPTR1->textChanged(ui->editRPTR1->text());
			emit ui->editRPTR2->textChanged(ui->editRPTR2->text());
			m_modethread->start();
		}
		if( (m_protocol == "YSF") || (m_protocol == "FCS") ){
			m_ysf = new YSFCodec(callsign, hostname, host, port, false, ui->comboVocoder->currentData().toString().simplified(), ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_ysf->moveToThread(m_modethread);
			connect(m_ysf, SIGNAL(update(Codec::MODEINFO)), this, SLOT(update_ysf_data(Codec::MODEINFO)));
			connect(m_ysf, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			ui->radioButtonM171600->setChecked(true);
			connect(this, SIGNAL(rate_changed(int)), m_ysf, SLOT(rate_changed(int)));
			connect(m_modethread, SIGNAL(started()), m_ysf, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_ysf, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_ysf, SLOT(input_src_changed(int, QString)));
			connect(ui->checkSWRX, SIGNAL(stateChanged(int)), m_ysf, SLOT(swrx_state_changed(int)));
			connect(ui->checkSWTX, SIGNAL(stateChanged(int)), m_ysf, SLOT(swtx_state_changed(int)));
			connect(ui->pushTX, SIGNAL(pressed()), m_ysf, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_ysf, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_ysf, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_ysf, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_ysf, SLOT(decoder_gain_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			m_modethread->start();
		}
		if(m_protocol == "DMR"){
			dmrid = ui->editDMRID->text().toUInt();
			dmr_password = (ui->editPassword->text().isEmpty()) ? sl.at(2).simplified() : ui->editPassword->text();
			dmr_destid = ui->editTG->text().toUInt();
			uint8_t essid = ui->comboESSID->currentIndex();
			QString opts = (ui->checkDMROptions->isChecked()) ? ui->editDMROptions->text() : "";
			m_dmr = new DMRCodec(callsign, dmrid, essid, dmr_password, ui->editLat->text(), ui->editLong->text(), ui->editLocation->text(), ui->editDesc->text(), ui->editFreq->text(), ui->editURL->text(), ui->editSWID->text(), ui->editPKID->text(), opts, dmr_destid, host, port, false, ui->comboVocoder->currentData().toString().simplified(), ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_dmr->moveToThread(m_modethread);
			connect(m_dmr, SIGNAL(update(Codec::MODEINFO)), this, SLOT(update_dmr_data(Codec::MODEINFO)));
			connect(m_dmr, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			connect(m_modethread, SIGNAL(started()), m_dmr, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_dmr, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_dmr, SLOT(input_src_changed(int, QString)));
			connect(this, SIGNAL(dmr_tgid_changed(unsigned int)), m_dmr, SLOT(dmr_tgid_changed(unsigned int)));
			connect(ui->checkPrivate, SIGNAL(stateChanged(int)), m_dmr, SLOT(dmrpc_state_changed(int)));
			ui->checkPrivate->isChecked() ? emit ui->checkPrivate->stateChanged(2) : emit ui->checkPrivate->stateChanged(0);
			connect(ui->checkSWRX, SIGNAL(stateChanged(int)), m_dmr, SLOT(swrx_state_changed(int)));
			connect(ui->checkSWTX, SIGNAL(stateChanged(int)), m_dmr, SLOT(swtx_state_changed(int)));
			connect(ui->pushTX, SIGNAL(pressed()), m_dmr, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_dmr, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_dmr, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_dmr, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_dmr, SLOT(decoder_gain_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			m_modethread->start();
		}
		if(m_protocol == "P25"){
			dmrid = ui->editDMRID->text().toUInt();
			dmr_destid = ui->editTG->text().toUInt();
			m_p25 = new P25Codec(callsign, dmrid, dmr_destid, host, port, false, ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_p25->moveToThread(m_modethread);
			connect(m_p25, SIGNAL(update(Codec::MODEINFO)), this, SLOT(update_p25_data(Codec::MODEINFO)));
			connect(m_p25, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			connect(m_modethread, SIGNAL(started()), m_p25, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_p25, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_p25, SLOT(input_src_changed(int, QString)));
			connect(this, SIGNAL(dmr_tgid_changed(unsigned int)), m_p25, SLOT(dmr_tgid_changed(unsigned int)));
			connect(ui->pushTX, SIGNAL(pressed()), m_p25, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_p25, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_p25, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_p25, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_p25, SLOT(decoder_gain_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			m_modethread->start();
		}
		if(m_protocol == "NXDN"){
			dmr_destid = ui->comboHost->currentText().toUInt();
			m_nxdn = new NXDNCodec(callsign, nxdnids.key(callsign), dmr_destid, host, port, false, ui->comboVocoder->currentData().toString().simplified(), ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_nxdn->moveToThread(m_modethread);
			connect(m_nxdn, SIGNAL(update(Codec::MODEINFO)), this, SLOT(update_nxdn_data(Codec::MODEINFO)));
			connect(m_nxdn, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			connect(m_modethread, SIGNAL(started()), m_nxdn, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_nxdn, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_nxdn, SLOT(input_src_changed(int, QString)));
			connect(ui->checkSWRX, SIGNAL(stateChanged(int)), m_nxdn, SLOT(swrx_state_changed(int)));
			connect(ui->checkSWTX, SIGNAL(stateChanged(int)), m_nxdn, SLOT(swtx_state_changed(int)));
			connect(ui->pushTX, SIGNAL(pressed()), m_nxdn, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_nxdn, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_nxdn, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_nxdn, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_nxdn, SLOT(decoder_gain_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			m_modethread->start();
		}
		if(m_protocol == "M17"){
			m_m17 = new M17Codec(callsign, module, hostname, host, port, false, ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_m17->moveToThread(m_modethread);
			connect(m_m17, SIGNAL(update(Codec::MODEINFO)), this, SLOT(update_m17_data(Codec::MODEINFO)));
			connect(m_m17, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			ui->radioButtonM173200->setChecked(true);
			connect(this, SIGNAL(rate_changed(int)), m_m17, SLOT(rate_changed(int)));
			connect(m_modethread, SIGNAL(started()), m_m17, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_m17, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_m17, SLOT(input_src_changed(int, QString)));
			connect(ui->pushTX, SIGNAL(pressed()), m_m17, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_m17, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_m17, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_m17, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_m17, SLOT(decoder_gain_changed(qreal)));
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			m_modethread->start();
		}
		if(m_protocol == "IAX"){
			m_iaxuser = ui->editIAXUsername->text();
			m_iaxpassword = ui->editIAXPassword->text();
			m_iaxnode = ui->editIAXNode->text();
			m_iaxhost = ui->editIAXHost->text();
			m_iaxport = ui->editIAXPort->text().toUInt();
			m_iax = new IAXCodec(callsign, m_iaxuser, m_iaxpassword, m_iaxnode, m_iaxhost, m_iaxport, ui->comboCapture->currentText(), ui->comboPlayback->currentText());
			m_modethread = new QThread;
			m_iax->moveToThread(m_modethread);
			connect(m_iax, SIGNAL(update()), this, SLOT(update_iax_data()));
			connect(m_iax, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
			connect(m_modethread, SIGNAL(started()), m_iax, SLOT(send_connect()));
			connect(m_modethread, SIGNAL(finished()), m_iax, SLOT(deleteLater()));
			connect(this, SIGNAL(input_source_changed(int, QString)), m_iax, SLOT(input_src_changed(int, QString)));
			connect(ui->pushTX, SIGNAL(pressed()), m_iax, SLOT(start_tx()));
			connect(ui->pushTX, SIGNAL(released()), m_iax, SLOT(stop_tx()));
			connect(this, SIGNAL(out_audio_vol_changed(qreal)), m_iax, SLOT(out_audio_vol_changed(qreal)));
			connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_iax, SLOT(in_audio_vol_changed(qreal)));
			connect(this, SIGNAL(codec_gain_changed(qreal)), m_iax, SLOT(decoder_gain_changed(qreal)));
			connect(this, SIGNAL(send_dtmf(QByteArray)), m_iax, SLOT(send_dtmf(QByteArray)));
			emit input_source_changed(tts_voices->checkedId(), ui->editTTSTXT->text());
			m_modethread->start();
		}
    }
}

void DudeStar::process_codecgain_changed(int v)
{
	qreal vf = v / 100.0;
	qreal db = 10 * log10f(vf);
	ui->labelCodecGain->setText(QString("%1dB").arg(db, 1, 'g', 2));
	emit codec_gain_changed(vf);
}

void DudeStar::process_volume_changed(int v)
{
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(!muted){
		emit out_audio_vol_changed(linear_vol);
	}
}

void DudeStar::process_mute_button()
{
	int v = ui->sliderVolume->value();
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(muted){
		muted = false;
		emit out_audio_vol_changed(linear_vol);
	}
	else{
		muted = true;
		emit out_audio_vol_changed(0.0);
	}
}

void DudeStar::process_mic_gain_changed(int v)
{
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(!input_muted){
		emit in_audio_vol_changed(linear_vol);
	}
}

void DudeStar::process_mic_mute_button()
{
	int v = ui->sliderMic->value();
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(input_muted){
		input_muted = false;
		emit in_audio_vol_changed(linear_vol);
	}
	else{
		input_muted = true;
		emit in_audio_vol_changed(linear_vol);
	}
}

void DudeStar::update_iax_data()
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_iax_data() called after disconnected";
		return;
	}
	if((connect_status == Codec::CONNECTING) && (m_iax->get_status() == Codec::DISCONNECTED)){
		process_connect();
		QMessageBox::warning(this, tr("Connection refused"), tr("IAX connection refused.  Check allstar settings and confirm that you have permission to connect to this node."));
		return;
	}
	if( (connect_status == Codec::CONNECTING) && ( m_iax->get_status() == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->comboModule->setEnabled(false);
		ui->pushTX->setDisabled(false);
		ui->checkSWRX->setChecked(true);
		ui->checkSWTX->setChecked(true);
		ui->checkSWRX->setEnabled(false);
		ui->checkSWTX->setEnabled(false);
		process_codecgain_changed(ui->sliderCodecGain->value());
	}
	status_txt->setText(" Host: " + m_iax->get_host() + ":" + QString::number( m_iax->get_port()) + " Ping: " + QString::number(m_iax->get_cnt()));
	++m_rxcnt;
}

void DudeStar::update_m17_data(Codec::MODEINFO info)
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_m17_data() called after disconnected";
		return;
	}
	if((connect_status == Codec::CONNECTING) && (info.status == Codec::DISCONNECTED)){
		process_connect();
		QMessageBox::warning(this, tr("Connection refused"), tr("M17 connection refused.  Check callsign and confirm this callsign or IP is not already connected to this reflector"));
		return;
	}
	if( (connect_status == Codec::CONNECTING) && (info.status == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->comboModule->setEnabled(false);
		ui->pushTX->setDisabled(false);
		ui->checkSWRX->setChecked(true);
		ui->checkSWTX->setChecked(true);
		ui->checkSWRX->setEnabled(false);
		ui->checkSWTX->setEnabled(false);
		process_codecgain_changed(ui->sliderCodecGain->value());
		ui->textLog->append("Connected to " + info.host + ":" + QString::number( info.port));
	}
	status_txt->setText(" Host: " + info.host + ":" + QString::number( info.port) + " Ping: " + QString::number(info.count));
	if(info.streamid){
		ui->data1->setText(info.src);
		ui->data2->setText(info.dst);
		ui->data3->setText(info.type ? "3200 Voice" : "1600 V/D");
		if(info.frame_number){
			QString n = QString("%1").arg(info.frame_number, 4, 16, QChar('0'));
			ui->data4->setText(n);
		}
		ui->data5->setText(QString::number(info.streamid, 16));
	}
	else{
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
	}
	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Codec::STREAM_NEW){
		ui->textLog->append(t + " M17 RX started id: " + QString::number(info.streamid, 16) + " src: " + info.src + " dst: " + info.dst);
	}
	if(info.stream_state == Codec::STREAM_END){
		ui->textLog->append(t + " M17 RX ended id: " + QString::number(info.streamid, 16) + " src: " + info.src + " dst: " + info.dst);
	}
	if(info.stream_state == Codec::STREAM_LOST){
		ui->textLog->append(t + " M17 RX lost id: " + QString::number(info.streamid, 16) + " src: " + info.src + " dst: " + info.dst);
	}
	++m_rxcnt;
}

void DudeStar::update_ysf_data(Codec::MODEINFO info)
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_ysf_data() called after disconnected";
		return;
	}
	if( (connect_status == Codec::CONNECTING) && (info.status == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->comboModule->setEnabled(false);
		ui->pushTX->setDisabled(false);

		ui->checkSWRX->setChecked(!(m_ysf->get_hwrx()));
		if(!(m_ysf->get_hwrx())){
			ui->checkSWRX->setEnabled(false);
		}
		ui->checkSWTX->setChecked(!(m_ysf->get_hwtx()));
		if(!(m_ysf->get_hwtx())){
			ui->checkSWTX->setEnabled(false);
		}
		process_codecgain_changed(ui->sliderCodecGain->value());
	}

	status_txt->setText(" Host: " + info.host + ":" + QString::number( info.port) + " Ping: " + QString::number(info.count));
	if(info.stream_state == Codec::STREAM_IDLE){
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
		ui->data6->clear();
	}
	else{
		ui->data1->setText(info.gw);
		ui->data2->setText(info.src);
		ui->data3->setText(info.dst);
		if(info.type == 0){
			ui->data4->setText("V/D mode 1");
		}
		else if(info.type == 1){
			ui->data4->setText("Data Full Rate");
		}
		else if(info.type == 2){
			ui->data4->setText("V/D mode 2");
		}
		else if(info.type == 3){
			ui->data4->setText("Voice Full Rate");
		}
		else{
			ui->data4->setText("");
		}
		if(info.type >= 0){
			ui->data5->setText(info.path  ? "Internet" : "Local");
			ui->data6->setText(QString::number(info.frame_number) + "/" + QString::number(info.frame_total));
		}
	}

	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Codec::STREAM_NEW){
		ui->textLog->append(t + " YSF RX started");
	}
	if(info.stream_state == Codec::STREAM_END){
		ui->textLog->append(t + " YSF RX ended");
	}
	if(info.stream_state == Codec::STREAM_LOST){
		ui->textLog->append(t + " YSF RX lost");
	}
	++m_rxcnt;
}

void DudeStar::update_nxdn_data(Codec::MODEINFO info)
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_nxdn_data() called after disconnected";
		return;
	}
	if( (connect_status == Codec::CONNECTING) && ( info.status == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->comboModule->setEnabled(false);
		ui->pushTX->setDisabled(false);

		ui->checkSWRX->setChecked(!(m_nxdn->get_hwrx()));
		if(!(m_nxdn->get_hwrx())){
			ui->checkSWRX->setEnabled(false);
		}
		ui->checkSWTX->setChecked(!(m_nxdn->get_hwtx()));
		if(!(m_nxdn->get_hwtx())){
			ui->checkSWTX->setEnabled(false);
		}
		process_codecgain_changed(ui->sliderCodecGain->value());
	}
	status_txt->setText(" Host: " + info.host + ":" + QString::number( info.port) + " Ping: " + QString::number(info.count));
	if(info.stream_state == Codec::STREAM_IDLE){
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
		ui->data6->clear();
	}
	else{
		if(info.srcid){
			ui->data1->setText(nxdnids[info.srcid]);
		}
		ui->data2->setText(QString::number(info.srcid));
		ui->data3->setText(info.dstid ? QString::number(info.dstid) : "");
		if(info.frame_number){
			QString n = QString("%1").arg(info.frame_number, 2, 16, QChar('0'));
			ui->data5->setText(n);
		}
	}
	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Codec::STREAM_NEW){
		ui->textLog->append(t + " NXDN RX started src: " + info.srcid + " dst: " + info.dstid);
	}
	if(info.stream_state == Codec::STREAM_END){
		ui->textLog->append(t + " NXDN RX ended src: " + info.srcid + " dst: " + info.dstid);
	}
	if(info.stream_state == Codec::STREAM_LOST){
		ui->textLog->append(t + " NXDN RX lost src: " + info.srcid + " dst: " + info.dstid);
	}
	++m_rxcnt;
}

void DudeStar::update_p25_data(Codec::MODEINFO info)
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_p25_data() called after disconnected";
		return;
	}
	if( (connect_status == Codec::CONNECTING) && ( info.status == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->editDMRID->setEnabled(false);
		ui->comboModule->setEnabled(false);
		ui->pushTX->setDisabled(false);
		ui->checkSWRX->setChecked(true);
		ui->checkSWTX->setChecked(true);
		ui->checkSWRX->setEnabled(false);
		ui->checkSWTX->setEnabled(false);
		process_codecgain_changed(ui->sliderCodecGain->value());
	}
	status_txt->setText(" Host: " + info.host + ":" + QString::number( info.port) + " Ping: " + QString::number(info.count));
	if(info.stream_state == Codec::STREAM_IDLE){
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
		ui->data6->clear();
	}
	else{
		if(info.srcid){
			ui->data1->setText(m_dmrids[info.srcid]);
			ui->data2->setText(QString::number(info.srcid));
			ui->data4->setText(QString::number(info.srcid));
		}
		ui->data3->setText(info.dstid ? QString::number(info.dstid) : "");
		if(info.frame_number){
			QString n = QString("%1").arg(info.frame_number, 2, 16, QChar('0'));
			ui->data5->setText(n);
		}
	}
	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Codec::STREAM_NEW){
		ui->textLog->append(t + " DMR RX started src: " + QString::number(info.srcid) + " dst: " + QString::number(info.dstid));
	}
	if(info.stream_state == Codec::STREAM_END){
		ui->textLog->append(t + " DMR RX ended src: " + QString::number(info.srcid) + " dst: " + QString::number(info.dstid));
	}
	if(info.stream_state == Codec::STREAM_LOST){
		ui->textLog->append(t + " DMR RX lost src: " + QString::number(info.srcid) + " dst: " + QString::number(info.dstid));
	}
	++m_rxcnt;
}

void DudeStar::update_dmr_data(Codec::MODEINFO info)
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_dmr_data() called after disconnected";
		return;
	}
	if((connect_status == Codec::CONNECTING) && (info.status == Codec::DISCONNECTED)){
		process_connect();
		QMessageBox::warning(this, tr("Connection refused"), tr("DMR connection refused.  Check callsign, DMR ID, ESSID, and hotspot security password.  Some servers require specific information for Lat/Long/Location/Description/Freq/URL/Software ID/Package ID, namely BrandMeister.  It's up to you to determine this information from the server you are attempting to connect to."));
		return;
	}
	if(info.status == Codec::CLOSED){
		process_connect();
		QMessageBox::warning(this, tr("Connection closed"), tr("Received MSTCL (Master closing connection) from DMR server. If this is BrandMeister, you may need to use different values for Software ID and Package ID in the Settings tab."));
		return;
	}
	if((connect_status == Codec::CONNECTING) && (info.status == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->editDMRID->setEnabled(false);
		ui->comboESSID->setEnabled(false);
		ui->editPassword->setEnabled(false);
		ui->pushTX->setDisabled(false);
		//ui->editTG->setEnabled(false);
		ui->checkSWRX->setChecked(!(m_dmr->get_hwrx()));
		if(!(m_dmr->get_hwrx())){
			ui->checkSWRX->setEnabled(false);
		}
		ui->checkSWTX->setChecked(!(m_dmr->get_hwtx()));
		if(!(m_dmr->get_hwtx())){
			ui->checkSWTX->setEnabled(false);
		}
		process_codecgain_changed(ui->sliderCodecGain->value());
	}
	status_txt->setText(" Host: " + info.host + ":" + QString::number(info.port) + " Ping: " + QString::number(info.count));
	if(info.stream_state == Codec::STREAM_IDLE){
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
		ui->data6->clear();
	}
	else{
		if(info.srcid){
			ui->data1->setText(m_dmrids[info.srcid]);
			ui->data2->setText(QString::number(info.srcid));
		}
		ui->data3->setText(info.dstid ? QString::number(info.dstid) : "");
		ui->data4->setText(info.gwid ? QString::number(info.gwid) : "");
		if(info.frame_number){
			QString n = QString("%1").arg(info.frame_number, 2, 16, QChar('0'));
			ui->data5->setText(n);
		}
	}
	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Codec::STREAM_NEW){
		ui->textLog->append(t + " DMR RX started src: " + QString::number(info.srcid) + " dst: " + QString::number(info.dstid));
	}
	if(info.stream_state == Codec::STREAM_END){
		ui->textLog->append(t + " DMR RX ended src: " + QString::number(info.srcid) + " dst: " + QString::number(info.dstid));
	}
	if(info.stream_state == Codec::STREAM_LOST){
		ui->textLog->append(t + " DMR RX lost src: " + QString::number(info.srcid) + " dst: " + QString::number(info.dstid));
	}
	++m_rxcnt;
}

void DudeStar::update_ref_data(Codec::MODEINFO info)
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_ref_data() called after disconnected";
		return;
	}
	if((connect_status == Codec::CONNECTING) && (info.status == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->editDMRID->setEnabled(false);
		ui->editPassword->setEnabled(false);
		ui->pushTX->setDisabled(false);
		//ui->editTG->setEnabled(false);
		ui->checkSWRX->setChecked(!(m_ref->get_hwrx()));
		if(!(m_ref->get_hwrx())){
			ui->checkSWRX->setEnabled(false);
		}
		ui->checkSWTX->setChecked(!(m_ref->get_hwtx()));
		if(!(m_ref->get_hwtx())){
			ui->checkSWTX->setEnabled(false);
		}
		process_codecgain_changed(ui->sliderCodecGain->value());
	}
	status_txt->setText(" Host: " + info.host + ":" + QString::number(info.port) + " Ping: " + QString::number(info.count));
	if(info.streamid){
		ui->data1->setText(info.src);
		ui->data2->setText(info.dst);
		ui->data3->setText(info.gw);
		ui->data4->setText(info.gw2);
		ui->data5->setText(QString::number(info.streamid, 16) + " " + QString::number(info.frame_number, 16));
		ui->data6->setText(info.usertxt);
	}
	else{
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
		ui->data6->clear();
	}
	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Codec::STREAM_NEW){
		ui->textLog->append(t + " REF RX started src: " + info.src + " dst: " + info.gw2);
	}
	if(info.stream_state == Codec::STREAM_END){
		ui->textLog->append(t + " REF RX ended src: " + info.src  + " dst: " + info.gw2);
	}
	if(info.stream_state == Codec::STREAM_LOST){
		ui->textLog->append(t + " REF RX lost src: " + info.src  + " dst: " + info.gw2);
	}
	++m_rxcnt;
}

void DudeStar::update_dcs_data(Codec::MODEINFO info)
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_dcs_data() called after disconnected";
		return;
	}
	if((connect_status == Codec::CONNECTING) && (info.status == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->comboModule->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->editDMRID->setEnabled(false);
		ui->editPassword->setEnabled(false);
		ui->pushTX->setDisabled(false);

		ui->checkSWRX->setChecked(!(m_dcs->get_hwrx()));
		if(!(m_dcs->get_hwrx())){
			ui->checkSWRX->setEnabled(false);
		}
		ui->checkSWTX->setChecked(!(m_dcs->get_hwtx()));
		if(!(m_dcs->get_hwtx())){
			ui->checkSWTX->setEnabled(false);
		}
		process_codecgain_changed(ui->sliderCodecGain->value());
	}
	status_txt->setText(" Host: " + info.host + ":" + QString::number(info.port) + " Ping: " + QString::number(info.count));
	if(info.streamid){
		ui->data1->setText(info.src);
		ui->data2->setText(info.dst);
		ui->data3->setText(info.gw);
		ui->data4->setText(info.gw2);
		ui->data5->setText(QString::number(info.streamid, 16) + " " + QString::number(info.frame_number, 16));
		ui->data6->setText(info.usertxt);
	}
	else{
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
		ui->data6->clear();
	}
	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Codec::STREAM_NEW){
		ui->textLog->append(t + " DCS RX started src: " + info.src + " dst: " + info.gw2);
	}
	if(info.stream_state == Codec::STREAM_END){
		ui->textLog->append(t + " DCS RX ended src: " + info.src  + " dst: " + info.gw2);
	}
	if(info.stream_state == Codec::STREAM_LOST){
		ui->textLog->append(t + " DCS RX lost src: " + info.src  + " dst: " + info.gw2);
	}
	if(info.netmsg.size()){
		ui->textLog->append(t + " MSG: " + info.netmsg);
	}
	++m_rxcnt;
}

void DudeStar::update_xrf_data(Codec::MODEINFO info)
{
	if(connect_status == Codec::DISCONNECTED){
		qDebug() << "update_xrf_data() called after disconnected";
		return;
	}
	if((connect_status == Codec::CONNECTING) && (info.status == Codec::CONNECTED_RW)){
		connect_status = Codec::CONNECTED_RW;
		ui->pushConnect->setText("Disconnect");
		ui->pushConnect->setEnabled(true);
		ui->comboVocoder->setEnabled(false);
		ui->comboPlayback->setEnabled(false);
		ui->comboCapture->setEnabled(false);
		ui->comboMode->setEnabled(false);
		ui->comboHost->setEnabled(false);
		ui->comboModule->setEnabled(false);
		ui->editCallsign->setEnabled(false);
		ui->editDMRID->setEnabled(false);
		ui->editPassword->setEnabled(false);
		ui->pushTX->setDisabled(false);
		ui->checkSWRX->setChecked(!(m_xrf->get_hwrx()));
		if(!(m_xrf->get_hwrx())){
			ui->checkSWRX->setEnabled(false);
		}
		ui->checkSWTX->setChecked(!(m_xrf->get_hwtx()));
		if(!(m_xrf->get_hwtx())){
			ui->checkSWTX->setEnabled(false);
		}
		process_codecgain_changed(ui->sliderCodecGain->value());
	}
	status_txt->setText(" Host: " + info.host + ":" + QString::number(info.port) + " Ping: " + QString::number(info.count));
	if(info.streamid){
		ui->data1->setText(info.src);
		ui->data2->setText(info.dst);
		ui->data3->setText(info.gw);
		ui->data4->setText(info.gw2);
		ui->data5->setText(QString::number(info.streamid, 16) + " " + QString::number(info.frame_number, 16));
		ui->data6->setText(info.usertxt);
	}
	else{
		ui->data1->clear();
		ui->data2->clear();
		ui->data3->clear();
		ui->data4->clear();
		ui->data5->clear();
		ui->data6->clear();
	}
	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Codec::STREAM_NEW){
		ui->textLog->append(t + " XRF RX started src: " + info.src + " dst: " + info.gw2);
	}
	if(info.stream_state == Codec::STREAM_END){
		ui->textLog->append(t + " XRF RX ended src: " + info.src  + " dst: " + info.gw2);
	}
	if(info.stream_state == Codec::STREAM_LOST){
		ui->textLog->append(t + " XRF RX lost src: " + info.src  + " dst: " + info.gw2);
	}
	++m_rxcnt;
}
