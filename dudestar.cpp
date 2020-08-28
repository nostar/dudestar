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

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#define ENDLINE	endl
#else
#define ENDLINE Qt::endl
#endif

#define LOBYTE(w)			((uint8_t)(uint16_t)(w & 0x00FF))
#define HIBYTE(w)			((uint8_t)((((uint16_t)(w)) >> 8) & 0xFF))
#define LOWORD(dw)			((uint16_t)(uint32_t)(dw & 0x0000FFFF))
#define HIWORD(dw)			((uint16_t)((((uint32_t)(dw)) >> 16) & 0xFFFF))
#define DEBUG
//#define DEBUGHW
//#define DEBUG_YSF
#define CHANNEL_FRAME_TX    0x1001
#define CODEC_FRAME_TX      0x1002
#define CHANNEL_FRAME_RX    0x2001
#define CODEC_FRAME_RX      0x2002

const unsigned char REC62[] = {0x62U, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x12U, 0x64U, 0x00U, 0x00U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
const unsigned char REC63[] = {0x63U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC64[] = {0x64U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC65[] = {0x65U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC66[] = {0x66U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC67[] = {0x67U, 0xF0U, 0x9DU, 0x6AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC68[] = {0x68U, 0x19U, 0xD4U, 0x26U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC69[] = {0x69U, 0xE0U, 0xEBU, 0x7BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC6A[] = {0x6AU, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
const unsigned char REC6B[] = {0x6BU, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x12U, 0x64U, 0x00U, 0x00U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
const unsigned char REC6C[] = {0x6CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC6D[] = {0x6DU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC6E[] = {0x6EU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC6F[] = {0x6FU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC70[] = {0x70U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC71[] = {0x71U, 0xACU, 0xB8U, 0xA4U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC72[] = {0x72U, 0x9BU, 0xDCU, 0x75U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
const unsigned char REC73[] = {0x73U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
const unsigned char REC80[] = {0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const uint8_t AMBEP251_4400_2800[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x05U, 0x58U, 0x08U, 0x6BU, 0x10U, 0x30U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x90U};//DVSI P25 USB Dongle FEC
//const uint8_t AMBEP251_4400_0000[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x05U, 0x58U, 0x08U, 0x6BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x58U};//DVSI P25 USB Dongle No-FEC
//const uint8_t AMBE1000_4400_2800[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x00U, 0x58U, 0x08U, 0x87U, 0x30U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x44U, 0x90U};
//const uint8_t AMBE2000_4400_2800[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x02U, 0x58U, 0x07U, 0x65U, 0x00U, 0x09U, 0x1eU, 0x0cU, 0x41U, 0x27U, 0x73U, 0x90U};
//const uint8_t AMBE3000_4400_2800[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x04U, 0x58U, 0x09U, 0x86U, 0x80U, 0x20U, 0x00U, 0x00U, 0x00U, 0x00U, 0x73U, 0x90U};
const uint8_t AMBE2000_2400_1200[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x01U, 0x30U, 0x07U, 0x63U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x48U};
const uint8_t AMBE3000_2450_1150[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x04U, 0x31U, 0x07U, 0x54U, 0x24U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x6fU, 0x48U};
const uint8_t AMBE3000_2450_0000[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x04U, 0x31U, 0x07U, 0x54U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x70U, 0x31U};

const uint8_t AMBE3000_PARITY_DISABLE[8] = {0x61, 0x00, 0x04, 0x00, 0x3f, 0x00, 0x2f, 0x14};
#ifdef USE_FLITE
extern "C" {
extern cst_voice * register_cmu_us_slt(const char *);
extern cst_voice * register_cmu_us_kal16(const char *);
extern cst_voice * register_cmu_us_awb(const char *);
extern cst_voice * register_cmu_us_rms(const char *);
}
#endif

DudeStar::DudeStar(QWidget *parent) :
    QMainWindow(parent),
	ui(new Ui::DudeStar),
	enable_swtx(false)
{
	dmrslot = 2;
	dmrcc = 1;
	dmrcalltype = 0;
	mbe = nullptr;
	ysf = nullptr;
	ping_cnt = 0;
	hwtx = false;
	hwrx = true;
	muted = false;
#ifdef USE_SWTX
	enable_swtx = true;
#endif
    ui->setupUi(this);
    init_gui();
    udp = new QUdpSocket(this);
    config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#ifndef Q_OS_WIN
	config_path += "/dudestar";
#endif
	connect(&qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(http_finished(QNetworkReply*)));
    QAudioFormat format;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
	QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
	//qDebug() << "Version == " << VERSION_NUMBER;
	if(devices.size() == 0){
		qDebug() << "No audio hardware found";
	}
	else{
		QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
#ifdef DEBUG
		QList<int> srs = info.supportedSampleRates();
		for(int i = 0; i < srs.size(); ++i){
			qDebug() << "Sample rate " << srs[i] << " supported";
		}
		QList<int> ss = info.supportedSampleSizes();
		for(int i = 0; i < ss.size(); ++i){
			qDebug() << "Sample size " << ss[i] << " supported";
		}
		QList<QAudioFormat::SampleType> st = info.supportedSampleTypes();
		for(int i = 0; i < st.size(); ++i){
			qDebug() << "Sample type " << st[i] << " supported";
		}
		QStringList sc = info.supportedCodecs();
		for(int i = 0; i < sc.size(); ++i){
			qDebug() << "Sample codec " << sc[i] << " supported";
		}
		 QList<QAudioFormat::Endian> bo = info.supportedByteOrders();
		for(int i = 0; i < bo.size(); ++i){
			qDebug() << "Sample byte order " << bo[i] << " supported";
		}
		QList<int> cc = info.supportedChannelCounts();
		for(int i = 0; i < cc.size(); ++i){
			qDebug() << "Sample channel count " << cc[i] << " supported";
		}
#endif
		if (!info.isFormatSupported(format)) {
			qWarning() << "Raw audio format not supported by backend, trying nearest format.";
			format = info.nearestFormat(format);
			qWarning() << "Format now set to " << format.sampleRate() << ":" << format.sampleSize();
		}
	}
#ifdef USE_FLITE
	flite_init();
	voice_slt = register_cmu_us_slt(nullptr);
	voice_kal = register_cmu_us_kal16(nullptr);
	voice_awb = register_cmu_us_awb(nullptr);
	voice_rms = register_cmu_us_rms(nullptr);
#endif
    audio = new QAudioOutput(format, this);
	audio->setBufferSize(8192);
	//format.setByteOrder(QAudioFormat::BigEndian);
	audioin = new QAudioInput(format, this);
    audiotimer = new QTimer();
	ping_timer = new QTimer();
	ysftimer = new QTimer();
    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    connect_status = DISCONNECTED;
	tx = false;
	txtimer = new QTimer();
	user_data.resize(21);
	connect(txtimer, SIGNAL(timeout()), this, SLOT(tx_timer()));
    connect(udp, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(audiotimer, SIGNAL(timeout()), this, SLOT(process_audio()));
	connect(ping_timer, SIGNAL(timeout()), this, SLOT(process_ping()));
	connect(ysftimer, SIGNAL(timeout()), this, SLOT(process_ysf_data()));

	//audiotimer->start(19);
	//ysftimer->start(90);
	process_dmr_ids();
	process_nxdn_ids();
	//process_settings();
	srand(time(0));
}

DudeStar::~DudeStar()
{
	QFile f(config_path + "/settings.conf");
	f.open(QIODevice::WriteOnly);
	QTextStream stream(&f);
	stream << "MODE:" << ui->modeCombo->currentText() << ENDLINE;
	stream << "REFHOST:" << saved_refhost << ENDLINE;
	stream << "DCSHOST:" << saved_dcshost << ENDLINE;
	stream << "XRFHOST:" << saved_xrfhost << ENDLINE;
	stream << "YSFHOST:" << saved_ysfhost << ENDLINE;
	stream << "DMRHOST:" << saved_dmrhost << ENDLINE;
	stream << "P25HOST:" << saved_p25host << ENDLINE;
	stream << "NXDNHOST:" << saved_nxdnhost << ENDLINE;
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
	status_txt = new QLabel("Not connected");
#ifdef USE_FLITE
	tts_voices = new QButtonGroup();
	tts_voices->addButton(ui->checkBoxTTSOff, 0);
	tts_voices->addButton(ui->checkBoxKal, 1);
	tts_voices->addButton(ui->checkBoxRms, 2);
	tts_voices->addButton(ui->checkBoxAwb, 3);
	tts_voices->addButton(ui->checkBoxSlt, 4);
#endif
#ifndef USE_FLITE
	ui->checkBoxTTSOff->hide();
	ui->checkBoxKal->hide();
	ui->checkBoxRms->hide();
	ui->checkBoxAwb->hide();
	ui->checkBoxSlt->hide();
	ui->TTSEdit->hide();
#endif
	ui->checkBoxTTSOff->setCheckState(Qt::Checked);
	ui->volumeSlider->setRange(0, 100);
	ui->volumeSlider->setValue(100);
	ui->involSlider->setRange(0, 100);
	ui->involSlider->setValue(100);
	ui->txButton->setDisabled(true);
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui->actionUpdate_DMR_IDs, SIGNAL(triggered()), this, SLOT(update_dmr_ids()));
	connect(ui->actionUpdate_host_files, SIGNAL(triggered()), this, SLOT(delete_host_files()));
    connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(process_connect()));
    connect(ui->txButton, SIGNAL(pressed()), this, SLOT(press_tx()));
    connect(ui->txButton, SIGNAL(released()), this, SLOT(release_tx()));
	connect(ui->muteButton, SIGNAL(clicked()), this, SLOT(process_mute_button()));
	connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(process_volume_changed(int)));
	connect(ui->inmuteButton, SIGNAL(clicked()), this, SLOT(process_input_mute_button()));
	connect(ui->involSlider, SIGNAL(valueChanged(int)), this, SLOT(process_input_volume_changed(int)));
	ui->statusBar->insertPermanentWidget(0, status_txt, 1);
	connect(ui->checkBoxSWRX, SIGNAL(stateChanged(int)), this, SLOT(swrx_state_changed(int)));
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
	ui->dmrccEdit->setText(QString::number(dmrcc));
	ui->dmrslotEdit->setText(QString::number(dmrslot));
	connect(ui->modeCombo, SIGNAL(currentTextChanged(const QString &)), this, SLOT(process_mode_change(const QString &)));
	connect(ui->hostCombo, SIGNAL(currentTextChanged(const QString &)), this, SLOT(process_host_change(const QString &)));

	for(char m = 0x41; m < 0x5b; ++m){
		ui->comboMod->addItem(QString(m));
	}

	ui->hostCombo->setEditable(true);
	ui->dmrtgEdit->setEnabled(false);

	if(!enable_swtx){
		ui->checkBoxSWTX->hide();
	}
}

void DudeStar::start_request(QString f)
{
	hosts_filename = f;
	qnam.get(QNetworkRequest(QUrl("http://www.dudetronics.com/ar-dns" + f)));
	status_txt->setText(tr("Downloading http://www.dudetronics.com/ar-dns" + f.toLocal8Bit()));
}

void DudeStar::http_finished(QNetworkReply *reply)
{
	if (reply->error()) {
		status_txt->setText(tr("Download failed:\n%1.").arg(reply->errorString()));
		reply->deleteLater();
		reply = nullptr;
		return;
	}
	else{
		QFile *hosts_file = new QFile(config_path + hosts_filename);
		hosts_file->open(QIODevice::WriteOnly);
		QFileInfo fileInfo(hosts_file->fileName());
		QString filename(fileInfo.fileName());
		hosts_file->write(reply->readAll());
		hosts_file->flush();
		hosts_file->close();
		delete hosts_file;
		status_txt->setText(tr("Downloaded " + filename.toLocal8Bit()));
		qDebug() << "Downloaded " << filename;
		if(filename == "dplus.txt"){
			process_ref_hosts();
		}
		else if(filename == "dextra.txt"){
			process_xrf_hosts();
		}
		else if(filename == "dcs.txt"){
			process_dcs_hosts();
		}
		else if(filename == "YSFHosts.txt"){
			process_ysf_hosts();
		}
		else if(filename == "P25Hosts.txt"){
			process_p25_hosts();
		}
		else if(filename == "DMRHosts.txt"){
			process_dmr_hosts();
		}
		else if(filename == "NXDNHosts.txt"){
			process_nxdn_hosts();
		}
		else if(filename == "DMRIDs.dat"){
			process_dmr_ids();
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
		ui->dmrtgEdit->setEnabled(false);
		ui->dmrccEdit->setEnabled(false);
		ui->dmrslotEdit->setEnabled(false);
		ui->checkBoxDMRPC->setEnabled(false);
		ui->mycallEdit->setEnabled(false);
		ui->urcallEdit->setEnabled(false);
		ui->rptr1Edit->setEnabled(false);
		ui->rptr2Edit->setEnabled(false);
		ui->usertxtEdit->setEnabled(false);
		ui->label_1->setText("Callsign");
		ui->label_2->setText("SrcID");
		ui->label_3->setText("DestID");
		ui->label_4->setText("");
		ui->label_5->setText("Seq#");
		ui->label_6->setText("");
	}
}

void DudeStar::swrx_state_changed(int s)
{
	if(s == Qt::Unchecked){
		hwrx = true;
	}
	else{
		hwrx = false;
	}
	if(ysfdec){
		//ysfdec->m_mbeDecoder->set_hwrx(hwrx);
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
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
					ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":20001");
				}
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_refhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		start_request("/dplus.txt");
	}
}

void DudeStar::process_dcs_hosts()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
					ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":30051");
				}
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_dcshost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		//QMessageBox::StandardButton reply;
		//reply = QMessageBox::question(this, "No DExtra file", "No DExtra file found, download?", QMessageBox::Yes|QMessageBox::No);
		//if (reply == QMessageBox::Yes) {
			start_request("/dcs.txt");
		//}
	}
}


void DudeStar::process_xrf_hosts()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
					ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":30001");
				}
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_xrfhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		start_request("/dextra.txt");
	}
}

void DudeStar::process_ysf_hosts()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
					ui->hostCombo->addItem(ll.at(1).simplified() + " - " + ll.at(2).simplified(), ll.at(3) + ":" + ll.at(4));
				}
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_ysfhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		start_request("/YSFHosts.txt");
	}
}

void DudeStar::process_dmr_hosts()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
						ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(2) + ":" + ll.at(4) + ":" + ll.at(3));
					}
				}
			}
		}
		f.close();
		//qDebug() << "saved_dmrhost == " << saved_dmrhost;
		int i = ui->hostCombo->findText(saved_dmrhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		start_request("/DMRHosts.txt");
	}
}

void DudeStar::process_p25_hosts()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
					ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":" + ll.at(2));
				}
			}
		}
		f.close();
		//qDebug() << "saved_p25Host == " << saved_p25host;
		int i = ui->hostCombo->findText(saved_p25host);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		start_request("/P25Hosts.txt");
	}
}

void DudeStar::process_nxdn_hosts()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
					ui->hostCombo->addItem(ll.at(0).simplified(), ll.at(1) + ":" + ll.at(2));
				}
			}
		}
		f.close();
		int i = ui->hostCombo->findText(saved_nxdnhost);
		ui->hostCombo->setCurrentIndex(i);
		ui->hostCombo->blockSignals(false);
	}
	else{
		start_request("/NXDNHosts.txt");
	}
}

void DudeStar::delete_host_files()
{
	QFileInfo check_file(config_path + "/dplus.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/dplus.txt");
		f.remove();
	}
	check_file.setFile(config_path + "/dextra.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/dextra.txt");
		f.remove();
	}
	check_file.setFile(config_path + "/dcs.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/dcs.txt");
		f.remove();
	}
	check_file.setFile(config_path + "/YSFHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/YSFHosts.txt");
		f.remove();
	}
	check_file.setFile(config_path + "/DMRHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/DMRHosts.txt");
		f.remove();
	}
	check_file.setFile(config_path + "/P25Hosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/P25Hosts.txt");
		f.remove();
	}
	check_file.setFile(config_path + "/NXDNHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/NXDNHosts.txt");
		f.remove();
	}
	process_mode_change(ui->modeCombo->currentText().simplified());
}

void DudeStar::process_dmr_ids()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
				dmrids[ll.at(0).toUInt()] = ll.at(1);
			}
		}
		f.close();
		process_settings();
	}
	else{
		start_request("/DMRIDs.dat");
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
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

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
		start_request("/NXDN.csv");
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
				if(sl.at(0) == "SWTX"){
					if(sl.at(1).simplified() == "enabled"){
						enable_swtx = true;
						ui->checkBoxSWTX->show();
					}
				}

				ui->hostCombo->blockSignals(false);
			}
		}
	}
	else{ //No settings.conf file, first time launch
		process_ref_hosts();
	}
}

void DudeStar::connect_to_serial()
{
	QString description;
	QString manufacturer;
	QString serialNumber;
	const QString blankString = "N/A";
	QTextStream out(stdout);
	hw_ambe_present = false;
	ui->checkBoxSWTX->setDisabled(true);
	ui->checkBoxSWRX->setDisabled(true);
	ui->checkBoxSWTX->setChecked(true);
	ui->checkBoxSWRX->setChecked(true);

	const auto serialPortInfos = QSerialPortInfo::availablePorts();
	if(serialPortInfos.count()){
		for(const QSerialPortInfo &serialPortInfo : serialPortInfos) {
			description = serialPortInfo.description();
			manufacturer = serialPortInfo.manufacturer();
			serialNumber = serialPortInfo.serialNumber();
			//out << "Port: " << serialPortInfo.portName() << endl << "Location: " << serialPortInfo.systemLocation() << endl << "Description: " << (!description.isEmpty() ? description : blankString) << endl << "Manufacturer: " << (!manufacturer.isEmpty() ? manufacturer : blankString) << endl << "Serial number: " << (!serialNumber.isEmpty() ? serialNumber : blankString) << endl << "Vendor Identifier: " << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : blankString) << endl << "Product Identifier: " << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : blankString) << endl << "Busy: " << (serialPortInfo.isBusy() ? "Yes" : "No") << endl;
			//if((serialPortInfo.vendorIdentifier() == 0x0483) && (serialPortInfo.productIdentifier() == 0x5740)){
			if((protocol != "P25") && (serialPortInfo.vendorIdentifier() == 0x0403) && (serialPortInfo.productIdentifier() >= 0x6001)){
				serial = new QSerialPort;
				serial->setPortName(serialPortInfo.portName());
				serial->setBaudRate(460800);
				serial->setDataBits(QSerialPort::Data8);
				serial->setStopBits(QSerialPort::OneStop);
				serial->setParity(QSerialPort::NoParity);
				//out << "Baud rate == " << serial->baudRate() << endl;
				if (serial->open(QIODevice::ReadWrite)) {
					connect(serial, &QSerialPort::readyRead, this, &DudeStar::process_serial);
					serial->setFlowControl(QSerialPort::HardwareControl);
					serial->setRequestToSend(true);
					QByteArray a;
					a.clear();
					a.append(reinterpret_cast<const char*>(AMBE3000_PARITY_DISABLE), sizeof(AMBE3000_PARITY_DISABLE));
					serial->write(a);
					QThread::msleep(100);
					a.clear();
					if(protocol == "DMR"){
						a.append(reinterpret_cast<const char*>(AMBE3000_2450_1150), sizeof(AMBE3000_2450_1150));
					}
					else if( (protocol == "YSF") || (protocol == "NXDN") ){
						a.append(reinterpret_cast<const char*>(AMBE3000_2450_0000), sizeof(AMBE3000_2450_0000));
					}
					else if(protocol == "P25"){
						a.append(reinterpret_cast<const char*>(AMBEP251_4400_2800), sizeof(AMBEP251_4400_2800));
					}
					else{ //D-Star
						a.append(reinterpret_cast<const char*>(AMBE2000_2400_1200), sizeof(AMBE2000_2400_1200));
				   }
                   serial->write(a);
				   hw_ambe_present = true;
				   ui->checkBoxSWTX->setDisabled(false);
				   ui->checkBoxSWRX->setDisabled(false);
				   ui->checkBoxSWTX->setChecked(false);
				   ui->checkBoxSWRX->setChecked(false);
				}
				else{
					hw_ambe_present = false;
					std::cerr << "Error: Failed to open device." << std::endl;
				}
			}
		}
	}
}

void DudeStar::disconnect_from_host()
{
	QByteArray d;
	if(protocol == "REF"){
		d.append(0x05);
		d.append('\x00');
		d.append(0x18);
		d.append('\x00');
		d.append('\x00');
	}
	if(protocol == "XRF"){
		d.append(callsign);
		d.append(8 - callsign.size(), ' ');
		d.append(module);
		d.append(' ');
		d.append('\x00');
	}
	if(protocol == "DCS"){
		d.append(callsign);
		d.append(8 - callsign.size(), ' ');
		d.append(module);
		d.append(' ');
		d.append('\x00');
	}
	else if(protocol == "XLX"){
		d.append('R');
		d.append('P');
		d.append('T');
		d.append('L');
		d.append((dmrid >> 24) & 0xff);
		d.append((dmrid >> 16) & 0xff);
		d.append((dmrid >> 8) & 0xff);
		d.append((dmrid >> 0) & 0xff);
	}
	else if(protocol == "YSF"){
		d.append('Y');
		d.append('S');
		d.append('F');
		d.append('U');
		d.append(callsign);
		d.append(5, ' ');
	}
	else if(protocol == "DMR"){
		d.append('R');
		d.append('P');
		d.append('T');
		d.append('C');
		d.append('L');
		d.append((dmrid >> 24) & 0xff);
		d.append((dmrid >> 16) & 0xff);
		d.append((dmrid >> 8) & 0xff);
		d.append((dmrid >> 0) & 0xff);
		ui->dmrtgEdit->setEnabled(true);
		ui->dmridEdit->setEnabled(true);
		ui->dmrpwEdit->setEnabled(true);
		dmr_header_timer->stop();
	}
	else if(protocol == "P25"){
		d.append(0xf1);
		d.append(callsign);
		ui->dmridEdit->setEnabled(true);
	}
	else if(protocol == "NXDN"){
		d.append('N');
		d.append('X');
		d.append('D');
		d.append('N');
		d.append('U');
		d.append(callsign);
		d.append(10 - callsign.size(), ' ');
		d.append((dmr_destid >> 8) & 0xff);
		d.append((dmr_destid >> 0) & 0xff);
	}
	ping_timer->stop();
	udp->writeDatagram(d, QHostAddress(host), port);
	//disconnect(udp, SIGNAL(readyRead()));
	udp->disconnect();
	udp->close();
	delete udp;
}

void DudeStar::process_connect()
{
    if(connect_status != DISCONNECTED){
        connect_status = DISCONNECTED;
        ui->connectButton->setText("Connect");
        ui->mycall->clear();
        ui->urcall->clear();
        ui->rptr1->clear();
        ui->rptr2->clear();
		ui->streamid->clear();
		ui->usertxt->clear();
		ui->modeCombo->setEnabled(true);
        ui->hostCombo->setEnabled(true);
        ui->callsignEdit->setEnabled(true);

		if((protocol == "DCS") || (protocol == "XRF")){
			ui->comboMod->setEnabled(true);
		}

		disconnect_from_host();

		if(hw_ambe_present){
			serial->close();
		}
		audiotimer->stop();
		ysftimer->stop();
		audioq.clear();
		ysfq.clear();
		ping_cnt = 0;
		ui->txButton->setDisabled(true);
		status_txt->setText("Not connected");
    }
    else{
		hostname = ui->hostCombo->currentText().simplified();
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

		if(protocol == "DMR"){
			//dmrid = dmrids.key(callsign);
			//dmr_password = sl.at(2).simplified();
			dmrid = ui->dmridEdit->text().toUInt();
			dmr_password = (ui->dmrpwEdit->text().isEmpty()) ? sl.at(2).simplified() : ui->dmrpwEdit->text();
			dmr_destid = ui->dmrtgEdit->text().toUInt();
		}
		if(protocol == "P25"){
			dmrid = dmrids.key(callsign);
			//dmr_destid = ui->hostCombo->currentText().toUInt();
			dmr_destid = ui->dmrtgEdit->text().toUInt();
		}
		if(protocol == "NXDN"){
			dmrid = nxdnids.key(callsign);
			dmr_destid = ui->hostCombo->currentText().toUInt();
		}
		connect_to_serial();
		QHostInfo::lookupHost(host, this, SLOT(hostname_lookup(QHostInfo)));
        audiodev = audio->start();
    }
}

void DudeStar::hostname_lookup(QHostInfo i)
{
	QByteArray d;
	if(protocol == "REF"){
		d.append(0x05);
		d.append('\x00');
		d.append(0x18);
		d.append('\x00');
		d.append(0x01);
	}
	if(protocol == "XRF"){
		d.append(callsign);
		d.append(8 - callsign.size(), ' ');
		d.append(module);
		d.append(module);
		d.append(11);
	}
	if(protocol == "DCS"){
		d.append(callsign);
		d.append(8 - callsign.size(), ' ');
		d.append(module);
		d.append(module);
		d.append(11);
		d.append(508, 0);
	}
	else if(protocol == "YSF"){
		d.append('Y');
		d.append('S');
		d.append('F');
		d.append('P');
		d.append(callsign);
		d.append(5, ' ');
	}
	else if(protocol == "DMR"){
		d.append('R');
		d.append('P');
		d.append('T');
		d.append('L');
		d.append((dmrid >> 24) & 0xff);
		d.append((dmrid >> 16) & 0xff);
		d.append((dmrid >> 8) & 0xff);
		d.append((dmrid >> 0) & 0xff);
	}
	else if(protocol == "P25"){
		d.append(0xf0);
		d.append(callsign);
		d.append(10 - callsign.size(), ' ');
	}
	else if(protocol == "NXDN"){
		d.append('N');
		d.append('X');
		d.append('D');
		d.append('N');
		d.append('P');
		d.append(callsign);
		d.append(10 - callsign.size(), ' ');
		d.append((dmr_destid >> 8) & 0xff);
		d.append((dmr_destid >> 0) & 0xff);
	}
	if (!i.addresses().isEmpty()) {
		address = i.addresses().first();
		udp = new QUdpSocket(this);
		connect(udp, SIGNAL(readyRead()), this, SLOT(readyRead()));
		udp->writeDatagram(d, address, port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < d.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)d.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	}
}

void DudeStar::process_volume_changed(int v)
{
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(!muted){
		audio->setVolume(linear_vol);
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
		audio->setVolume(linear_vol);
	}
	else{
		muted = true;
		ui->muteButton->setText("Unmute");
		audio->setVolume(0.0);
	}
}

void DudeStar::process_input_volume_changed(int v)
{
	qreal linear_vol = QAudio::convertVolume(v / qreal(100.0),QAudio::LogarithmicVolumeScale,QAudio::LinearVolumeScale);
	if(!input_muted){
		audioin->setVolume(linear_vol);
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
		audioin->setVolume(linear_vol);
	}
	else{
		input_muted = true;
		ui->inmuteButton->setText("Unmute");
		audioin->setVolume(0.0);
	}
}

void DudeStar::process_audio()
{
	int nbAudioSamples = 0;
	short *audioSamples;
	unsigned char d[11];
	char ch_pkt_hdr[6] = {0x61, 0x00, 0x0b, 0x01, 0x01, 0x48};
	QByteArray ambe;
	QByteArray audio;
	//qDebug() << audioq.size() << ":" << hw_ambe_audio.size() << ":" << hwrx;
	if( (protocol != "P25") && (audioq.size() < 9) ){
		//return;
	}
	else if( (protocol == "P25") && (audioq.size() < 11) ){
		//return;
	}
	if(connect_status != CONNECTED_RW){
		return;
	}

	if( (protocol == "P25") && (audioq.size() >= 11) ){
		for(int i = 0; i < 11; ++i){
			d[i] = audioq.dequeue();
		}
		mbe->process_p25(d);
	}
	else if( (protocol == "DMR") && (audioq.size() >= 9) ){
		for(int i = 0; i < 9; ++i){
			d[i] = audioq.dequeue();
		}
		if(hwrx){
			ambe.append(ch_pkt_hdr, 6);
			ambe.append(reinterpret_cast<char *>(d), 9);
			serial->write(ambe);
		}
		else{
			mbe->process_dmr(d);
		}
	}
	else if( (protocol == "NXDN") && (audioq.size() >= 7) ){
		mbe->set_hwrx(hwrx);
		for(int i = 0; i < 7; ++i){
			d[i] = audioq.dequeue();
		}
		mbe->process_nxdn(d);
		if(hwrx){
			ch_pkt_hdr[2] = 0x09;
			ch_pkt_hdr[5] = 0x31;
			ambe.append(ch_pkt_hdr, 6);
			ambe.append(reinterpret_cast<char *>(d), 7);
			serial->write(ambe);
		}
	}
	else if( (protocol == "YSF") && (audioq.size() >= 9) ){
		if(!hwrx){
			return;
		}
		for(int i = 0; i < 9; ++i){
			d[i] = audioq.dequeue();
		}
		ch_pkt_hdr[2] = 0x09;
		ch_pkt_hdr[5] = 0x31;
		ambe.append(ch_pkt_hdr, 6);
		ambe.append(reinterpret_cast<char *>(d), 7);
/*
		fprintf(stderr, "YSFAMBE: ");
		for(int i = 0; i < ambe.size(); ++i){
			fprintf(stderr, "%02x ", (unsigned char)ambe.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
*/
		serial->write(ambe);
	}
	else if(audioq.size() >= 9){
		for(int i = 0; i < 9; ++i){
			d[i] = audioq.dequeue();
		}
		if(hwrx){
			ambe.append(ch_pkt_hdr, 6);
			ambe.append(reinterpret_cast<char *>(d), 9);
			serial->write(ambe);
		}
		else{
			mbe->process_dstar(d);
		}
	}
	if(hwrx){
		if(hw_ambe_audio.size() < 326){
			//qDebug() << "Still collecting audio from AMBE size == " << hw_ambe_audio.size();
			return;
		}

		//qDebug() << "start proccessing hw audio : " << hw_ambe_audio.size() << ":" << hw_ambe_audio.at(0) << ":" << hw_ambe_audio.at(3);
		if((hw_ambe_audio.at(0) == 0x61) && (hw_ambe_audio.at(3) == 0x02) ){
			//qDebug() << "Received audio packet";
			for(int i = 0; i < 6; ++i){
				hw_ambe_audio.dequeue();
			}
			for(int i = 0; i < 320; i+=2){
				//audio.append(hw_ambe_audio.dequeue());
				//Byte swap BE to LE
				char t = hw_ambe_audio.dequeue();
				audio.append(hw_ambe_audio.dequeue());
				audio.append(t);
			}
			audiodev->write(audio);
		}
		else{
			//qDebug() << "WTF!";
			do{
				if(hw_ambe_audio.size()){
					hw_ambe_audio.dequeue();
					//qDebug() << "Lost HW AMBE audio sync";
				}
			}
			while(hw_ambe_audio.size() && (hw_ambe_audio.at(0) != 0x61));
		}
		//audiodev->write(audio);
	}
	else if (mbe && !hwrx){
		audioSamples = mbe->getAudio(nbAudioSamples);
		audiodev->write((const char *) audioSamples, sizeof(short) * nbAudioSamples);
		mbe->resetAudio();
	}
}

void DudeStar::AppendVoiceLCToBuffer(QByteArray& buffer, uint32_t uiSrcId, uint32_t uiDstId) const
{
	//uint8_t g_DmrSyncBSData[]     = { 0x0D,0xFF,0x57,0xD7,0x5D,0xF5,0xD0 };
	uint8_t g_DmrSyncMSData[]     = { 0x0D,0x5D,0x7F,0x77,0xFD,0x75,0x70 };
	uint8_t payload[33];

	// fill payload
	CBPTC19696 bptc;
	::memset(payload, 0, sizeof(payload));
	// LC data
	uint8_t lc[12];
	{
		::memset(lc, 0, sizeof(lc));
		lc[3] = (uint8_t)LOBYTE(HIWORD(uiDstId));
		lc[4] = (uint8_t)HIBYTE(LOWORD(uiDstId));
		lc[5] = (uint8_t)LOBYTE(LOWORD(uiDstId));
		// uiSrcId
		lc[6] = (uint8_t)LOBYTE(HIWORD(uiSrcId));
		lc[7] = (uint8_t)HIBYTE(LOWORD(uiSrcId));
		lc[8] = (uint8_t)LOBYTE(LOWORD(uiSrcId));
		// parity
		uint8_t parity[4];
		CRS129::encode(lc, 9, parity);
		lc[9]  = parity[2] ^ 0x96;
		lc[10] = parity[1] ^ 0x96;
		lc[11] = parity[0] ^ 0x96;
	}
	// sync
	::memcpy(payload+13, g_DmrSyncMSData, sizeof(g_DmrSyncMSData));
	// slot type
	{
		// slot type
		uint8_t slottype[3];
		::memset(slottype, 0, sizeof(slottype));
		slottype[0]  = (1 << 4) & 0xF0;
		slottype[0] |= (1  << 0) & 0x0FU;
		CGolay2087::encode(slottype);
		payload[12U] = (payload[12U] & 0xC0U) | ((slottype[0U] >> 2) & 0x3FU);
		payload[13U] = (payload[13U] & 0x0FU) | ((slottype[0U] << 6) & 0xC0U) | ((slottype[1U] >> 2) & 0x30U);
		payload[19U] = (payload[19U] & 0xF0U) | ((slottype[1U] >> 2) & 0x0FU);
		payload[20U] = (payload[20U] & 0x03U) | ((slottype[1U] << 6) & 0xC0U) | ((slottype[2U] >> 2) & 0x3CU);

	}
	// and encode
	bptc.encode(lc, payload);

	// and append
	buffer.append((char *)payload, sizeof(payload));
}

void DudeStar::tx_dmr_header()
{
	QByteArray out;

	out.append("DMRD", 4);
	out.append('\0');
	out.append((dmrid >> 16) & 0xff);
	out.append((dmrid >> 8) & 0xff);
	out.append((dmrid >> 0) & 0xff);
	out.append((dmr_destid >> 16) & 0xff);
	out.append((dmr_destid >> 8) & 0xff);
	out.append((dmr_destid >> 0) & 0xff);
	out.append((dmrid >> 24) & 0xff);
	out.append((dmrid >> 16) & 0xff);
	out.append((dmrid >> 8) & 0xff);
	out.append((dmrid >> 0) & 0xff);
	out.append(0xa1); // 0xa1 = S2, 0x21 = S1
	out.append(0x0e);
	out.append('\x00');
	out.append('\x00');
	out.append('\x00');
	AppendVoiceLCToBuffer(out, dmrid, dmr_destid);
	out.append(2, 0);

	udp->writeDatagram(out, address, port);

	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
}

void DudeStar::process_ysf_data()
{
	int nbAudioSamples = 0;
	short *audioSamples;
	unsigned char d[115];
	if(ysfq.size() < 115){
		//std::cerr << "process_ysf_data() no data" << std::endl;
		return;
	}
	for(int i = 0; i < 115; ++i){
		d[i] = ysfq.dequeue();
	}
	DSDYSF::FICH f = ysfdec->process_ysf(d);
	//std::cerr << "process_ysf_data() f: " << f << std::endl;

	if(f.getDataType() == 0){
		ui->rptr2->setText("V/D mode 1");
	}
	else if(f.getDataType() == 1){
		ui->rptr2->setText("Data Full Rate");
	}
	else if(f.getDataType() == 2){
		ui->rptr2->setText("V/D mode 2");
	}
	else if(f.getDataType() == 3){
		ui->rptr2->setText("Voice Full Rate");
	}

	ui->streamid->setText(f.isInternetPath() ? "Internet" : "Local");
	ui->usertxt->setText(QString::number(f.getFrameNumber()) + "/" + QString::number(f.getFrameTotal()));
	ysfdec->m_mbeDecoder->set_hwrx(hwrx);
	if(hwrx){
		//qDebug() << "mbe->ambe72.size() == " << mbe->ambe72.size();
		while(ysfdec->m_mbeDecoder->ambe72.size()){
			audioq.enqueue(ysfdec->m_mbeDecoder->ambe72.front());
			ysfdec->m_mbeDecoder->ambe72.pop();
		}
	}
	else{
		audioSamples = ysfdec->getAudio(nbAudioSamples);
		audiodev->write((const char *) audioSamples, sizeof(short) * nbAudioSamples);
		ysfdec->resetAudio();
	}
}

void DudeStar::readyRead()
{
	if(protocol == "REF"){
		readyReadREF();
	}
	else if (protocol == "XRF"){
		readyReadXRF();
	}
	else if (protocol == "DCS"){
		readyReadDCS();
	}
	else if (protocol == "YSF"){
		readyReadYSF();
	}
	else if (protocol == "DMR"){
		readyReadDMR();
	}
	else if (protocol == "P25"){
		readyReadP25();
	}
	else if (protocol == "NXDN"){
		readyReadNXDN();
	}
}

void DudeStar::process_ping()
{
	QByteArray out;
	out.clear();
	if(protocol == "REF"){
		out.append(0x03);
		out.append(0x60);
		out.append('\x00');
	}
	else if(protocol == "XRF"){
		out.append(callsign);
		out.append(8 - callsign.size(), ' ');
		out.append('\x00');
	}
	else if(protocol == "DCS"){
		out.append(callsign);
		out.append(7 - callsign.size(), ' ');
		out.append(module);
		out.append('\x00');
		out.append(hostname);
		out.append('\x00');
		out.append(module);
		out.append(module);
		out.append(0x0a);
		out.append('\x00');
		out.append(0x20);
		out.append(0x20);
	}
	else if(protocol == "YSF"){
		out.append('Y');
		out.append('S');
		out.append('F');
		out.append('P');
		out.append(callsign);
		out.append(5, ' ');
	}
	else if(protocol == "DMR"){
		char tag[] = { 'R','P','T','P','I','N','G' };
		out.append(tag, 7);
		out.append((dmrid >> 24) & 0xff);
		out.append((dmrid >> 16) & 0xff);
		out.append((dmrid >> 8) & 0xff);
		out.append((dmrid >> 0) & 0xff);
	}
	else if(protocol == "P25"){
		out.append(0xf0);
		out.append(callsign);
		out.append(10 - callsign.size(), ' ');
	}
	else if(protocol == "NXDN"){
		out.append('N');
		out.append('X');
		out.append('D');
		out.append('N');
		out.append('P');
		out.append(callsign);
		out.append(10 - callsign.size(), ' ');
		out.append((dmr_destid >> 8) & 0xff);
		out.append((dmr_destid >> 0) & 0xff);
	}
	udp->writeDatagram(out, address, port);
#ifdef DEBUG
	fprintf(stderr, "PING: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void DudeStar::readyReadYSF()
{
	QByteArray buf;
	QByteArray out;
	QHostAddress sender;
	quint16 senderPort;
	char ysftag[11], ysfsrc[11], ysfdst[11];
	buf.resize(udp->pendingDatagramSize());
	udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
#ifdef DEBUG_YSF
	fprintf(stderr, "RECV: ");
	for(int i = 0; i < buf.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)buf.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	if(buf.size() == 14){
		if(connect_status == CONNECTING){
			ysf = new YSFEncoder();
			ysf->set_callsign(callsign.toStdString().c_str());
			ysfdec = new DSDYSF();
			mbeenc = new MBEEncoder();
			mbeenc->set_49bit_mode();
			//mbeenc->set_gain_adjust(2.5);
			mbeenc->set_gain_adjust(1.0);
			ui->connectButton->setText("Disconnect");
			ui->connectButton->setEnabled(true);
			ui->modeCombo->setEnabled(false);
			ui->hostCombo->setEnabled(false);
			ui->callsignEdit->setEnabled(false);
			ui->comboMod->setEnabled(false);
			connect_status = CONNECTED_RW;
			ysftimer->start(90);
			audiotimer->start(19);
			ping_timer->start(5000);
			if(hw_ambe_present || enable_swtx){
				ui->txButton->setDisabled(false);
			}
		}
		status_txt->setText(" Host: " + host + ":" + QString::number(port) + " Ping: " + QString::number(ping_cnt++));
	}
	if((buf.size() == 155) && (::memcmp(buf.data(), "YSFD", 4U) == 0)){
		memcpy(ysftag, buf.data() + 4, 10);ysftag[10] = '\0';
		memcpy(ysfsrc, buf.data() + 14, 10);ysfsrc[10] = '\0';
		memcpy(ysfdst, buf.data() + 24, 10);ysfdst[10] = '\0';
		ui->mycall->setText(QString(ysftag));
		ui->urcall->setText(QString(ysfsrc));
		ui->rptr1->setText(QString(ysfdst));
		for(int i = 0; i < 115; ++i){
			ysfq.enqueue(buf.data()[40+i]);
		}
	}
}

void DudeStar::readyReadNXDN()
{
	QByteArray buf;
	QHostAddress sender;
	quint16 senderPort;
	static uint8_t cnt = 0;

	buf.resize(udp->pendingDatagramSize());
	udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
#ifdef DEBUG
	fprintf(stderr, "RECV: ");
	for(int i = 0; i < buf.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)buf.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	if(buf.size() == 17){
		if(connect_status == CONNECTING){
			nxdn = new NXDNEncoder();
			mbe = new MBEDecoder();
			mbe->setAutoGain(true);
			mbeenc = new MBEEncoder();
			mbeenc->set_49bit_mode();
			ui->connectButton->setText("Disconnect");
			ui->connectButton->setEnabled(true);
			ui->modeCombo->setEnabled(false);
			ui->hostCombo->setEnabled(false);
			ui->callsignEdit->setEnabled(false);
			ui->comboMod->setEnabled(false);
			connect_status = CONNECTED_RW;
			nxdn->set_srcid(dmrid);
			nxdn->set_dstid(dmr_destid);
			if(hw_ambe_present || enable_swtx){
				ui->txButton->setDisabled(false);
			}
			audiotimer->start(19);
			ping_timer->start(1000);
		}
		status_txt->setText(" Host: " + host + ":" + QString::number(port) + " Ping: " + QString::number(ping_cnt++));
	}
	if(buf.size() == 43){
		uint16_t id = (uint16_t)((buf.data()[5] << 8) & 0xff00) | (buf.data()[6] & 0xff);
		ui->mycall->setText(nxdnids[id]);
		ui->urcall->setText(QString::number(id));
		ui->rptr1->setText(QString::number((uint16_t)((buf.data()[7] << 8) & 0xff00) | (buf.data()[8] & 0xff)));
		ui->streamid->setText(QString::number((cnt++ % 16), 16));
		for(int i = 0; i < 7; ++i){
			audioq.enqueue(buf.data()[i+15]);
		}
		char t[7];
		char *d = &(buf.data()[21]);
		for(int i = 0; i < 6; ++i){
			t[i] = d[i] << 1;
			t[i] |= (1 & (d[i+1] >> 7));
		}
		t[6] = d[6] << 1;
		for(int i = 0; i < 7; ++i){
			audioq.enqueue(t[i]);
		}
		for(int i = 0; i < 7; ++i){
			audioq.enqueue(buf.data()[i+29]);
		}
		d = &(buf.data()[35]);
		for(int i = 0; i < 6; ++i){
			t[i] = d[i] << 1;
			t[i] |= (1 & (d[i+1] >> 7));
		}
		t[6] = d[6] << 1;
		for(int i = 0; i < 7; ++i){
			audioq.enqueue(t[i]);
		}
	}
}

void DudeStar::readyReadP25()
{
	QByteArray buf;
	QHostAddress sender;
	quint16 senderPort;

	buf.resize(udp->pendingDatagramSize());
	udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
#ifdef DEBUG
	fprintf(stderr, "RECV: ");
	for(int i = 0; i < buf.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)buf.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	if(buf.size() == 11){
		if(connect_status == CONNECTING){
			mbe = new MBEDecoder();
			mbe->setAutoGain(true);
			p25 = new p25encoder();
			mbeenc = new MBEEncoder();
			mbeenc->set_88bit_mode();
			ui->connectButton->setText("Disconnect");
			ui->connectButton->setEnabled(true);
			ui->modeCombo->setEnabled(false);
			ui->hostCombo->setEnabled(false);
			ui->callsignEdit->setEnabled(false);
			ui->dmridEdit->setEnabled(false);
			ui->comboMod->setEnabled(false);
			connect_status = CONNECTED_RW;
			audiotimer->start(19);
			ping_timer->start(5000);
			ui->txButton->setDisabled(false);
		}
		status_txt->setText(" Host: " + host + ":" + QString::number(port) + " Ping: " + QString::number(ping_cnt++));
	}
	if(buf.size() > 11){
		int offset = 0;
		uint32_t id = 0;
		ui->streamid->setText(QString::number(buf.data()[0] & 0xff, 16));
		switch (buf.data()[0U]) {
		case 0x62U:
			offset = 10U;
			break;
		case 0x63U:
			offset = 1U;
			break;
		case 0x64U:
			offset = 5U;
			break;
		case 0x65U:
			ui->rptr1->setText(QString::number((uint32_t)((buf.data()[1] << 16) | ((buf.data()[2] << 8) & 0xff00) | (buf.data()[3] & 0xff))));
			offset = 5U;
			break;
		case 0x66U:
			id = (uint32_t)((buf.data()[1] << 16) | ((buf.data()[2] << 8) & 0xff00) | (buf.data()[3] & 0xff));
			ui->mycall->setText(dmrids[id]);
			ui->urcall->setText(QString::number(id));
			//ui->rptr1->setText(QString::number((uint32_t)((buf.data()[1] << 16) | ((buf.data()[2] << 8) & 0xff00) | (buf.data()[3] & 0xff))));
			offset = 5U;
			break;
		case 0x67U:
		case 0x68U:
		case 0x69U:
			offset = 5U;
			break;
		case 0x6AU:
			offset = 4U;
			break;
		case 0x6BU:
			offset = 10U;
			break;
		case 0x6CU:
			offset = 1U;
			break;
		case 0x6DU:
		case 0x6EU:
		case 0x6FU:
		case 0x70U:
		case 0x71U:
		case 0x72U:
			offset = 5U;
			break;
		case 0x73U:
			offset = 4U;
			break;
		default:
			break;
		}
		for(int i = 0; i < 11; ++i){
			audioq.enqueue(buf.data()[i + offset]);
		}
	}
}

void DudeStar::readyReadDMR()
{
	QByteArray buf;
	QByteArray in;
	QByteArray out;
	QHostAddress sender;
	quint16 senderPort;
	CSHA256 sha256;
	char buffer[400U];

	buf.resize(udp->pendingDatagramSize());
	udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
#ifdef DEBUG
	fprintf(stderr, "RECV: ");
	for(int i = 0; i < buf.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)buf.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	if((connect_status != CONNECTED_RW) && (::memcmp(buf.data() + 3, "NAK", 3U) == 0)){
		if(hw_ambe_present){
			serial->close();
		}
		udp->disconnect();
		udp->close();
		delete udp;
		connect_status = DISCONNECTED;
		ui->connectButton->setText("Connect");
		ui->modeCombo->setEnabled(true);
		ui->hostCombo->setEnabled(true);
		ui->callsignEdit->setEnabled(true);
		ui->dmridEdit->setEnabled(true);
		ui->dmrpwEdit->setEnabled(true);
		status_txt->setText("Connection refused");
	}
	if((connect_status != CONNECTED_RW) && (::memcmp(buf.data(), "RPTACK", 6U) == 0)){
		switch(connect_status){
		case CONNECTING:
			connect_status = DMR_AUTH;
			in.append(buf[6]);
			in.append(buf[7]);
			in.append(buf[8]);
			in.append(buf[9]);
			in.append(dmr_password);

			out.clear();
			out.resize(40);
			out[0] = 'R';
			out[1] = 'P';
			out[2] = 'T';
			out[3] = 'K';
			out[4] = (dmrid >> 24) & 0xff;
			out[5] = (dmrid >> 16) & 0xff;
			out[6] = (dmrid >> 8) & 0xff;
			out[7] = (dmrid >> 0) & 0xff;

			sha256.buffer((unsigned char *)in.data(), (unsigned int)(dmr_password.size() + sizeof(uint32_t)), (unsigned char *)out.data() + 8U);
			break;
		case DMR_AUTH:
			out.clear();
			buffer[0] = 'R';
			buffer[1] = 'P';
			buffer[2] = 'T';
			buffer[3] = 'C';
			buffer[4] = (dmrid >> 24) & 0xff;
			buffer[5] = (dmrid >> 16) & 0xff;
			buffer[6] = (dmrid >> 8) & 0xff;
			buffer[7] = (dmrid >> 0) & 0xff;

			connect_status = DMR_CONF;
			char latitude[20U];
			::sprintf(latitude, "50.00000");

			char longitude[20U];
			::sprintf(longitude, "03.000000");
			::sprintf(buffer + 8U, "%-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s", callsign.toStdString().c_str(),
					438800000, 438800000, 1, 1, latitude, longitude, 0, "Detroit","USA", '2', "www.dudetronics.com", "20190131", "MMDVM");
			out.append(buffer, 302);
			break;
		case DMR_CONF:
			connect_status = CONNECTED_RW;
			dmr = new DMREncoder();
			dmr->set_srcid(dmrid);
			dmr->set_dstid(dmr_destid);
			mbe = new MBEDecoder();
			mbe->setAutoGain(false);
			mbeenc = new MBEEncoder();
			mbeenc->set_49bit_mode();
			mbeenc->set_gain_adjust(2.5);
			dmr_header_timer = new QTimer();
			connect(dmr_header_timer, SIGNAL(timeout()), this, SLOT(tx_dmr_header()));
			ui->connectButton->setText("Disconnect");
			ui->connectButton->setEnabled(true);
			ui->modeCombo->setEnabled(false);
			ui->hostCombo->setEnabled(false);
			ui->callsignEdit->setEnabled(false);
			ui->dmridEdit->setEnabled(false);
			ui->dmrpwEdit->setEnabled(false);
			//ui->dmrtgEdit->setEnabled(false);
			audiotimer->start(19);
			ping_timer->start(5000);
			if(hw_ambe_present || enable_swtx){
				ui->txButton->setDisabled(false);
			}
			else{
				tx_dmr_header();
				dmr_header_timer->start(300000);
			}
			status_txt->setText(" Host: " + host + ":" + QString::number(port) + " Ping: " + QString::number(ping_cnt));
			break;
		default:
			break;
		}
		udp->writeDatagram(out, address, port);
	}
	if((buf.size() == 11) && (::memcmp(buf.data(), "MSTPONG", 7U) == 0)){
		status_txt->setText(" Host: " + host + ":" + QString::number(port) + " Ping: " + QString::number(ping_cnt++));
	}
	if((buf.size() == 55) && (::memcmp(buf.data(), "DMRD", 4U) == 0) && !((uint8_t)buf.data()[15] & 0x20)){
		uint8_t dmrframe[33];
		uint8_t dmr3ambe[27];
		uint8_t dmrsync[7];
		// get the 33 bytes ambe
		memcpy(dmrframe, &(buf.data()[20]), 33);
		// extract the 3 ambe frames
		memcpy(dmr3ambe, dmrframe, 14);
		dmr3ambe[13] &= 0xF0;
		dmr3ambe[13] |= (dmrframe[19] & 0x0F);
		memcpy(&dmr3ambe[14], &dmrframe[20], 14);
		// extract sync
		dmrsync[0] = dmrframe[13] & 0x0F;
		::memcpy(&dmrsync[1], &dmrframe[14], 5);
		dmrsync[6] = dmrframe[19] & 0xF0;
		for(int i = 0; i < 27; ++i){
			audioq.enqueue(dmr3ambe[i]);
		}
		uint32_t id = (uint32_t)((buf.data()[5] << 16) | ((buf.data()[6] << 8) & 0xff00) | (buf.data()[7] & 0xff));
		ui->mycall->setText(dmrids[id]);
		ui->urcall->setText(QString::number(id));
		ui->rptr1->setText(QString::number((uint32_t)((buf.data()[8] << 16) | ((buf.data()[9] << 8) & 0xff00) | (buf.data()[10] & 0xff))));
		ui->rptr2->setText(QString::number((uint32_t)((buf.data()[11] << 24) | ((buf.data()[12] << 16) & 0xff0000) | ((buf.data()[13] << 8) & 0xff00) | (buf.data()[14] & 0xff))));
		ui->streamid->setText(QString::number(buf.data()[4] & 0xff, 16));
	}
#ifdef DEBUG
	if(out.size() > 0){
		fprintf(stderr, "SEND: ");
		for(int i = 0; i < out.size(); ++i){
			fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
	}
#endif
}

void DudeStar::readyReadXRF()
{
	QByteArray buf;
	QByteArray out;
	QHostAddress sender;
	quint16 senderPort;
	static bool sd_sync = 0;
	static int sd_seq = 0;
	char mycall[9], urcall[9], rptr1[9], rptr2[9];
	static unsigned short streamid = 0, s = 0;

	buf.resize(udp->pendingDatagramSize());
	udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
#ifdef DEBUG
	fprintf(stderr, "RECV: ");
	for(int i = 0; i < buf.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)buf.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	if ((buf.size() == 14) && (!memcmp(buf.data()+10, "ACK", 3))){
		mbe = new MBEDecoder();
		mbe->setAutoGain(true);
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->comboMod->setEnabled(false);
		connect_status = CONNECTED_RW;
		audiotimer->start(19);
		ping_timer->start(3000);
		memset(rptr2, ' ', 8);
		memcpy(rptr2, hostname.toLocal8Bit(), hostname.size());
		rptr2[7] = module;
		rptr2[8] = 0;
		ui->rptr2Edit->setText(rptr2);
		if(hw_ambe_present || enable_swtx){
			ui->txButton->setDisabled(false);
		}
		status_txt->setText("RW connect to " + host + ":" + QString::number(port));
	}
	if(buf.size() == 9){
		status_txt->setText(" Host: " + host + ":" + QString::number(port) + " Ping: " + QString::number(ping_cnt++));
	}
	if((buf.size() == 56) && (!memcmp(buf.data(), "DSVT", 4))) {
		streamid = (buf.data()[12] << 8) | (buf.data()[13] & 0xff);
		memcpy(rptr2, buf.data() + 18, 8); rptr1[8] = '\0';
		memcpy(rptr1, buf.data() + 26, 8); rptr2[8] = '\0';
		memcpy(urcall, buf.data() + 34, 8); urcall[8] = '\0';
		memcpy(mycall, buf.data() + 42, 8); mycall[8] = '\0';
		ui->mycall->setText(QString(mycall));
		ui->urcall->setText(QString(urcall));
		ui->rptr1->setText(QString(rptr1));
		ui->rptr2->setText(QString(rptr2));
		ui->streamid->setText(QString::number(streamid, 16));
	}
	if((buf.size() == 27) && (!memcmp(buf.data(), "DSVT", 4))) {
		s = (buf.data()[12] << 8) | (buf.data()[13] & 0xff);
		if(s != streamid){
			//return;
		}
		QString ss = QString("%1").arg(s, 4, 16, QChar('0'));
		QString n = QString("%1").arg(buf.data()[14], 2, 10, QChar('0'));
		ui->streamid->setText(ss + " " + n);
		if((buf.data()[14] == 0) && (buf.data()[24] == 0x55) && (buf.data()[25] == 0x2d) && (buf.data()[26] == 0x16)){
			sd_sync = 1;
			sd_seq = 1;
		}
		if(sd_sync && (sd_seq == 1) && (buf.data()[14] == 1) && (buf.data()[24] == 0x30)){
			user_data[0] = buf.data()[25] ^ 0x4f;
			user_data[1] = buf.data()[26] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 2) && (buf.data()[14] == 2)){
			user_data[2] = buf.data()[24] ^ 0x70;
			user_data[3] = buf.data()[25] ^ 0x4f;
			user_data[4] = buf.data()[26] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 3) && (buf.data()[14] == 3) && (buf.data()[24] == 0x31)){
			user_data[5] = buf.data()[25] ^ 0x4f;
			user_data[6] = buf.data()[26] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 4) && (buf.data()[14] == 4)){
			user_data[7] = buf.data()[24] ^ 0x70;
			user_data[8] = buf.data()[25] ^ 0x4f;
			user_data[9] = buf.data()[26] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 5) && (buf.data()[14] == 5) && (buf.data()[24] == 0x32)){
			user_data[10] = buf.data()[25] ^ 0x4f;
			user_data[11] = buf.data()[26] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 6) && (buf.data()[14] == 6)){
			user_data[12] = buf.data()[24] ^ 0x70;
			user_data[13] = buf.data()[25] ^ 0x4f;
			user_data[14] = buf.data()[26] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 7) && (buf.data()[14] == 7) && (buf.data()[24] == 0x33)){
			user_data[15] = buf.data()[25] ^ 0x4f;
			user_data[16] = buf.data()[26] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 8) && (buf.data()[14] == 8)){
			user_data[17] = buf.data()[24] ^ 0x70;
			user_data[18] = buf.data()[25] ^ 0x4f;
			user_data[19] = buf.data()[26] ^ 0x93;
			user_data[20] = '\0';
			sd_sync = 0;
			sd_seq = 0;
			ui->usertxt->setText(QString::fromUtf8(user_data.data()));
		}

		for(int i = 0; i < 9; ++i){
			audioq.enqueue(buf.data()[15+i]);
		}
	}
}

void DudeStar::readyReadDCS()
{
	QByteArray buf;
	QByteArray out;
	QHostAddress sender;
	quint16 senderPort;
	static bool sd_sync = 0;
	static int sd_seq = 0;
	char mycall[9], urcall[9], rptr1[9], rptr2[9];
	static unsigned short streamid = 0;

	buf.resize(udp->pendingDatagramSize());
	udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
#ifdef DEBUG
	fprintf(stderr, "RECV: ");
	for(int i = 0; i < buf.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)buf.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	if ((buf.size() == 14) && (!memcmp(buf.data()+10, "ACK", 3))){
		mbe = new MBEDecoder();
		mbe->setAutoGain(true);
		ui->connectButton->setText("Disconnect");
		ui->connectButton->setEnabled(true);
		ui->modeCombo->setEnabled(false);
		ui->hostCombo->setEnabled(false);
		ui->callsignEdit->setEnabled(false);
		ui->comboMod->setEnabled(false);
		connect_status = CONNECTED_RW;
		audiotimer->start(19);
		ping_timer->start(1000);
		memset(rptr2, ' ', 8);
		memcpy(rptr2, hostname.toLocal8Bit(), hostname.size());
		rptr2[7] = module;
		rptr2[8] = 0;
		ui->rptr2Edit->setText(rptr2);
		if(hw_ambe_present || enable_swtx){
			ui->txButton->setDisabled(false);
		}
		status_txt->setText("RW connect to " + host + ":" +  QString::number(port));
	}
	if(buf.size() == 22){
		status_txt->setText("Host: " + host + ":" + QString::number(port) + " Ping: " + QString::number(ping_cnt++));
	}
	if((buf.size() >= 100) && (!memcmp(buf.data(), "0001", 4))) {
		streamid = (buf.data()[43] << 8) | (buf.data()[44] & 0xff);
		memcpy(rptr2, buf.data() + 7, 8); rptr1[8] = '\0';
		memcpy(rptr1, buf.data() + 15, 8); rptr2[8] = '\0';
		memcpy(urcall, buf.data() + 23, 8); urcall[8] = '\0';
		memcpy(mycall, buf.data() + 31, 8); mycall[8] = '\0';
		ui->mycall->setText(QString(mycall));
		ui->urcall->setText(QString(urcall));
		ui->rptr1->setText(QString(rptr1));
		ui->rptr2->setText(QString(rptr2));
		//ui->streamid->setText(QString::number(streamid, 16));
		QString ss = QString("%1").arg(streamid, 4, 16, QChar('0'));
		QString n = QString("%1").arg(buf.data()[0x2d], 2, 10, QChar('0'));
		ui->streamid->setText(ss + " " + n);
		if((buf.data()[45] == 0) && (buf.data()[55] == 0x55) && (buf.data()[56] == 0x2d) && (buf.data()[57] == 0x16)){
			sd_sync = 1;
			sd_seq = 1;
		}
		if(sd_sync && (sd_seq == 1) && (buf.data()[45] == 1) && (buf.data()[55] == 0x30)){
			user_data[0] = buf.data()[56] ^ 0x4f;
			user_data[1] = buf.data()[57] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 2) && (buf.data()[45] == 2)){
			user_data[2] = buf.data()[55] ^ 0x70;
			user_data[3] = buf.data()[56] ^ 0x4f;
			user_data[4] = buf.data()[57] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 3) && (buf.data()[45] == 3) && (buf.data()[55] == 0x31)){
			user_data[5] = buf.data()[56] ^ 0x4f;
			user_data[6] = buf.data()[57] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 4) && (buf.data()[45] == 4)){
			user_data[7] = buf.data()[55] ^ 0x70;
			user_data[8] = buf.data()[56] ^ 0x4f;
			user_data[9] = buf.data()[57] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 5) && (buf.data()[45] == 5) && (buf.data()[55] == 0x32)){
			user_data[10] = buf.data()[56] ^ 0x4f;
			user_data[11] = buf.data()[57] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 6) && (buf.data()[45] == 6)){
			user_data[12] = buf.data()[55] ^ 0x70;
			user_data[13] = buf.data()[56] ^ 0x4f;
			user_data[14] = buf.data()[57] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 7) && (buf.data()[45] == 7) && (buf.data()[55] == 0x33)){
			user_data[15] = buf.data()[56] ^ 0x4f;
			user_data[16] = buf.data()[57] ^ 0x93;
			++sd_seq;
		}
		if(sd_sync && (sd_seq == 8) && (buf.data()[45] == 8)){
			user_data[17] = buf.data()[55] ^ 0x70;
			user_data[18] = buf.data()[56] ^ 0x4f;
			user_data[19] = buf.data()[57] ^ 0x93;
			user_data[20] = '\0';
			sd_sync = 0;
			sd_seq = 0;
			ui->usertxt->setText(QString::fromUtf8(user_data.data()));
		}

		for(int i = 0; i < 9; ++i){
			audioq.enqueue(buf.data()[46+i]);
		}
	}
}

void DudeStar::readyReadREF()
{
    QByteArray buf;
    QByteArray out;
    QHostAddress sender;
    quint16 senderPort;
    static bool sd_sync = 0;
    static int sd_seq = 0;
    char mycall[9], urcall[9], rptr1[9], rptr2[9];
    static unsigned short streamid = 0, s = 0;

    buf.resize(udp->pendingDatagramSize());
    udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);

#ifdef DEBUG
    fprintf(stderr, "RECV: ");
    for(int i = 0; i < buf.size(); ++i){
        fprintf(stderr, "%02x ", (unsigned char)buf.data()[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
#endif

    if ((buf.size() == 5) && (buf.data()[0] == 5)){
		int x = (::rand() % (999999 - 7245 + 1)) + 7245;
		QString serial = "HS" + QString("%1").arg(x, 6, 10, QChar('0'));
		out.append(0x1c);
		out.append(0xc0);
		out.append(0x04);
		out.append('\x00');
        out.append(callsign.toUpper().toLocal8Bit().data(), 6);
		out.append(10,'\x00');
		out.append(serial);
		//out.append("DV072475", 8);
		qDebug() << "serial == " << serial;
        udp->writeDatagram(out, address, 20001);
    }
    if(buf.size() == 3){ //2 way keep alive ping
		QString s;
		if(connect_status == CONNECTED_RW){
			s = "RW";
		}
		else if(connect_status == CONNECTED_RO){
			s = "RO";
		}
		status_txt->setText(s + " Host: " + host + ":" + QString::number(port) + " Ping: " + QString::number(ping_cnt++));
    }
#ifdef DEBUG
	if(out.size()){
		fprintf(stderr, "SEND: ");
		for(int i = 0; i < out.size(); ++i){
			fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
	}
#endif
    if((connect_status == CONNECTING) && (buf.size() == 0x08)){
		if((memcmp(&buf.data()[4], "OKRW", 4) == 0) || (memcmp(&buf.data()[4], "OKRO", 4) == 0) || (memcmp(&buf.data()[4], "BUSY", 4) == 0)){
		//if((buf.data()[4] == 0x4f) && (buf.data()[5] == 0x4b) && (buf.data()[6] == 0x52)){ // OKRW/OKRO response
			mbe = new MBEDecoder();
			mbe->setAutoGain(true);
			mbeenc = new MBEEncoder();
			mbeenc->set_dstar_mode();
			mbeenc->set_gain_adjust(3);
			ui->connectButton->setText("Disconnect");
			ui->connectButton->setEnabled(true);
			ui->modeCombo->setEnabled(false);
			ui->hostCombo->setEnabled(false);
			ui->callsignEdit->setEnabled(false);
			//ui->comboMod->setEnabled(false);
			if(buf.data()[7] == 0x57){ //OKRW
				connect_status = CONNECTED_RW;
				memset(rptr2, ' ', 8);
				memcpy(rptr2, hostname.toLocal8Bit(), hostname.size());
				rptr2[7] = module;
				rptr2[8] = 0;
				ui->rptr2Edit->setText(rptr2);
				if(hw_ambe_present || enable_swtx){
					ui->txButton->setDisabled(false);
				}
				audiotimer->start(19);
				ping_timer->start(1000);
				status_txt->setText("RW connect to " + host);
			}
			else if(buf.data()[7] == 0x4f){ //OKRO -- Go get registered!
				connect_status = CONNECTED_RO;
				status_txt->setText("RO connect to " + host);
			}
		}
		else if((buf.data()[4] == 0x46) && (buf.data()[5] == 0x41) && (buf.data()[6] == 0x49) && (buf.data()[7] == 0x4c)){ // FAIL response
			status_txt->setText("Connection refused by " + host);
			connect_status = DISCONNECTED;
			ui->connectButton->setText("Connect");
			ui->connectButton->setEnabled(true);
		}
		else{ //Unknown response
			ui->statusBar->showMessage("Unknown response by " + host);
			connect_status = DISCONNECTED;
		}
    }

	if((buf.size() == 0x3a) && (!memcmp(buf.data()+1, header, 5)) ){
		memcpy(rptr2, buf.data() + 20, 8); rptr1[8] = '\0';
		memcpy(rptr1, buf.data() + 28, 8); rptr2[8] = '\0';
        memcpy(urcall, buf.data() + 36, 8); urcall[8] = '\0';
        memcpy(mycall, buf.data() + 44, 8); mycall[8] = '\0';
		module = ui->comboMod->currentText().toStdString()[0];
		QString h = hostname + " " + module;
		if( (QString(rptr2).simplified() == h.simplified()) || (QString(rptr1).simplified() == h.simplified()) ){
			streamid = (buf.data()[14] << 8) | (buf.data()[15] & 0xff);
			QString ss = QString("%1").arg(streamid, 4, 16, QChar('0'));
			ui->mycall->setText(QString(mycall));
			ui->urcall->setText(QString(urcall));
			ui->rptr1->setText(QString(rptr1));
			ui->rptr2->setText(QString(rptr2));
			ui->streamid->setText(ss);
		}
		else{
			//streamid = 0;
		}
    }
	if((buf.size() == 0x1d) && (!memcmp(buf.data()+1, header, 5)) ){ //29
        s = (buf.data()[14] << 8) | (buf.data()[15] & 0xff);
        if(s != streamid){
            return;
        }
        QString ss = QString("%1").arg(s, 4, 16, QChar('0'));
		QString n = QString("%1").arg(buf.data()[16], 2, 10, QChar('0'));
		ui->streamid->setText(ss + " " + n);
        if((buf.data()[16] == 0) && (buf.data()[26] == 0x55) && (buf.data()[27] == 0x2d) && (buf.data()[28] == 0x16)){
            sd_sync = 1;
            sd_seq = 1;
        }
        if(sd_sync && (sd_seq == 1) && (buf.data()[16] == 1) && (buf.data()[26] == 0x30)){
           user_data[0] = buf.data()[27] ^ 0x4f;
           user_data[1] = buf.data()[28] ^ 0x93;
           ++sd_seq;
        }
        if(sd_sync && (sd_seq == 2) && (buf.data()[16] == 2)){
           user_data[2] = buf.data()[26] ^ 0x70;
           user_data[3] = buf.data()[27] ^ 0x4f;
           user_data[4] = buf.data()[28] ^ 0x93;
           ++sd_seq;
        }
        if(sd_sync && (sd_seq == 3) && (buf.data()[16] == 3) && (buf.data()[26] == 0x31)){
           user_data[5] = buf.data()[27] ^ 0x4f;
           user_data[6] = buf.data()[28] ^ 0x93;
           ++sd_seq;
        }
        if(sd_sync && (sd_seq == 4) && (buf.data()[16] == 4)){
           user_data[7] = buf.data()[26] ^ 0x70;
           user_data[8] = buf.data()[27] ^ 0x4f;
           user_data[9] = buf.data()[28] ^ 0x93;
           ++sd_seq;
        }
        if(sd_sync && (sd_seq == 5) && (buf.data()[16] == 5) && (buf.data()[26] == 0x32)){
           user_data[10] = buf.data()[27] ^ 0x4f;
           user_data[11] = buf.data()[28] ^ 0x93;
           ++sd_seq;
        }
        if(sd_sync && (sd_seq == 6) && (buf.data()[16] == 6)){
           user_data[12] = buf.data()[26] ^ 0x70;
           user_data[13] = buf.data()[27] ^ 0x4f;
           user_data[14] = buf.data()[28] ^ 0x93;
           ++sd_seq;
        }
        if(sd_sync && (sd_seq == 7) && (buf.data()[16] == 7) && (buf.data()[26] == 0x33)){
           user_data[15] = buf.data()[27] ^ 0x4f;
           user_data[16] = buf.data()[28] ^ 0x93;
           ++sd_seq;
        }
        if(sd_sync && (sd_seq == 8) && (buf.data()[16] == 8)){
           user_data[17] = buf.data()[26] ^ 0x70;
           user_data[18] = buf.data()[27] ^ 0x4f;
           user_data[19] = buf.data()[28] ^ 0x93;
           user_data[20] = '\0';
           sd_sync = 0;
           sd_seq = 0;
           ui->usertxt->setText(QString::fromUtf8(user_data.data()));
        }

        for(int i = 0; i < 9; ++i){
            audioq.enqueue(buf.data()[17+i]);
        }
    }
    if(buf.size() == 0x20){ //32
        ui->streamid->setText("Stream complete");
        ui->usertxt->clear();
    }
}

void DudeStar::handleStateChanged(QAudio::State)
{
}

void DudeStar::audioin_data_ready()
{
	QByteArray data;
	qint64 len = audioin->bytesReady();

	if (len > 0){
		data.resize(len);
		audioindev->read(data.data(), len);
/*
		fprintf(stderr, "AUDIOIN: ");
		for(int i = 0; i < len; ++i){
			fprintf(stderr, "%02x ", (unsigned char)data.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
*/
		for(int i = 0; i < len; i+=2){
			audioinq.enqueue(data.data()[i+1]);
			audioinq.enqueue(data.data()[i]);
			s16_le_audioinq.enqueue(((data.data()[i+1] << 8) & 0xff00) | (data.data()[i] & 0xff));
		}
	}
}

void DudeStar::press_tx()
{
    //std::cerr << "Pressed TX buffersize == " << audioin->bufferSize() << std::endl;
	tx = true;
	audiotx_cnt = 0;
#ifdef USE_FLITE
	int id = tts_voices->checkedId();
	if(id == 1){
		tts_audio = flite_text_to_wave(ui->TTSEdit->text().toStdString().c_str(), voice_kal);
	}
	else if(id == 2){
		tts_audio = flite_text_to_wave(ui->TTSEdit->text().toStdString().c_str(), voice_rms);
	}
	else if(id == 3){
		tts_audio = flite_text_to_wave(ui->TTSEdit->text().toStdString().c_str(), voice_awb);
	}
	else if(id == 4){
		tts_audio = flite_text_to_wave(ui->TTSEdit->text().toStdString().c_str(), voice_slt);
	}
#endif
	if(!txtimer->isActive()){
		//fprintf(stderr, "press_tx()\n");
		//audio_buffer.open(QBuffer::ReadWrite|QBuffer::Truncate);
		//audiofile.setFileName("audio.pcm");
		//audiofile.open(QIODevice::WriteOnly | QIODevice::Truncate);
		audioin->setBufferSize(512);
		//audioin->start(&audio_buffer);
		ambeq.clear();
		audioinq.clear();
		s16_le_audioinq.clear();

		audioindev = audioin->start();
		connect(audioindev, SIGNAL(readyRead()), SLOT(audioin_data_ready()));
		txtimer->start(19);
	}
}

void DudeStar::release_tx()
{
	//std::cerr << "Released TX" << std::endl;
	tx = false;

	/*
	QByteArray a = audio_buffer.readAll();
	fprintf(stderr, "RECV %d:%d:%d", audio_buffer.buffer().size(), audio_buffer.bytesAvailable(), audioin->bytesReady());
	for(int i = 0; i < audio_buffer.buffer().size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)audio_buffer.buffer().data()[i]);
		std::cout << audio_buffer.buffer().data()[i];
	}
	fprintf(stderr, "\n");
	fflush(stderr);
	*/
	//audio_buffer.reset();
	//audio_buffer.close();

	//audiofile.close();
}

void DudeStar::tx_timer()
{
	static uint8_t cnt = 0;
	//static uint32_t cnt2 = 0;
	QByteArray a;
	uint8_t ambe_frame[88];
	uint8_t ambe_bytes[11];
	uint8_t frame_len = 0;
	int16_t audio_samples[160];
	memset(ambe_bytes, 0, 11);
	memset(ambe_frame, 0, 88);
	memset(audio_samples, 0, 320);

	a.append(0x61);
	a.append(0x01);
	a.append(0x43);
	a.append(0x02);
	a.append(0x40);
	a.append('\x00');
	a.append(0xa0);
	//a.append(320, 0);
/*
	for(int i = 0; i < 320; i += 2){
		a.append(audio_count[audiotx_cnt+1]);
		a.append(audio_count[audiotx_cnt]);
		audio_samples[i/2] = ((audio_count[audiotx_cnt+1] << 8) & 0xff00) | (audio_count[audiotx_cnt] & 0xff);
		audiotx_cnt += 2;
		if(audiotx_cnt >= sizeof(audio_count)){
			audiotx_cnt = 0;
		}
	}
*/
#ifdef USE_FLITE
	if(tts_voices->checkedId() > 0){
		for(int i = 0; i < 160; ++i){
			if(audiotx_cnt >= tts_audio->num_samples/2){
				//audiotx_cnt = 0;
				audio_samples[i] = 0;
				a.append(static_cast<char>(0));
				a.append(static_cast<char>(0));
			}
			else{
				a.append((tts_audio->samples[audiotx_cnt*2] >> 8) & 0xff);
				a.append(tts_audio->samples[audiotx_cnt*2] & 0xff);
				audio_samples[i] = tts_audio->samples[audiotx_cnt*2] / 2;
				++audiotx_cnt;
			}
		}
	}
	else{
#endif
		for(int i = 0; i < 320; ++i){
			if(audioinq.size()){
				a.append(audioinq.dequeue());
			}
			else{
				a.append(1, 0);
			}
			if((i < 160) && (s16_le_audioinq.size())){
				audio_samples[i] = s16_le_audioinq.dequeue();
			}
		}
#ifdef USE_FLITE
	}
#endif

/*
	a[0] = 0x61;
	a[1] = 0x00;
	a[2] = 0x0b;
	a[3] = 0x01;
	a[4] = 0x01;
	a[5] = 0x48;
	for(int i = 0; i < 9; ++i){
		a.append(ambe[cnt2]);
		++cnt2;
		if(cnt2 >= sizeof(ambe)){
			cnt2 = 0;
		}
	}
*/
	if(tx){
		if(hwtx){
			if(protocol == "YSF"){
				ysf->use_hwambe(true);
			}
			if(protocol == "NXDN"){
				nxdn->set_hwtx(true);
			}
			serial->write(a);
		}
		else{
			if(protocol == "P25"){
				mbeenc->encode(audio_samples, ambe_bytes);
				frame_len = 11;
			}
			else{
				mbeenc->encode(audio_samples, ambe_frame);
				if((protocol == "YSF") || (protocol == "NXDN")){
					frame_len = 7;
				}
				else{
					frame_len = 9;
				}
			}
			ambeq.enqueue(0x61);
			ambeq.enqueue(0);
			ambeq.enqueue(0);
			ambeq.enqueue(0x01);
			ambeq.enqueue(0);
			ambeq.enqueue(0);
/*
			fprintf(stderr, "AMBEF: ");
			for(int i = 0; i < 72; ++i){
				fprintf(stderr, "%02x ", ambe_frame[i]);
			}
			fprintf(stderr, "\n");
			fflush(stderr);
*/
			for(int i = 0; i < frame_len; ++i){
				if(protocol != "P25"){
					for(int j = 0; j < 8; ++j){
						//ambe_bytes[i] |= (ambe_frame[((8-i)*8)+(7-j)] << (7-j));
						//if(protocol != "YSF"){
						if( (protocol == "DCS") || (protocol == "XRF") || (protocol == "REF") ){
							ambe_bytes[i] |= (ambe_frame[(i*8)+j] << j);
						}
						else{
							if(protocol == "YSF"){
								ysf->use_hwambe(false);
							}
							if(protocol == "NXDN"){
								nxdn->set_hwtx(false);
							}
							ambe_bytes[i] |= (ambe_frame[(i*8)+j] << (7-j));
						}
					}
				}
				ambeq.enqueue(ambe_bytes[i]);
			}

			fprintf(stderr, "AMBESW: ");
			for(int i = 0; i < frame_len; ++i){
				fprintf(stderr, "%02x ", ambe_bytes[i]);
			}
			fprintf(stderr, "\n");
			fflush(stderr);
		}
	}
	if((protocol == "YSF") && (cnt == 5)){
		transmit();
		cnt = 0;
	}
	else if((protocol == "NXDN") && (cnt == 4)){
		transmit();
		cnt = 0;
	}
	else if((protocol == "DMR") && (cnt == 3)){
		transmit();
		cnt = 0;
	}
	else{
		transmit();
	}
	++cnt;
}

void DudeStar::process_serial()
{
	QByteArray data = serial->readAll();
#ifdef DEBUGHW
	fprintf(stderr, "AMBEHW %d:%d:", data.size(), ambeq.size());
	for(int i = 0; i < data.size(); ++i){
		//if((data.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
		//	i+= 6;
		//}
		fprintf(stderr, "%02x ", (unsigned char)data.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	if( (data.data()[0] == 0x61) && (data.data()[3] == 0x00) ){
		//qDebug() << "Dropping control packet response";
		return;
	}

	if( (data.data()[0] == 0x61) && (data.data()[3] == 0x01) && (data.data()[2] == (data.size()-4))){
		//fprintf(stderr, "AMBE %d:%d:", data.size(), ambeq.size());
		for(int i = 0; i < data.size(); ++i){
			//if (i > 5) fprintf(stderr, "%02x ", (unsigned char)data.data()[i]);
			ambeq.enqueue(data.data()[i]);
		}
		//fprintf(stderr, "\n");
		//fflush(stderr);
	}
	//if( (data.data()[0] == 0x61) && (data.data()[3] == 0x02) ){
	if(!tx){
		//fprintf(stderr, "AMBE %d:%d:", data.size(), ambeq.size());
		for(int i = 0; i < data.size(); ++i){
			//if (i > 5) fprintf(stderr, "%02x ", (unsigned char)data.data()[i]);
			//ambeq.enqueue(data.data()[i]);
			hw_ambe_audio.enqueue(data.data()[i]);
		}
		//fprintf(stderr, "\n");
		//fflush(stderr);
	}
}

void DudeStar::transmit()
{
	if(protocol == "REF"){
		transmitREF();
	}
	if(protocol == "DCS"){
		transmitDCS();
	}
	if(protocol == "XRF"){
		transmitXRF();
	}
	else if (protocol == "YSF"){
		transmitYSF();
	}
	else if (protocol == "DMR"){
		transmitDMR();
	}
	else if (protocol == "P25"){
		transmitP25();
	}
	else if (protocol == "NXDN"){
		transmitNXDN();
	}
}

void DudeStar::transmitNXDN()
{
	QByteArray ambe;
	QByteArray txdata;
	unsigned char *temp_nxdn;
	if(tx || ambeq.size()){

		ambe.clear();
		if(ambeq.size() < 55){
			if(!tx){
				ambeq.clear();
				return;
			}
			else{
				std::cerr << "ERROR: NXDN AMBEQ < 55" << std::endl;
				return;
			}
		}
		while(ambeq.size() && (ambeq[0] != 0x61) && (ambeq[3] != 0x01)){
			std::cerr << "ERROR: Not an AMBE frame" << std::endl;
			ambeq.dequeue();
		}
		for(int i = 0; i < 5; ++i){
			if(ambeq.size() < 13){
				std::cerr << "ERROR: NXDN AMBE Q empty" << std::endl;
				return;
			}
			else{
				for (int i = 0; i < 6; ++i){
					ambeq.dequeue();
				}
				for (int i = 0; i < 7; ++i){
					ambe.append(ambeq.dequeue());
				}
				//ambe.append('\x00');
				//ambe.append('\x00');
			}
		}
		temp_nxdn = nxdn->get_frame((unsigned char *)ambe.data());

		txdata.append((char *)temp_nxdn, 43);
		udp->writeDatagram(txdata, address, port);

		fprintf(stderr, "SEND:%d: ", ambeq.size());
		for(int i = 0; i < txdata.size(); ++i){
			fprintf(stderr, "%02x ", (unsigned char)txdata.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	else{
		fprintf(stderr, "NXDN TX stopped\n");
		txtimer->stop();
		audioindev->disconnect();
		audioin->stop();
		temp_nxdn = nxdn->get_eot();
		txdata.append((char *)temp_nxdn, 43);
		udp->writeDatagram(txdata, address, port);
	}

}

void DudeStar::transmitYSF()
{
	QByteArray ambe;
	QByteArray txdata;
	unsigned char *temp_ysf;
	if(tx || ambeq.size()){

		ambe.clear();
		if(ambeq.size() < 65){
			if(!tx){
				ambeq.clear();
				return;
			}
			else{
				std::cerr << "ERROR: AMBEQ < 65" << std::endl;
				return;
			}
		}
		while(ambeq.size() && (ambeq[0] != 0x61) && (ambeq[3] != 0x01)){
			std::cerr << "ERROR: Not an AMBE frame" << std::endl;
			ambeq.dequeue();
		}
		for(int i = 0; i < 5; ++i){
			if(ambeq.size() < 13){
				std::cerr << "ERROR:  AMBE Q empty" << std::endl;
				return;
			}
			else{
				for (int i = 0; i < 6; ++i){
					ambeq.dequeue();
				}
				for (int i = 0; i < 7; ++i){
					ambe.append(ambeq.dequeue());
				}
				ambe.append('\x00');
				ambe.append('\x00');
			}
		}
		temp_ysf = ysf->get_frame((unsigned char *)ambe.data());

/*
		temp_ysf = ysftxdata + txcnt;
		txcnt += 155;
		if(txcnt >= sizeof(ysftxdata))
			txcnt = 0;

*/
		txdata.append((char *)temp_ysf, 155);
		udp->writeDatagram(txdata, address, port);
	}
	else{
		fprintf(stderr, "YSF TX stopped\n");
		txtimer->stop();
		audioindev->disconnect();
		audioin->stop();
		temp_ysf = ysf->get_eot();
		txdata.append((char *)temp_ysf, 155);
		udp->writeDatagram(txdata, address, port);
	}
}

void DudeStar::transmitP25()
{
	QByteArray ambe;
	QByteArray txdata;
	//unsigned char *m_p25Frame;
	unsigned char m_p25Frame[11];
	unsigned char buffer[22];
	static uint8_t p25step = 0;

	if(ambeq.size() < 17){
		if(!tx){
			txdata.append((char *)REC80, 17U);
			udp->writeDatagram(txdata, address, port);
			fprintf(stderr, "P25 TX stopped ambeq.size() == %d\n", ambeq.size());
			txtimer->stop();
			audioindev->disconnect();
			audioin->stop();
			p25step = 0;
			ambeq.clear();
			return;
		}
		else{
			std::cerr << "ERROR: AMBEQ < 24" << std::endl;
			return;
		}
	}
	while(ambeq.size() && (ambeq[0] != 0x61) && (ambeq[2] != 0x0d) && (ambeq[3] != 0x01)){
		std::cerr << "ERROR: Not an AMBE frame" << std::endl;
		ambeq.dequeue();
	}

	if(ambeq.size() < 17){
		std::cerr << "ERROR:  AMBE Q empty" << std::endl;
		return;
	}
	else{
		for (int i = 0; i < 6; ++i){
			ambeq.dequeue();
		}
		for (int i = 0; i < 11; ++i){
			//ambe.append(ambeq.dequeue());
			m_p25Frame[i] = ambeq.dequeue();
		}
	}

	if(tx || ambeq.size()){
		//m_p25Frame = p25->get_frame((unsigned char *)ambe.data());

		switch (p25step) {
		case 0x00U:
			::memcpy(buffer, REC62, 22U);
			::memcpy(buffer + 10U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 22U);
			++p25step;
			break;
		case 0x01U:
			::memcpy(buffer, REC63, 14U);
			::memcpy(buffer + 1U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 14U);
			++p25step;
			break;
		case 0x02U:
			::memcpy(buffer, REC64, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			buffer[1U] = 0x00U;
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x03U:
			dmr_destid = ui->dmrtgEdit->text().toUInt();
			::memcpy(buffer, REC65, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			buffer[1U] = (dmr_destid >> 16) & 0xFFU;
			buffer[2U] = (dmr_destid >> 8) & 0xFFU;
			buffer[3U] = (dmr_destid >> 0) & 0xFFU;
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x04U:
			::memcpy(buffer, REC66, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			buffer[1U] = (dmrid >> 16) & 0xFFU;
			buffer[2U] = (dmrid >> 8) & 0xFFU;
			buffer[3U] = (dmrid >> 0) & 0xFFU;
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x05U:
			::memcpy(buffer, REC67, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x06U:
			::memcpy(buffer, REC68, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x07U:
			::memcpy(buffer, REC69, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x08U:
			::memcpy(buffer, REC6A, 16U);
			::memcpy(buffer + 4U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 16U);
			++p25step;
			break;
		case 0x09U:
			::memcpy(buffer, REC6B, 22U);
			::memcpy(buffer + 10U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 22U);
			++p25step;
			break;
		case 0x0AU:
			::memcpy(buffer, REC6C, 14U);
			::memcpy(buffer + 1U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 14U);
			++p25step;
			break;
		case 0x0BU:
			::memcpy(buffer, REC6D, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x0CU:
			::memcpy(buffer, REC6E, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x0DU:
			::memcpy(buffer, REC6F, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x0EU:
			::memcpy(buffer, REC70, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			buffer[1U] = 0x80U;
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x0FU:
			::memcpy(buffer, REC71, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x10U:
			::memcpy(buffer, REC72, 17U);
			::memcpy(buffer + 5U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 17U);
			++p25step;
			break;
		case 0x11U:
			::memcpy(buffer, REC73, 16U);
			::memcpy(buffer + 4U, m_p25Frame, 11U);
			txdata.append((char *)buffer, 16U);
			p25step = 0;
			break;
		}
		udp->writeDatagram(txdata, address, port);
	}
	else{
		txdata.append((char *)REC80, 17U);
		udp->writeDatagram(txdata, address, port);
		fprintf(stderr, "P25 TX stopped\n");
		txtimer->stop();
		audioindev->disconnect();
		audioin->stop();
		p25step = 0;
	}

}

void DudeStar::transmitDMR()
{
	QByteArray ambe;
	QByteArray txdata;
	unsigned char *temp_dmr;
	//static uint16_t txcnt = 0;
	dmr_destid = ui->dmrtgEdit->text().toUInt();
	dmr->set_dstid(dmr_destid);
	dmr->set_cc(ui->dmrccEdit->text().toUInt());
	dmr->set_slot(ui->dmrslotEdit->text().toUInt());
	if(ui->checkBoxDMRPC->isChecked()){
		dmr->set_calltype(3);
	}
	else{
		dmr->set_calltype(0);
	}
	if(tx || ambeq.size()){

		ambe.clear();
		if(ambeq.size() < 45){
			if(!tx){
				ambeq.clear();
				return;
			}
			else{
				//std::cerr << "ERROR: AMBEQ < 45" << std::endl;
				return;
			}
		}
		while(ambeq.size() && (ambeq[0] != 0x61) && (ambeq[2] != 0x0b) && (ambeq[3] != 0x01)){
			//std::cerr << "ERROR: Not an AMBE frame" << std::endl;
			ambeq.dequeue();
		}
		for(int i = 0; i < 3; ++i){
			if(ambeq.size() < 15){
				//std::cerr << "ERROR:  AMBE Q empty" << std::endl;
				return;
			}
			else{
				for (int i = 0; i < 6; ++i){
					ambeq.dequeue();
				}
				for (int i = 0; i < 9; ++i){
					ambe.append(ambeq.dequeue());
				}
			}
		}
		temp_dmr = dmr->get_frame((unsigned char *)ambe.data());

/*
		temp_ysf = ysftxdata + txcnt;
		txcnt += 155;
		if(txcnt >= sizeof(ysftxdata))
			txcnt = 0;
*/
		txdata.append((char *)temp_dmr, 55);
		udp->writeDatagram(txdata, address, port);
	}
	else{
		//fprintf(stderr, "DMR TX stopped\n");
		txtimer->stop();
		audioindev->disconnect();
		audioin->stop();
		temp_dmr = dmr->get_eot();
		txdata.append((char *)temp_dmr, 55);
		udp->writeDatagram(txdata, address, port);
	}
#ifdef DEBUG
	fprintf(stderr, "SEND:%d: ", ambeq.size());
	for(int i = 0; i < txdata.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)txdata.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void DudeStar::transmitDCS()
{
	static uint32_t tx_cnt = 0;
	static QByteArray txdata;
	static uint16_t txstreamid = 0;
	uint8_t last_frame[9] = {0xdc, 0x8e, 0x0a, 0x40, 0xad, 0xed, 0xad, 0x39, 0x6e};

	txdata.resize(100);
	memset(txdata.data(), 0, 100);

	QString mycall = ui->mycallEdit->text().toUpper().simplified();
	QString urcall = ui->urcallEdit->text().toUpper().simplified();
	QString rptr1 = ui->rptr1Edit->text().toUpper().simplified();
	QString rptr2 = ui->rptr2Edit->text().toUpper().simplified();
	ui->mycallEdit->setText(mycall);
	ui->urcallEdit->setText(urcall);
	ui->rptr1Edit->setText(rptr1);
	ui->rptr2Edit->setText(rptr2);

	int j = mycall.size();
	for(int i = 0; i < (9 - j); ++i){
		mycall.append(' ');
	}
	j = urcall.size();
	for(int i = 0; i < (9 - j); ++i){
		urcall.append(' ');
	}

	QStringList sl = rptr1.split(' ');
	if(sl.size() > 1){
		rptr1 = sl.at(0).simplified();
		while(rptr1.size() < 7){
			rptr1.append(' ');
		}
		rptr1 += sl.at(1).simplified();
	}
	else{
		while(rptr1.size() < 8){
			rptr1.append(' ');
		}
	}
	sl = rptr2.split(' ');
	if(sl.size() > 1){
		rptr2 = sl.at(0).simplified();
		while(rptr2.size() < 7){
			rptr2.append(' ');
		}
		rptr2 += sl.at(1).simplified();
	}
	else{
		while(rptr1.size() < 8){
			rptr2.append(' ');
		}
	}

	if(txstreamid == 0){
	   txstreamid = static_cast<uint16_t>((::rand() & 0xFFFF));
	   //std::cerr << "txstreamid == " << txstreamid << std::endl;
	}
	memcpy(txdata.data(), "0001", 4);
	memcpy(txdata.data() + 7, rptr2.toLocal8Bit().data(), 8);
	memcpy(txdata.data() + 15, rptr1.toLocal8Bit().data(), 8);
	memcpy(txdata.data() + 23, urcall.toLocal8Bit().data(), 8);
	memcpy(txdata.data() + 31, mycall.toLocal8Bit().data(), 8);
	memcpy(txdata.data() + 39, "DUDE", 4);
	memcpy(txdata.data() + 43, &txstreamid, 2);
	txdata[45] = (tx_cnt % 21) & 0xff;

	switch(txdata.data()[45]){
	case 0:
		txdata[55] = 0x55;
		txdata[56] = 0x2d;
		txdata[57] = 0x16;
		break;
	case 1:
		txdata[55] = 0x40 ^ 0x70;
		txdata[56] = ui->usertxtEdit->text().toLocal8Bit().data()[0] ^ 0x4f;
		txdata[57] = ui->usertxtEdit->text().toLocal8Bit().data()[1] ^ 0x93;
		break;
	case 2:
		txdata[55] = ui->usertxtEdit->text().toLocal8Bit().data()[2] ^ 0x70;
		txdata[56] = ui->usertxtEdit->text().toLocal8Bit().data()[3] ^ 0x4f;
		txdata[57] = ui->usertxtEdit->text().toLocal8Bit().data()[4] ^ 0x93;
		break;
	case 3:
		txdata[55] = 0x41 ^ 0x70;
		txdata[56] = ui->usertxtEdit->text().toLocal8Bit().data()[5] ^ 0x4f;
		txdata[57] = ui->usertxtEdit->text().toLocal8Bit().data()[6] ^ 0x93;
		break;
	case 4:
		txdata[55] = ui->usertxtEdit->text().toLocal8Bit().data()[7] ^ 0x70;
		txdata[56] = ui->usertxtEdit->text().toLocal8Bit().data()[8] ^ 0x4f;
		txdata[57] = ui->usertxtEdit->text().toLocal8Bit().data()[9] ^ 0x93;
		break;
	case 5:
		txdata[55] = 0x42 ^ 0x70;
		txdata[56] = ui->usertxtEdit->text().toLocal8Bit().data()[10] ^ 0x4f;
		txdata[57] = ui->usertxtEdit->text().toLocal8Bit().data()[11] ^ 0x93;
		break;
	case 6:
		txdata[55] = ui->usertxtEdit->text().toLocal8Bit().data()[12] ^ 0x70;
		txdata[56] = ui->usertxtEdit->text().toLocal8Bit().data()[13] ^ 0x4f;
		txdata[57] = ui->usertxtEdit->text().toLocal8Bit().data()[14] ^ 0x93;
		break;
	case 7:
		txdata[55] = 0x43 ^ 0x70;
		txdata[56] = ui->usertxtEdit->text().toLocal8Bit().data()[15] ^ 0x4f;
		txdata[57] = ui->usertxtEdit->text().toLocal8Bit().data()[16] ^ 0x93;
		break;
	case 8:
		txdata[55] = ui->usertxtEdit->text().toLocal8Bit().data()[17] ^ 0x70;
		txdata[56] = ui->usertxtEdit->text().toLocal8Bit().data()[18] ^ 0x4f;
		txdata[57] = ui->usertxtEdit->text().toLocal8Bit().data()[19] ^ 0x93;
		break;
	default:
		txdata[55] = 0x16;
		txdata[56] = 0x29;
		txdata[57] = 0xf5;
		break;
	}

	txdata[58] = tx_cnt & 0xff;
	txdata[59] = (tx_cnt >> 8) & 0xff;
	txdata[60] = (tx_cnt >> 16) & 0xff;
	txdata[61] = 0x01;

	if(tx || ambeq.size()){
		while(ambeq.size() && (ambeq[0] != 0x61)){
			std::cerr << "ERROR: Lost sync" << std::endl;
			ambeq.dequeue();
		}
		if(ambeq.size() < 15){
			std::cerr << "ERROR:  AMBE Q empty" << std::endl;
			return;
		}
		else{
			for (int i = 0; i < 6; ++i){
				ambeq.dequeue();
			}
		}

		if(ambeq.size() > 8){
			txdata[46] = ambeq.dequeue();
			txdata[47] = ambeq.dequeue();
			txdata[48] = ambeq.dequeue();
			txdata[49] = ambeq.dequeue();
			txdata[50] = ambeq.dequeue();
			txdata[51] = ambeq.dequeue();
			txdata[52] = ambeq.dequeue();
			txdata[53] = ambeq.dequeue();
			txdata[54] = ambeq.dequeue();
		}

		//for(int i = 0; i < 9; ++i){
			//txdata[17 + i] = ad8dp[(tx_cnt * 9) + i];
			//if(ambeq.size()){
			//	txdata[17 + i] = ambeq.dequeue();
			//}
			//else{
			//	txdata[17 + i] = 0;
			//}
		//}

		//memset(txdata.data() + 17, 0x00, 9);
		++tx_cnt;
		udp->writeDatagram(txdata, address, 30051);
	}
	else if(!ambeq.size()){
		txdata.data()[45] |= 0x40;
		memcpy(txdata.data() + 46, last_frame, 9);
		tx_cnt = 0;
		txstreamid = 0;
		udp->writeDatagram(txdata, address, 30051);
		txtimer->stop();
		audioindev->disconnect();
		audioin->stop();
	}
#ifdef DEBUG
	fprintf(stderr, "SEND:%d: ", ambeq.size());
	for(int i = 0; i < txdata.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)txdata.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void DudeStar::transmitXRF()
{
	static uint32_t tx_cnt = 0;
	static QByteArray txdata;
	static uint16_t txstreamid = 0;
	static bool sendheader = 1;

	QString mycall = ui->mycallEdit->text().toUpper().simplified();
	QString urcall = ui->urcallEdit->text().toUpper().simplified();
	QString rptr1 = ui->rptr1Edit->text().toUpper().simplified();
	QString rptr2 = ui->rptr2Edit->text().toUpper().simplified();
	ui->mycallEdit->setText(mycall);
	ui->urcallEdit->setText(urcall);
	ui->rptr1Edit->setText(rptr1);
	ui->rptr2Edit->setText(rptr2);

	int j = mycall.size();
	for(int i = 0; i < (9 - j); ++i){
		mycall.append(' ');
	}
	j = urcall.size();
	for(int i = 0; i < (9 - j); ++i){
		urcall.append(' ');
	}

	QStringList sl = rptr1.split(' ');
	if(sl.size() > 1){
		rptr1 = sl.at(0).simplified();
		while(rptr1.size() < 7){
			rptr1.append(' ');
		}
		rptr1 += sl.at(1).simplified();
	}
	else{
		while(rptr1.size() < 8){
			rptr1.append(' ');
		}
	}
	sl = rptr2.split(' ');
	if(sl.size() > 1){
		rptr2 = sl.at(0).simplified();
		while(rptr2.size() < 7){
			rptr2.append(' ');
		}
		rptr2 += sl.at(1).simplified();
	}
	else{
		while(rptr1.size() < 8){
			rptr2.append(' ');
		}
	}

	if(txstreamid == 0){
	   txstreamid = static_cast<uint16_t>((::rand() & 0xFFFF));
	   //std::cerr << "txstreamid == " << txstreamid << std::endl;
	}
	if(sendheader){
		sendheader = 0;
		//txdata.clear();
		txdata.resize(56);
		txdata[0] = 0x44;
		txdata[1] = 0x53;
		txdata[2] = 0x56;
		txdata[3] = 0x54;
		txdata[4] = 0x10;
		txdata[5] = 0x00;
		txdata[6] = 0x00;
		txdata[7] = 0x00;
		txdata[8] = 0x20;
		txdata[9] = 0x00;
		txdata[10] = 0x01;
		txdata[11] = 0x02;
		txdata[12] = txstreamid & 0xff;
		txdata[13] = (txstreamid >> 8) & 0xff;
		txdata[14] = 0x80;
		txdata[15] = 0x00;
		txdata[16] = 0x00;
		txdata[17] = 0x00;
		//memcpy(txdata.data() + 20, ui->rptr1Edit->text().toStdString().c_str(), 8);
		memcpy(txdata.data() + 18, rptr2.toLocal8Bit().data(), 8);
		memcpy(txdata.data() + 26, rptr1.toLocal8Bit().data(), 8);
		memcpy(txdata.data() + 34, urcall.toLocal8Bit().data(), 8);
		memcpy(txdata.data() + 42, mycall.toLocal8Bit().data(), 8);
		memcpy(txdata.data() + 50, "51a+", 4);
		txdata[54] = 0;
		txdata[55] = 0;
		calcPFCS(txdata.data());
		udp->writeDatagram(txdata, address, 30001);
	}
	else if(tx || ambeq.size()){
		while(ambeq.size() && (ambeq[0] != 0x61)){
			//std::cerr << "ERROR: Lost sync" << std::endl;
			ambeq.dequeue();
		}
		if(ambeq.size() < 15){
			//std::cerr << "ERROR:  AMBE Q empty" << std::endl;
			return;
		}
		else{
			for (int i = 0; i < 6; ++i){
				ambeq.dequeue();
			}
		}
		txdata.resize(27);
		txdata[4] = 0x20;
		txdata[14] = tx_cnt % 21;
		if(ambeq.size() > 8){
			txdata[15] = ambeq.dequeue();
			txdata[16] = ambeq.dequeue();
			txdata[17] = ambeq.dequeue();
			txdata[18] = ambeq.dequeue();
			txdata[19] = ambeq.dequeue();
			txdata[20] = ambeq.dequeue();
			txdata[21] = ambeq.dequeue();
			txdata[22] = ambeq.dequeue();
			txdata[23] = ambeq.dequeue();
		}

		//for(int i = 0; i < 9; ++i){
			//txdata[17 + i] = ad8dp[(tx_cnt * 9) + i];
			//if(ambeq.size()){
			//	txdata[17 + i] = ambeq.dequeue();
			//}
			//else{
			//	txdata[17 + i] = 0;
			//}
		//}

		//memset(txdata.data() + 17, 0x00, 9);
		switch(txdata.data()[14]){
		case 0:
			txdata[24] = 0x55;
			txdata[25] = 0x2d;
			txdata[26] = 0x16;
			break;
		case 1:
			txdata[24] = 0x40 ^ 0x70;
			txdata[25] = ui->usertxtEdit->text().toLocal8Bit().data()[0] ^ 0x4f;
			txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[1] ^ 0x93;
			break;
		case 2:
			txdata[24] = ui->usertxtEdit->text().toLocal8Bit().data()[2] ^ 0x70;
			txdata[25] = ui->usertxtEdit->text().toLocal8Bit().data()[3] ^ 0x4f;
			txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[4] ^ 0x93;
			break;
		case 3:
			txdata[24] = 0x41 ^ 0x70;
			txdata[25] = ui->usertxtEdit->text().toLocal8Bit().data()[5] ^ 0x4f;
			txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[6] ^ 0x93;
			break;
		case 4:
			txdata[24] = ui->usertxtEdit->text().toLocal8Bit().data()[7] ^ 0x70;
			txdata[25] = ui->usertxtEdit->text().toLocal8Bit().data()[8] ^ 0x4f;
			txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[9] ^ 0x93;
			break;
		case 5:
			txdata[24] = 0x42 ^ 0x70;
			txdata[25] = ui->usertxtEdit->text().toLocal8Bit().data()[10] ^ 0x4f;
			txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[11] ^ 0x93;
			break;
		case 6:
			txdata[24] = ui->usertxtEdit->text().toLocal8Bit().data()[12] ^ 0x70;
			txdata[25] = ui->usertxtEdit->text().toLocal8Bit().data()[13] ^ 0x4f;
			txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[14] ^ 0x93;
			break;
		case 7:
			txdata[24] = 0x43 ^ 0x70;
			txdata[25] = ui->usertxtEdit->text().toLocal8Bit().data()[15] ^ 0x4f;
			txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[16] ^ 0x93;
			break;
		case 8:
			txdata[24] = ui->usertxtEdit->text().toLocal8Bit().data()[17] ^ 0x70;
			txdata[25] = ui->usertxtEdit->text().toLocal8Bit().data()[18] ^ 0x4f;
			txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[19] ^ 0x93;
			break;
		default:
			txdata[24] = 0x16;
			txdata[25] = 0x29;
			txdata[26] = 0xf5;
			break;
		}
		udp->writeDatagram(txdata, address, 30001);
		++tx_cnt;
		//if((tx_cnt * 9) >= sizeof(ad8dp)){
		//	tx_cnt = 0;
		//}
	}
	else {
		if(!ambeq.size()){
			txdata[14] = (++tx_cnt % 21) | 0x40;
			udp->writeDatagram(txdata, address, 30001);
			tx_cnt = 0;
			txstreamid = 0;
			sendheader = 1;
			txtimer->stop();
			audioindev->disconnect();
			audioin->stop();
		}
	}
#ifdef DEBUG
	fprintf(stderr, "SEND:%d: ", ambeq.size());
	for(int i = 0; i < txdata.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)txdata.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void DudeStar::transmitREF()
{
    static uint32_t tx_cnt = 0;
    static QByteArray txdata;
    static uint16_t txstreamid = 0;
    static bool sendheader = 1;

	QString mycall = ui->mycallEdit->text().toUpper().simplified();
	QString urcall = ui->urcallEdit->text().toUpper().simplified();
	QString rptr1 = ui->rptr1Edit->text().toUpper().simplified();
	QString rptr2 = ui->rptr2Edit->text().toUpper().simplified();
	ui->mycallEdit->setText(mycall);
	ui->urcallEdit->setText(urcall);
	ui->rptr1Edit->setText(rptr1);
	ui->rptr2Edit->setText(rptr2);

	int j = mycall.size();
	for(int i = 0; i < (9 - j); ++i){
		mycall.append(' ');
	}
	j = urcall.size();
	for(int i = 0; i < (9 - j); ++i){
		urcall.append(' ');
	}

	QStringList sl = rptr1.split(' ');
	if(sl.size() > 1){
		rptr1 = sl.at(0).simplified();
		while(rptr1.size() < 7){
			rptr1.append(' ');
		}
		rptr1 += sl.at(1).simplified();
	}
	else{
		while(rptr1.size() < 8){
			rptr1.append(' ');
		}
	}
	sl = rptr2.split(' ');
	if(sl.size() > 1){
		rptr2 = sl.at(0).simplified();
		while(rptr2.size() < 7){
			rptr2.append(' ');
		}
		rptr2 += sl.at(1).simplified();
	}
	else{
		while(rptr1.size() < 8){
			rptr2.append(' ');
		}
	}

    if(txstreamid == 0){
       txstreamid = static_cast<uint16_t>((::rand() & 0xFFFF));
	   //std::cerr << "txstreamid == " << txstreamid << std::endl;
    }
    if(sendheader){
        sendheader = 0;
        //txdata.clear();
        txdata.resize(58);
		txdata[0] = 0x3a;
        txdata[1] = 0x80;
        txdata[2] = 0x44;
        txdata[3] = 0x53;
        txdata[4] = 0x56;
        txdata[5] = 0x54;
        txdata[6] = 0x10;
        txdata[7] = 0x00;
        txdata[8] = 0x00;
        txdata[9] = 0x00;
        txdata[10] = 0x20;
        txdata[11] = 0x00;
        txdata[12] = 0x02;
        txdata[13] = 0x01;
        txdata[14] = txstreamid & 0xff;
        txdata[15] = (txstreamid >> 8) & 0xff;
        txdata[16] = 0x80;
        txdata[17] = 0x00;
        txdata[18] = 0x00;
        txdata[19] = 0x00;
        //memcpy(txdata.data() + 20, ui->rptr1Edit->text().toStdString().c_str(), 8);
		memcpy(txdata.data() + 20, rptr2.toLocal8Bit().data(), 8);
		memcpy(txdata.data() + 28, rptr1.toLocal8Bit().data(), 8);
		memcpy(txdata.data() + 36, urcall.toLocal8Bit().data(), 8);
		memcpy(txdata.data() + 44, mycall.toLocal8Bit().data(), 8);
		memcpy(txdata.data() + 52, "dude", 4);
        txdata[56] = 0;
        txdata[57] = 0;
        calcPFCS(txdata.data());
        udp->writeDatagram(txdata, address, 20001);
    }
	else if(tx || ambeq.size()){
		while(ambeq.size() && (ambeq[0] != 0x61)){
			//std::cerr << "ERROR: Lost sync" << std::endl;
			ambeq.dequeue();
		}
		if(ambeq.size() < 15){
			//std::cerr << "ERROR:  AMBE Q empty" << std::endl;
			return;
		}
		else{
			for (int i = 0; i < 6; ++i){
				ambeq.dequeue();
			}
		}
        txdata.resize(29);
        txdata[0] = 0x1d;
        txdata[6] = 0x20;
        txdata[16] = tx_cnt % 21;
		if(ambeq.size() > 8){
			txdata[17] = ambeq.dequeue();
			txdata[18] = ambeq.dequeue();
			txdata[19] = ambeq.dequeue();
			txdata[20] = ambeq.dequeue();
			txdata[21] = ambeq.dequeue();
			txdata[22] = ambeq.dequeue();
			txdata[23] = ambeq.dequeue();
			txdata[24] = ambeq.dequeue();
			txdata[25] = ambeq.dequeue();
		}

		//for(int i = 0; i < 9; ++i){
			//txdata[17 + i] = ad8dp[(tx_cnt * 9) + i];
			//if(ambeq.size()){
			//	txdata[17 + i] = ambeq.dequeue();
			//}
			//else{
			//	txdata[17 + i] = 0;
			//}
		//}

		//memset(txdata.data() + 17, 0x00, 9);
        switch(txdata.data()[16]){
        case 0:
            txdata[26] = 0x55;
            txdata[27] = 0x2d;
            txdata[28] = 0x16;
            break;
        case 1:
            txdata[26] = 0x40 ^ 0x70;
            txdata[27] = ui->usertxtEdit->text().toLocal8Bit().data()[0] ^ 0x4f;
            txdata[28] = ui->usertxtEdit->text().toLocal8Bit().data()[1] ^ 0x93;
            break;
        case 2:
            txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[2] ^ 0x70;
            txdata[27] = ui->usertxtEdit->text().toLocal8Bit().data()[3] ^ 0x4f;
            txdata[28] = ui->usertxtEdit->text().toLocal8Bit().data()[4] ^ 0x93;
            break;
        case 3:
            txdata[26] = 0x41 ^ 0x70;
            txdata[27] = ui->usertxtEdit->text().toLocal8Bit().data()[5] ^ 0x4f;
            txdata[28] = ui->usertxtEdit->text().toLocal8Bit().data()[6] ^ 0x93;
            break;
        case 4:
            txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[7] ^ 0x70;
            txdata[27] = ui->usertxtEdit->text().toLocal8Bit().data()[8] ^ 0x4f;
            txdata[28] = ui->usertxtEdit->text().toLocal8Bit().data()[9] ^ 0x93;
            break;
        case 5:
            txdata[26] = 0x42 ^ 0x70;
            txdata[27] = ui->usertxtEdit->text().toLocal8Bit().data()[10] ^ 0x4f;
            txdata[28] = ui->usertxtEdit->text().toLocal8Bit().data()[11] ^ 0x93;
            break;
        case 6:
            txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[12] ^ 0x70;
            txdata[27] = ui->usertxtEdit->text().toLocal8Bit().data()[13] ^ 0x4f;
            txdata[28] = ui->usertxtEdit->text().toLocal8Bit().data()[14] ^ 0x93;
            break;
        case 7:
            txdata[26] = 0x43 ^ 0x70;
            txdata[27] = ui->usertxtEdit->text().toLocal8Bit().data()[15] ^ 0x4f;
            txdata[28] = ui->usertxtEdit->text().toLocal8Bit().data()[16] ^ 0x93;
            break;
        case 8:
            txdata[26] = ui->usertxtEdit->text().toLocal8Bit().data()[17] ^ 0x70;
            txdata[27] = ui->usertxtEdit->text().toLocal8Bit().data()[18] ^ 0x4f;
            txdata[28] = ui->usertxtEdit->text().toLocal8Bit().data()[19] ^ 0x93;
            break;
        default:
            txdata[26] = 0x16;
            txdata[27] = 0x29;
            txdata[28] = 0xf5;
            break;
        }
        udp->writeDatagram(txdata, address, 20001);
        ++tx_cnt;
		//if((tx_cnt * 9) >= sizeof(ad8dp)){
		//	tx_cnt = 0;
		//}
        if((tx_cnt % 21) == 0){
            sendheader = 1;
        }
    }
	else {
		if(!ambeq.size()){
			txdata.resize(32);
			txdata[0] = 0x20;
			txdata[6] = 0x20;
			txdata[16] = tx_cnt % 21;
			memset(txdata.data() + 17, 0, 9);
			txdata[26] = 0x55;
			txdata[27] = 0x55;
			txdata[28] = 0x55;
			txdata[29] = 0x55;
			txdata[30] = 0xc8;
			txdata[31] = 0x7a;
			udp->writeDatagram(txdata, address, 20001);
			tx_cnt = 0;
			txstreamid = 0;
			sendheader = 1;
			txtimer->stop();
			audioindev->disconnect();
			audioin->stop();
		}
    }
#ifdef DEBUG
	fprintf(stderr, "SEND:%d: ", ambeq.size());
    for(int i = 0; i < txdata.size(); ++i){
        fprintf(stderr, "%02x ", (unsigned char)txdata.data()[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
#endif
}

void DudeStar::calcPFCS(char *d)
{
   int crc = 65535;
   int poly = 32840;
   int i,j;
   char b;
   bool bit;
   bool c15;

   for (j = 17; j < 41; ++j){
      b = d[j];
      for (i = 0; i < 8; ++i) {
		 bit = ((b >> 7) - i & 0x1) == 1;
         c15 = (crc >> 15 & 0x1) == 1;
         crc <<= 1;
         if (c15 & bit)
            crc ^= poly;
      }
   }

   crc ^= 65535;
   d[56] = (char)(crc & 0xFF);
   d[57] = (char)(crc >> 8 & 0xFF);
}
